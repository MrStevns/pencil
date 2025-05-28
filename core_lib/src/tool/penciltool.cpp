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

    mPropertyUsed[StrokeSettings::WIDTH_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::PRESSURE_ENABLED] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::FILLCONTOUR_ENABLED] = { Layer::VECTOR };
    mPropertyUsed[StrokeSettings::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };

    QSettings settings(PENCIL2D, PENCIL2D);

    QHash<int, PropertyInfo> info;

    info[StrokeSettings::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 4.0 };
    info[StrokeSettings::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 50.0 };
    info[StrokeSettings::PRESSURE_ENABLED] = true;
    info[StrokeSettings::FEATHER_ENABLED] = false;
    info[StrokeSettings::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG };
    info[StrokeSettings::FILLCONTOUR_ENABLED] = false;

    mStrokeSettings->load(typeName(), settings, info);

    if (mStrokeSettings->requireMigration(settings, 1)) {
        mStrokeSettings->setBaseValue(StrokeSettings::WIDTH_VALUE, settings.value("pencilWidth", 4.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::PRESSURE_ENABLED, settings.value("pencilPressure", true).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::STABILIZATION_VALUE, settings.value("pencilLineStabilization", StabilizationLevel::STRONG).toInt());
        mStrokeSettings->setBaseValue(StrokeSettings::FILLCONTOUR_ENABLED, settings.value("FillContour", false).toBool());

        settings.remove("pencilWidth");
        settings.remove("pencilPressure");
        settings.remove("pencilLineStabilization");
        settings.remove("FillContour");
    }

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeSettings::WIDTH_VALUE);
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

    vectorImage->addCurve(curve, qAbs(mEditor->view()->scaling()), false);

    if (mStrokeSettings->fillContourEnabled())
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
