/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "penciltool.h"

#include <QSettings>
#include <QPixmap>
#include "pointerevent.h"

#include "layermanager.h"
#include "colormanager.h"
#include "strokemanager.h"
#include "viewmanager.h"
#include "preferencemanager.h"
#include "selectionmanager.h"

#include "editor.h"
#include "scribblearea.h"
#include "blitrect.h"
#include "layervector.h"
#include "vectorimage.h"


PencilTool::PencilTool(QObject* parent) : StrokeTool(parent)
{
}

void PencilTool::loadSettings()
{
    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[PRESSURE] = true;
    mPropertyEnabled[VECTORMERGE] = false;
    mPropertyEnabled[STABILIZATION] = true;
    mPropertyEnabled[FILLCONTOUR] = true;

    QSettings settings(PENCIL2D, PENCIL2D);
    properties.width = settings.value("pencilWidth", 4).toDouble();
    properties.feather = 50;
    properties.pressure = settings.value("pencilPressure", true).toBool();
    properties.stabilizerLevel = settings.value("pencilLineStabilization", StabilizationLevel::STRONG).toInt();
    properties.useFeather = true;
    properties.useFillContour = false;

    mQuickSizingProperties.insert(Qt::ShiftModifier, QuickPropertyType::WIDTH);
}

void PencilTool::resetToDefault()
{
    setWidth(4.0);
    setFeather(50);
    setUseFeather(true);
    setStabilizerLevel(StabilizationLevel::STRONG);
}

void PencilTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("pencilWidth", width);
    settings.sync();
}

void PencilTool::setFeather(const qreal feather)
{
    properties.feather = feather;
}

void PencilTool::setUseFeather(const bool usingFeather)
{
    // Set current property
    properties.useFeather = usingFeather;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("brushUseFeather", usingFeather);
    settings.sync();
}

void PencilTool::setInvisibility(const bool)
{
    // force value
    properties.invisibility = 1;
}

void PencilTool::setPressure(const bool pressure)
{
    // Set current property
    properties.pressure = pressure;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("pencilPressure", pressure);
    settings.sync();
}

void PencilTool::setPreserveAlpha(const bool preserveAlpha)
{
    // force value
    Q_UNUSED( preserveAlpha );
    properties.preserveAlpha = 0;
}

void PencilTool::setStabilizerLevel(const int level)
{
    properties.stabilizerLevel = level;

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("pencilLineStabilization", level);
    settings.sync();
}

void PencilTool::setUseFillContour(const bool useFillContour)
{
    properties.useFillContour = useFillContour;

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("FillContour", useFillContour);
    settings.sync();
}

QCursor PencilTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        return QCursor(QPixmap(":icons/pencil2.png"), 0, 16);
    }
    return QCursor(QPixmap(":icons/cross.png"), 10, 10);
}

void PencilTool::pointerPressEvent(PointerEvent *event)
{
    mMouseDownPoint = getCurrentPoint();
    mLastBrushPoint = getCurrentPoint();

    startStroke(event->inputType());

    // note: why are we doing this on device press event?
    if ( !mEditor->preference()->isOn(SETTING::INVISIBLE_LINES) )
    {
        mScribbleArea->toggleThinLines();
    }
}

void PencilTool::pointerMoveEvent(PointerEvent* event)
{
    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        mCurrentPressure = strokeManager()->getPressure();
        drawStroke(event);
        if (properties.stabilizerLevel != strokeManager()->getStabilizerLevel())
            strokeManager()->setStabilizerLevel(properties.stabilizerLevel);
    }
}

void PencilTool::pointerReleaseEvent(PointerEvent *event)
{
    if (event->inputType() != mCurrentInputType) return;
    endStroke();
}

void PencilTool::endStroke()
{
    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::BITMAP)
        StrokeTool::paintBitmapStroke();
    else if (layer->type() == Layer::VECTOR)
        paintVectorStroke();

    StrokeTool::endStroke();
}

void PencilTool::drawStroke(PointerEvent* event)
{
    StrokeTool::drawStroke(event);

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::VECTOR)
    {
        QList<QPointF> p = strokeManager()->interpolateStroke();
        properties.useFeather = false;
        mCurrentWidth = 0; // FIXME: WTF?
        QPen pen(mEditor->color()->frontColor(),
                 1,
                 Qt::DotLine,
                 Qt::RoundCap,
                 Qt::MiterJoin);

        int rad = qRound((mCurrentWidth / 2 + 2) * mEditor->view()->scaling());

        if (p.size() >= 2)
        {
            QPainterPath path(p.first());
            path.quadTo(p.first(),
                         p.last());
            mScribbleArea->drawPath(path, pen, Qt::NoBrush, QPainter::CompositionMode_Source);
            mScribbleArea->refreshVector(path.boundingRect().toRect(), rad);
        }
    }
}

void PencilTool::paintVectorStroke()
{
    if (mStrokePoints.empty())
        return;

    Layer* layer = mEditor->layers()->currentLayer();
    // Clear the temporary pixel path
    mScribbleArea->clearBitmapBuffer();
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(0);
    curve.setFeather(0);
    curve.setFilled(false);
    curve.setInvisibility(true);
    curve.setVariableWidth(false);
    curve.setColorNumber(mEditor->color()->frontColorNumber());
    VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);

    vectorImage->addCurve(curve, qAbs(mEditor->view()->scaling()), properties.vectorMergeEnabled);

    if (properties.useFillContour)
    {
        vectorImage->fillContour(mStrokePoints,
                                 mEditor->color()->frontColorNumber());
    }

    if (vectorImage->isAnyCurveSelected() || mEditor->select()->somethingSelected())
    {
        mEditor->deselectAll();
    }

    // select last/newest curve
    vectorImage->setSelected(vectorImage->getLastCurveNumber(), true);

    // TODO: selection doesn't apply on enter

    mScribbleArea->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}
