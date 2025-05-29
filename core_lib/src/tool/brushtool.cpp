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

#include "brushtool.h"

#include <cmath>
#include <QSettings>
#include <QPixmap>
#include <QPainter>
#include <QColor>

#include "beziercurve.h"
#include "vectorimage.h"
#include "editor.h"
#include "colormanager.h"
#include "layermanager.h"
#include "viewmanager.h"
#include "selectionmanager.h"
#include "undoredomanager.h"
#include "scribblearea.h"
#include "pointerevent.h"


BrushTool::BrushTool(QObject* parent) : StrokeTool(parent)
{
}

ToolType BrushTool::type() const
{
    return BRUSH;
}

void BrushTool::loadSettings()
{
    StrokeTool::loadSettings();

    mPropertyUsed[StrokeSettings::WIDTH_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::FEATHER_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::PRESSURE_ENABLED] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::INVISIBILITY_ENABLED] = { Layer::VECTOR };
    mPropertyUsed[StrokeSettings::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };

    QSettings settings(PENCIL2D, PENCIL2D);

    QHash<int, PropertyInfo> info;
    info[StrokeSettings::WIDTH_VALUE] = { 1.0, 100.0, 24.0 };
    info[StrokeSettings::FEATHER_VALUE] = { 1.0, 99.0, 48.0 };
    info[StrokeSettings::PRESSURE_ENABLED] = true;
    info[StrokeSettings::INVISIBILITY_ENABLED] = false;
    info[StrokeSettings::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG } ;

    mStrokeSettings->load(typeName(), settings, info);

    if (mStrokeSettings->requireMigration(settings, 1)) {
        mStrokeSettings->setBaseValue(StrokeSettings::WIDTH_VALUE, settings.value("brushWidth", 24.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::FEATHER_VALUE, settings.value("brushFeather", 48.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::PRESSURE_ENABLED, settings.value("brushPressure", true).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::INVISIBILITY_ENABLED, settings.value("brushInvisibility", false).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::STABILIZATION_VALUE, settings.value("brushLineStabilization", StabilizationLevel::STRONG).toInt());

        settings.remove("brushWidth");
        settings.remove("brushFeather");
        settings.remove("brushPressure");
        settings.remove("brushInvisibility");
        settings.remove("brushLineStabilization");
    }

    mInterpolator.setStabilizerLevel(mStrokeSettings->stabilizerLevel());

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeSettings::WIDTH_VALUE);
    mQuickSizingProperties.insert(Qt::ControlModifier, StrokeSettings::FEATHER_VALUE);
}

QCursor BrushTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        return QCursor(QPixmap(":icons/general/cursor-brush.svg"), 4, 14);
    }
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

void BrushTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    mScribbleArea->drawBrush(point,
                             dynamics.width,
                             dynamics.feather,
                             dynamics.color,
                             dynamics.blending,
                             dynamics.opacity,
                             dynamics.antiAliasingEnabled);
}

void BrushTool::drawPath(const QPainterPath& path, QPen pen, QBrush brush)
{
    mScribbleArea->drawPath(path, pen, brush, QPainter::CompositionMode_Source);
}

void BrushTool::drawStroke()
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

// This function uses the points from DrawStroke
// and turns them into vector lines.
void BrushTool::applyVectorBuffer(VectorImage* vectorImage)
{
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(mStrokeSettings->width());
    curve.setFeather(mStrokeSettings->feather());
    curve.setFilled(false);
    curve.setInvisibility(mStrokeSettings->invisibilityEnabled());
    curve.setVariableWidth(mStrokeSettings->pressureEnabled());
    curve.setColorNumber(mEditor->color()->frontColorNumber());

    vectorImage->addCurve(curve, mEditor->view()->scaling(), false);

    if (vectorImage->isAnyCurveSelected() || mEditor->select()->somethingSelected())
    {
        mEditor->deselectAll();
    }

    vectorImage->setSelected(vectorImage->getLastCurveNumber(), true);

    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}
