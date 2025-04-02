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
    mPropertyEnabled[FEATHER] = true;
    mPropertyEnabled[PRESSURE] = true;
    mPropertyEnabled[STABILIZATION] = true;

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.width = settings.value("eraserWidth", 24.0).toDouble();
    properties.feather = settings.value("eraserFeather", 48.0).toDouble();
    properties.pressure = settings.value("eraserPressure", true).toBool();
    properties.invisibility = DISABLED;
    properties.preserveAlpha = OFF;
    properties.stabilizerLevel = settings.value("stabilizerLevel", StabilizationLevel::NONE).toInt();

    mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
}

void EraserTool::saveSettings()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.setValue("eraserWidth", properties.width);
    settings.setValue("eraserFeather", properties.feather);
    settings.setValue("eraserUseFeather", properties.useFeather);
    settings.setValue("eraserPressure", properties.pressure);
    settings.setValue("stabilizerLevel", properties.stabilizerLevel);

    settings.sync();
}

void EraserTool::resetToDefault()
{
    setWidth(24.0);
    setFeather(48.0);
    setPressure(true);
    setStabilizerLevel(StabilizationLevel::NONE);
}

void EraserTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;
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
    mLastBrushPoint = getCurrentPoint();
    mMouseDownPoint = getCurrentPoint();

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
        mCurrentPressure = mInterpolator.getPressure();
        updateStrokes(event);
        if (properties.stabilizerLevel != mInterpolator.getStabilizerLevel())
        {
            mInterpolator.setStabilizerLevel(properties.stabilizerLevel);
        }
    }

    StrokeTool::pointerMoveEvent(event);
}

void EraserTool::pointerReleaseEvent(PointerEvent *event)
{
    mInterpolator.pointerReleaseEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->inputType() != mCurrentInputType) return;
    removeVectorPaint();
    endStroke();

    StrokeTool::pointerReleaseEvent(event);
}

void EraserTool::drawStroke(PointerEvent* event)
{
    StrokeTool::drawStroke(event);
    QList<QPointF> p = mInterpolator.interpolateStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::VECTOR)
    {
        mCurrentWidth = properties.width;
        if (properties.pressure)
        {
            mCurrentWidth = (mCurrentWidth + (mInterpolator.getPressure() * mCurrentWidth)) * 0.5;
        }
        qreal brushWidth = mCurrentWidth;

        QPen pen(Qt::white, brushWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

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

void EraserTool::removeVectorPaint()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::VECTOR)
    {
        mScribbleArea->clearDrawingBuffer();
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
        if (vectorImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing
        // Clear the area containing the last point
        //vectorImage->removeArea(lastPoint);
        // Clear the temporary pixel path
        vectorImage->deleteSelectedPoints();

        mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    }
}

void EraserTool::updateStrokes(PointerEvent* event)
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::BITMAP || layer->type() == Layer::VECTOR)
    {
        drawStroke(event);
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
