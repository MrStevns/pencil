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
#include "erasertool.h"

#include <QSettings>
#include <QPixmap>
#include <QPainter>

#include "editor.h"
#include "blitrect.h"
#include "scribblearea.h"
#include "layermanager.h"
#include "viewmanager.h"
#include "undoredomanager.h"
#include "layervector.h"
#include "vectorimage.h"
#include "pointerevent.h"


EraserTool::EraserTool(QObject* parent) : StrokeTool(parent)
{
}

ToolType EraserTool::type() const
{
    return ERASER;
}

void EraserTool::loadSettings()
{
    StrokeTool::loadSettings();

    QSettings settings(PENCIL2D, PENCIL2D);

    QHash<int, PropertyInfo> info;

    mPropertyUsed[StrokeSettings::WIDTH_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::FEATHER_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::FEATHER_ENABLED] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::PRESSURE_ENABLED] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::ANTI_ALIASING_ENABLED] = { Layer::BITMAP };

    info[StrokeSettings::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 24.0 };
    info[StrokeSettings::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 48.0 };
    info[StrokeSettings::FEATHER_ENABLED] = true;
    info[StrokeSettings::PRESSURE_ENABLED] = true;
    info[StrokeSettings::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::NONE };
    info[StrokeSettings::ANTI_ALIASING_ENABLED] = true;

    if (mStrokeSettings->requireMigration(settings, 1)) {
        mStrokeSettings->setBaseValue(StrokeSettings::WIDTH_VALUE, settings.value("eraserWidth", 24.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::FEATHER_VALUE, settings.value("eraserFeather", 48.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::STABILIZATION_VALUE, settings.value("stabilizerLevel", StabilizationLevel::NONE).toInt());
        mStrokeSettings->setBaseValue(StrokeSettings::FEATHER_ENABLED, settings.value("eraserUseFeather", true).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::PRESSURE_ENABLED, settings.value("eraserPressure", true).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::ANTI_ALIASING_ENABLED, settings.value("eraserAA", true).toBool());

        settings.remove("eraserWidth");
        settings.remove("eraserFeather");
        settings.remove("stabilizerLevel");
        settings.remove("eraserUseFeather");
        settings.remove("eraserPressure");
        settings.remove("eraserAA");
    }

    mInterpolator.setStabilizerLevel(mStrokeSettings->stabilizerLevel());

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeSettings::WIDTH_VALUE);
    mQuickSizingProperties.insert(Qt::ControlModifier, StrokeSettings::FEATHER_VALUE);
}

QCursor EraserTool::cursor()
{
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

void EraserTool::pointerPressEvent(PointerEvent *event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    startStroke(event->inputType());

    StrokeTool::pointerPressEvent(event);
}

StrokeDynamics EraserTool::createDynamics() const
{
    StrokeDynamics dynamics = StrokeTool::createDynamics();

    dynamics.color = Qt::white;

    return dynamics;
}

void EraserTool::drawStroke()
{
    StrokeTool::drawStroke();
    QList<QPointF> p = mInterpolator.interpolateStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::BITMAP)
    {
        doStroke();
    }
    else if (layer->type() == Layer::VECTOR)
    {
        const StrokeDynamics& dynamics = createDynamics();
        QPen pen(Qt::white, dynamics.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

        doPath(mStrokeSegment, Qt::NoBrush, pen);
    }
}

void EraserTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    mScribbleArea->drawBrush(point,
                             dynamics.width,
                             dynamics.feather,
                             dynamics.color,
                             dynamics.blending,
                             dynamics.opacity,
                             mStrokeSettings->featherEnabled(),
                             mStrokeSettings->AntiAliasingEnabled() == ON);
}

void EraserTool::drawPath(const QPainterPath& path, QPen pen, QBrush brush)
{
    mScribbleArea->drawPath(path, pen, brush, QPainter::CompositionMode_Source);
}

void EraserTool::applyVectorBuffer(VectorImage* vectorImage)
{
    // Clear the area containing the current point
    vectorImage->removeArea(getCurrentPoint());
    // Clear the temporary pixel path
    vectorImage->deleteSelectedPoints();

    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());

    StrokeTool::applyVectorBuffer(vectorImage);
}

void EraserTool::updateStrokes()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::BITMAP || layer->type() == Layer::VECTOR)
    {
        drawStroke();
    }

    if (layer->type() == Layer::VECTOR)
    {
        qreal radius = mStrokeSettings->width() / 2;

        VectorImage* currKey = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(mEditor->currentFrame()));
        QList<VertexRef> nearbyVertices = currKey->getVerticesCloseTo(getCurrentPoint(), radius);
        for (auto nearbyVertice : nearbyVertices)
        {
            currKey->setSelected(nearbyVertice, true);
        }
    }
}
