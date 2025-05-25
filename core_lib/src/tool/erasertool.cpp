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

ToolType EraserTool::type()
{
    return ERASER;
}

void EraserTool::loadSettings()
{
    StrokeTool::loadSettings();

    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[USEFEATHER] = true;
    mPropertyEnabled[FEATHER] = true;
    mPropertyEnabled[USEFEATHER] = true;
    mPropertyEnabled[PRESSURE] = true;
    mPropertyEnabled[STABILIZATION] = true;
    mPropertyEnabled[ANTI_ALIASING] = true;

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.width = settings.value("eraserWidth", 24.0).toDouble();
    properties.feather = settings.value("eraserFeather", 48.0).toDouble();
    properties.useFeather = settings.value("eraserUseFeather", true).toBool();
    properties.pressure = settings.value("eraserPressure", true).toBool();
    properties.invisibility = DISABLED;
    properties.preserveAlpha = OFF;
    properties.stabilizerLevel = settings.value("stabilizerLevel", StabilizationLevel::NONE).toInt();
    properties.useAA = settings.value("eraserAA", 1).toInt();

    if (properties.useFeather) { properties.useAA = -1; }

    mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
    mQuickSizingProperties.insert(Qt::ControlModifier, FEATHER);
}

void EraserTool::saveSettings()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.setValue("eraserWidth", properties.width);
    settings.setValue("eraserFeather", properties.feather);
    settings.setValue("eraserUseFeather", properties.useFeather);
    settings.setValue("eraserPressure", properties.pressure);
    settings.setValue("eraserAA", properties.useAA);
    settings.setValue("stabilizerLevel", properties.stabilizerLevel);

    settings.sync();
}

void EraserTool::resetToDefault()
{
    setWidth(24.0);
    setFeather(48.0);
    setUseFeather(true);
    setPressure(true);
    setAA(true);
    setStabilizerLevel(StabilizationLevel::NONE);
}

void EraserTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;
}

void EraserTool::setUseFeather(const bool usingFeather)
{
    // Set current property
    properties.useFeather = usingFeather;
}

void EraserTool::setFeather(const qreal feather)
{
    // Set current property
    properties.feather = feather;
}

void EraserTool::setPressure(const bool pressure)
{
    // Set current property
    properties.pressure = pressure;
}

void EraserTool::setAA(const int AA)
{
    // Set current property
    properties.useAA = AA;
}

void EraserTool::setStabilizerLevel(const int level)
{
    properties.stabilizerLevel = level;
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

void EraserTool::pointerMoveEvent(PointerEvent* event)
{
    mInterpolator.pointerMoveEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        updateStrokes();
        if (properties.stabilizerLevel != mInterpolator.getStabilizerLevel())
        {
            mInterpolator.setStabilizerLevel(properties.stabilizerLevel);
        }
    }

    StrokeTool::pointerMoveEvent(event);
}

void EraserTool::pointerReleaseEvent(PointerEvent *event)
{
    StrokeTool::pointerReleaseEvent(event);
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
                             properties.useFeather,
                             properties.useAA == ON);
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
        qreal radius = properties.width / 2;

        VectorImage* currKey = static_cast<VectorImage*>(layer->getLastKeyFrameAtPosition(mEditor->currentFrame()));
        QList<VertexRef> nearbyVertices = currKey->getVerticesCloseTo(getCurrentPoint(), radius);
        for (auto nearbyVertice : nearbyVertices)
        {
            currKey->setSelected(nearbyVertice, true);
        }
    }
}
