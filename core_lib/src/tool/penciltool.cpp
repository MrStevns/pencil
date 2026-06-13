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

    mPropertyUsed[StrokeToolProperties::WIDTH_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeToolProperties::PRESSURE_ENABLED] = { Layer::BITMAP };
    mPropertyUsed[StrokeToolProperties::FILLCONTOUR_ENABLED] = { Layer::VECTOR };
    mPropertyUsed[StrokeToolProperties::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };

    QSettings pencilSettings(PENCIL2D, PENCIL2D);

    QHash<int, PropertyInfo> info;

    info[StrokeToolProperties::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 4.0 };
    info[StrokeToolProperties::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 50.0 };
    info[StrokeToolProperties::PRESSURE_ENABLED] = true;
    info[StrokeToolProperties::FEATHER_ENABLED] = false;
    info[StrokeToolProperties::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG };
    info[StrokeToolProperties::FILLCONTOUR_ENABLED] = false;

    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    if (toolProperties().requireMigration(pencilSettings, ToolProperties::VERSION_1)) {
        toolProperties().setBaseValue(StrokeToolProperties::WIDTH_VALUE, pencilSettings.value("pencilWidth", 4.0).toReal());
        toolProperties().setBaseValue(StrokeToolProperties::PRESSURE_ENABLED, pencilSettings.value("pencilPressure", true).toBool());
        toolProperties().setBaseValue(StrokeToolProperties::STABILIZATION_VALUE, pencilSettings.value("pencilLineStabilization", StabilizationLevel::STRONG).toInt());
        toolProperties().setBaseValue(StrokeToolProperties::FILLCONTOUR_ENABLED, pencilSettings.value("FillContour", false).toBool());

        pencilSettings.remove("pencilWidth");
        pencilSettings.remove("pencilPressure");
        pencilSettings.remove("pencilLineStabilization");
        pencilSettings.remove("FillContour");
    }

    toolProperties().setBaseValue(StrokeToolProperties::FEATHER_VALUE, info[StrokeToolProperties::FEATHER_VALUE].defaultReal());

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeToolProperties::WIDTH_VALUE);
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

    mMouseDownPoint = getCurrentPoint();
    mLastBrushPoint = getCurrentPoint();

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
        mCurrentPressure = mInterpolator.getPressure();
        drawStroke(event);
        if (mSettings.stabilizerLevel() != mInterpolator.getStabilizerLevel())
        {
            mInterpolator.setStabilizerLevel(mSettings.stabilizerLevel());
        }
    }
    StrokeTool::pointerMoveEvent(event);
}

void PencilTool::pointerReleaseEvent(PointerEvent *event)
{
    mInterpolator.pointerReleaseEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->inputType() != mCurrentInputType) return;

    mEditor->backup(typeName());

    drawStroke(event);

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::VECTOR) {
        paintVectorStroke(layer);
    }
    endStroke();

    StrokeTool::pointerReleaseEvent(event);
}

void PencilTool::drawStroke(PointerEvent* event)
{
    StrokeTool::drawStroke();
    QList<QPointF> p = mInterpolator.interpolateStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::BITMAP)
    {
        const float pressure = static_cast<float>(mCurrentPressure);

        double dt = calculateDeltaTime(event->timeStamp());

        if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP) {
            mScribbleArea->strokeTo(getCurrentPoint(), pressure, 0.0f,  0.0f, dt);
        } else {
            // Only mypaint utilizes a strokeTo method currently...
        }
    }
    else if (layer->type() == Layer::VECTOR)
    {
        mCurrentWidth = 0; // FIXME: WTF?
        QPen pen(mEditor->color()->frontColor(),
                 1,
                 Qt::DotLine,
                 Qt::RoundCap,
                 Qt::RoundJoin);

        if (p.size() == 4)
        {
            QPainterPath path(p[0]);
            path.cubicTo(p[1],
                         p[2],
                         p[3]);
            mScribbleArea->drawPath(path, pen, Qt::NoBrush, QPainter::CompositionMode_Source);
        }
    }
}

void PencilTool::paintVectorStroke(Layer* layer)
{
    if (mStrokePoints.empty())
        return;

    // Clear the temporary pixel path
    mScribbleArea->clearDrawingBuffer();
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(0);
    curve.setFeather(0);
    curve.setFilled(false);
    curve.setInvisibility(true);
    curve.setVariableWidth(false);
    curve.setColorNumber(mEditor->color()->frontColorNumber());
    VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame());
    if (vectorImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing
    vectorImage->addCurve(curve, qAbs(mEditor->view()->scaling()), false);

    if (mSettings.fillContourEnabled())
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
