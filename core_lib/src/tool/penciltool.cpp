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
#include "viewmanager.h"
#include "preferencemanager.h"
#include "selectionmanager.h"
#include "undoredomanager.h"

#include "editor.h"
#include "scribblearea.h"
#include "layervector.h"
#include "vectorimage.h"


PencilTool::PencilTool(QObject* parent) : StrokeTool(parent)
{
}

void PencilTool::loadSettings()
{
    StrokeTool::loadSettings();

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
    properties.useAA = DISABLED;
    properties.useFillContour = false;

    mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
}

void PencilTool::saveSettings()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.setValue("pencilWidth", properties.width);
    settings.setValue("pencilPressure", properties.pressure);
    settings.setValue("brushUseFeather", properties.useFeather);
    settings.setValue("pencilLineStabilization", properties.stabilizerLevel);
    settings.setValue("FillContour", properties.useFillContour);

    settings.sync();
}

void PencilTool::resetToDefault()
{
    setWidth(4.0);
    setFeather(50);
    setUseFeather(false);
    setStabilizerLevel(StabilizationLevel::STRONG);
}

void PencilTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;
}

void PencilTool::setFeather(const qreal feather)
{
    properties.feather = feather;
}

void PencilTool::setUseFeather(const bool usingFeather)
{
    // Set current property
    properties.useFeather = usingFeather;

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
}

void PencilTool::setPreserveAlpha(const bool preserveAlpha)
{
    // force value
    Q_UNUSED( preserveAlpha )
    properties.preserveAlpha = 0;
}

void PencilTool::setStabilizerLevel(const int level)
{
    properties.stabilizerLevel = level;
}

void PencilTool::setUseFillContour(const bool useFillContour)
{
    properties.useFillContour = useFillContour;
}

QCursor PencilTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        return QCursor(QPixmap(":icons/general/cursor-pencil.svg"), 4, 14);
    }
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

void PencilTool::pointerPressEvent(PointerEvent *event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    startStroke(event->inputType());

    // note: why are we doing this on device press event?
    if (mEditor->layers()->currentLayer()->type() == Layer::VECTOR && !mEditor->preference()->isOn(SETTING::INVISIBLE_LINES))
    {
        mScribbleArea->toggleThinLines();
    }

    StrokeTool::pointerPressEvent(event);
}

void PencilTool::pointerMoveEvent(PointerEvent* event)
{
    mInterpolator.pointerMoveEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        drawStroke();
    }
    StrokeTool::pointerMoveEvent(event);
}

void PencilTool::pointerReleaseEvent(PointerEvent *event)
{
    StrokeTool::pointerReleaseEvent(event);
}

void PencilTool::drawStroke()
{
    StrokeTool::drawStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::BITMAP)
    {
        doStroke();
    }
    else if (layer->type() == Layer::VECTOR)
    {
        const StrokeDynamics& dynamics = createDynamics();
        QPen pen(dynamics.color,
                 1,
                 Qt::DotLine,
                 Qt::RoundCap,
                 Qt::RoundJoin);

        doPath(mStrokeSegment, Qt::NoBrush, pen);
    }
}

void PencilTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    mScribbleArea->drawPencil(point,
                              dynamics.width,
                              dynamics.feather,
                              dynamics.color,
                              dynamics.opacity);
}

void PencilTool::drawPath(const QPainterPath & path, QPen pen, QBrush brush)
{
    mScribbleArea->drawPath(path, pen, brush, QPainter::CompositionMode_Source);
}

void PencilTool::applyVectorBuffer(VectorImage* vectorImage)
{
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(0);
    curve.setFeather(0);
    curve.setFilled(false);
    curve.setInvisibility(true);
    curve.setVariableWidth(false);
    curve.setColorNumber(mEditor->color()->frontColorNumber());

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

    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}
