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
#include "pentool.h"

#include <QPixmap>
#include <QSettings>

#include "vectorimage.h"
#include "layervector.h"
#include "colormanager.h"
#include "layermanager.h"
#include "viewmanager.h"
#include "undoredomanager.h"
#include "selectionmanager.h"
#include "editor.h"
#include "scribblearea.h"
#include "blitrect.h"
#include "pointerevent.h"


PenTool::PenTool(QObject* parent) : StrokeTool(parent)
{
}

void PenTool::loadSettings()
{
    StrokeTool::loadSettings();

    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[PRESSURE] = true;
    mPropertyEnabled[VECTORMERGE] = true;
    mPropertyEnabled[ANTI_ALIASING] = true;
    mPropertyEnabled[STABILIZATION] = true;

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.width = settings.value("penWidth", 12.0).toDouble();
    properties.pressure = settings.value("penPressure", true).toBool();
    properties.invisibility = OFF;
    properties.preserveAlpha = OFF;
    properties.useAA = settings.value("penAA", true).toBool();
    properties.stabilizerLevel = settings.value("penLineStabilization", StabilizationLevel::STRONG).toInt();

    mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
}

void PenTool::saveSettings()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.setValue("penWidth", properties.width);
    settings.setValue("penPressure", properties.pressure);
    settings.setValue("penAA", properties.useAA);
    settings.setValue("penLineStabilization", properties.stabilizerLevel);

    settings.sync();
}

void PenTool::resetToDefault()
{
    setWidth(12.0);
    setUseFeather(false);
    setPressure(true);
    setStabilizerLevel(StabilizationLevel::STRONG);
    setAA(1);
}

void PenTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;
}

void PenTool::setPressure(const bool pressure)
{
    // Set current property
    properties.pressure = pressure;
}

void PenTool::setAA(const int AA)
{
    // Set current property
    properties.useAA = AA;

}

void PenTool::setStabilizerLevel(const int level)
{
    properties.stabilizerLevel = level;
}

QCursor PenTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        return QCursor(QPixmap(":icons/general/cursor-pen.svg"), 5, 14);
    }
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

void PenTool::pointerPressEvent(PointerEvent *event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    startStroke(event->inputType());

    StrokeTool::pointerPressEvent(event);
}

void PenTool::pointerMoveEvent(PointerEvent* event)
{
    mInterpolator.pointerMoveEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        drawStroke();
        if (properties.stabilizerLevel != mInterpolator.getStabilizerLevel())
        {
            mInterpolator.setStabilizerLevel(properties.stabilizerLevel);
        }
    }

    StrokeTool::pointerMoveEvent(event);
}

void PenTool::pointerReleaseEvent(PointerEvent *event)
{
    StrokeTool::pointerReleaseEvent(event);
}

StrokeDynamics PenTool::createDynamics() const
{
    StrokeDynamics dynamics = StrokeTool::createDynamics();
    dynamics.opacity = 1.0;

    return dynamics;
}

void PenTool::drawStroke()
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
                 dynamics.width,
                 Qt::SolidLine,
                 Qt::RoundCap,
                 Qt::RoundJoin);

        doPath(mStrokeSegment, Qt::NoBrush, pen);
    }
}

void PenTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    mScribbleArea->drawPen(point,
                           dynamics.width,
                           dynamics.color,
                           properties.useAA);
}

void PenTool::drawPath(const QPainterPath& path, QPen pen, QBrush brush)
{
    mScribbleArea->drawPath(path, pen, brush, QPainter::CompositionMode_Source);
}

void PenTool::applyVectorBuffer(VectorImage* vectorImage)
{
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(properties.width);
    curve.setFeather(properties.feather);
    curve.setFilled(false);
    curve.setInvisibility(properties.invisibility);
    curve.setVariableWidth(properties.pressure);
    curve.setColorNumber(mEditor->color()->frontColorNumber());

    vectorImage->addCurve(curve, mEditor->view()->scaling(), false);

    if (vectorImage->isAnyCurveSelected() || mEditor->select()->somethingSelected())
    {
        mEditor->deselectAll();
    }

    vectorImage->setSelected(vectorImage->getLastCurveNumber(), true);

    StrokeTool::applyVectorBuffer(vectorImage);
}
