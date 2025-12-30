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

    QSettings pencilSettings(PENCIL2D, PENCIL2D);

    QHash<int, PropertyInfo> info;

    mPropertyUsed[StrokeToolProperties::WIDTH_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeToolProperties::FEATHER_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeToolProperties::FEATHER_ENABLED] = { Layer::BITMAP };
    mPropertyUsed[StrokeToolProperties::PRESSURE_ENABLED] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeToolProperties::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeToolProperties::ANTI_ALIASING_ENABLED] = { Layer::BITMAP };

    info[StrokeToolProperties::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 24.0 };
    info[StrokeToolProperties::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 48.0 };
    info[StrokeToolProperties::FEATHER_ENABLED] = true;
    info[StrokeToolProperties::PRESSURE_ENABLED] = true;
    info[StrokeToolProperties::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::NONE };
    info[StrokeToolProperties::ANTI_ALIASING_ENABLED] = true;

    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    if (toolProperties().requireMigration(pencilSettings, ToolProperties::VERSION_1)) {
        toolProperties().setBaseValue(StrokeToolProperties::WIDTH_VALUE, pencilSettings.value("eraserWidth", 24.0).toReal());
        toolProperties().setBaseValue(StrokeToolProperties::FEATHER_VALUE, pencilSettings.value("eraserFeather", 48.0).toReal());
        toolProperties().setBaseValue(StrokeToolProperties::STABILIZATION_VALUE, pencilSettings.value("stabilizerLevel", StabilizationLevel::NONE).toInt());
        toolProperties().setBaseValue(StrokeToolProperties::FEATHER_ENABLED, pencilSettings.value("eraserUseFeather", true).toBool());
        toolProperties().setBaseValue(StrokeToolProperties::PRESSURE_ENABLED, pencilSettings.value("eraserPressure", true).toBool());
        toolProperties().setBaseValue(StrokeToolProperties::ANTI_ALIASING_ENABLED, pencilSettings.value("eraserAA", true).toBool());

        pencilSettings.remove("eraserWidth");
        pencilSettings.remove("eraserFeather");
        pencilSettings.remove("stabilizerLevel");
        pencilSettings.remove("eraserUseFeather");
        pencilSettings.remove("eraserPressure");
        pencilSettings.remove("eraserAA");
    }

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeToolProperties::WIDTH_VALUE);
    mQuickSizingProperties.insert(Qt::ControlModifier, StrokeToolProperties::FEATHER_VALUE);
}

QCursor EraserTool::cursor()
{
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

StrokeDynamics EraserTool::createDynamics() const
{
    StrokeDynamics dynamics = StrokeTool::createDynamics();

    dynamics.color = Qt::white;
    dynamics.blending = QPainter::CompositionMode_DestinationOut;

    return dynamics;
}

void EraserTool::drawStroke()
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
        QPen pen(Qt::white, dynamics.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

        doPath(Qt::NoBrush, pen);
    }
}

void EraserTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    mScribbleArea->drawBrush(point,
                             dynamics.width,
                             dynamics.feather,
                             dynamics.color,
                             // This is deliberately not using dynamics blending
                             // because the stroke is drawn in two passes..
                             // Once with an opaque color here, and the second pass is when
                             // the tiled buffer is composited onto the image.
                             QPainter::CompositionMode_SourceOver,
                             dynamics.opacity,
                             mSettings.featherEnabled(),
                             mSettings.AntiAliasingEnabled());
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

    StrokeTool::applyVectorBuffer(vectorImage);
}
