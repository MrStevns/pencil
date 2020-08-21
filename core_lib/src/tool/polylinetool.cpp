/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "polylinetool.h"


#include "editor.h"
#include "scribblearea.h"
#include "mphandler.h"
#include "pencilsettings.h"

#include "strokemanager.h"
#include "layermanager.h"
#include "colormanager.h"
#include "viewmanager.h"
#include "pointerevent.h"
#include "layervector.h"
#include "layerbitmap.h"
#include "vectorimage.h"


PolylineTool::PolylineTool(QObject* parent) : StrokeTool(parent)
{
}

ToolType PolylineTool::type()
{
    return POLYLINE;
}

void PolylineTool::loadSettings()
{
    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[BEZIER] = true;
    mPropertyEnabled[ANTI_ALIASING] = true;

    mDefaultBrushSettings = { RadiusLog, Hardness, Opacity };

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.width = settings.value("polyLineWidth", 8.0).toDouble();
    properties.feather = -1;
    properties.pressure = false;
    properties.invisibility = OFF;
    properties.preserveAlpha = OFF;
    properties.stabilizerLevel = -1;

    mQuickSizingProperties.insert(Qt::ShiftModifier, QuickPropertyType::WIDTH);
}

void PolylineTool::resetToDefault()
{
    setWidth(8.0);
    setBezier(false);
}

void PolylineTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("polyLineWidth", width);
    settings.sync();
}

void PolylineTool::setFeather(const qreal feather)
{
    Q_UNUSED(feather)
    properties.feather = -1;
}

bool PolylineTool::isActive()
{
    return !mPoints.isEmpty();
}

QCursor PolylineTool::cursor()
{
    return Qt::CrossCursor;
}

void PolylineTool::clearToolData()
{
    mPoints.clear();
}

void PolylineTool::pointerPressEvent(PointerEvent* event)
{
    Layer* layer = mEditor->layers()->currentLayer();

    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    if (layer->type() == Layer::VECTOR) {
        pointerPressOnVector(event);
    } else if (layer->type() == Layer::BITMAP) {
        pointerPressOnBitmap(event);
    }

    if (previousPoint.isNull()) {
        mPoints << getCurrentPoint();
    } else {
        mPoints << previousPoint;
    }
    mScribbleArea->setAllDirty();
}

void PolylineTool::pointerPressOnVector(PointerEvent*)
{
    Layer* layer = mEditor->layers()->currentLayer();

    mEditor->backup(typeName());
    mScribbleArea->handleDrawingOnEmptyFrame();

    mScribbleArea->clearBitmapBuffer();
    static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0)->deselectAll();
    if (mScribbleArea->makeInvisible() && !mEditor->preference()->isOn(SETTING::INVISIBLE_LINES))
    {
        mScribbleArea->toggleThinLines();
    }
}

void PolylineTool::pointerPressOnBitmap(PointerEvent*)
{
    mScribbleArea->mMyPaint->clearSurface();

    mEditor->backup(typeName());
    mScribbleArea->handleDrawingOnEmptyFrame();

    if (!mPoints.isEmpty()) {

        for(int i=0; i < mPoints.size(); i++) {
            drawStroke(mPoints[i]);
        }

        mPoints.takeAt(0);
    }

    mScribbleArea->setIsPainting(true);

    mScribbleArea->paintBitmapBuffer(QPainter::CompositionMode_SourceOver);
    mScribbleArea->clearTilesBuffer();
}

void PolylineTool::pointerMoveEvent(PointerEvent* event)
{
    Layer* layer = mEditor->layers()->currentLayer();

    if (mPoints.size() > 0)
    {
        if (layer->type() == Layer::VECTOR)
        {
            pointerMoveOnVector(event);
        } else if (layer->type() == Layer::BITMAP) {
            pointerMoveOnBitmap(event);
        }
    }
    previousPoint = getCurrentPoint();
}

void PolylineTool::pointerMoveOnVector(PointerEvent *)
{
    QPen pen(mEditor->color()->frontColor(),
             properties.width,
             Qt::SolidLine,
             Qt::RoundCap,
             Qt::RoundJoin);

    QPainterPath tempPath;
    if (properties.bezier_state)
    {
        tempPath = BezierCurve(mPoints).getSimplePath();
    }
    else
    {
        tempPath = BezierCurve(mPoints).getStraightPath();
    }
    tempPath.lineTo(getCurrentPoint());

    tempPath = mEditor->view()->mapCanvasToScreen(tempPath);
    if (mScribbleArea->makeInvisible() == true)
    {
        pen.setWidth(0);
        pen.setStyle(Qt::DotLine);
    }
    else
    {
        pen.setWidth(properties.width * mEditor->view()->scaling());
    }

    mScribbleArea->drawPolyline(tempPath, pen, true);
}

void PolylineTool::pointerMoveOnBitmap(PointerEvent *)
{
    mScribbleArea->mMyPaint->clearSurface();
    for(int i=0; i<mPoints.size(); i++) {
        mScribbleArea->mMyPaint->startStroke();
        drawStroke(mPoints[i]);
    }
    drawStroke(getCurrentPoint());

    mScribbleArea->updateCurrentFrame();
}

void PolylineTool::pointerReleaseEvent(PointerEvent *)
{
}

void PolylineTool::pointerDoubleClickEvent(PointerEvent*)
{
    // include the current point before ending the line.
    mPoints << getCurrentPoint();

    endPolyline(mPoints);
    clearToolData();
}


bool PolylineTool::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Return:
        if (mPoints.size() > 0)
        {
            endPolyline(mPoints);
            clearToolData();
            return true;
        }
        break;

    case Qt::Key_Escape:
        if (mPoints.size() > 0)
        {
            cancelPolyline();
            clearToolData();
            return true;
        }
        break;

    default:
        return false;
    }

    return false;
}

void PolylineTool::drawPolyline(QList<QPointF> points, QPointF endPoint)
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (points.size() > 0)
    {

        if (layer->type() == Layer::BITMAP) {
            mScribbleArea->mMyPaint->clearSurface();
            mScribbleArea->mMyPaint->startStroke();
            for(int i=0; i<points.size(); i++) {
                drawStroke(points[i]);
            }
            drawStroke(endPoint);
            mScribbleArea->updateCurrentFrame();
            mScribbleArea->mMyPaint->endStroke();
        }
        else if (layer->type() == Layer::VECTOR)
        {
            QPen pen(mEditor->color()->frontColor(),
                     properties.width,
                     Qt::SolidLine,
                     Qt::RoundCap,
                     Qt::RoundJoin);
            Layer* layer = mEditor->layers()->currentLayer();

            // Bitmap by default
            QPainterPath tempPath;
            if (properties.bezier_state)
            {
                tempPath = BezierCurve(points).getSimplePath();
            }
            else
            {
                tempPath = BezierCurve(points).getStraightPath();
            }
            tempPath.lineTo(endPoint);

            // Vector otherwise
            if (layer->type() == Layer::VECTOR)
            {
                if (mEditor->layers()->currentLayer()->type() == Layer::VECTOR)
                {
                    tempPath = mEditor->view()->mapCanvasToScreen(tempPath);
                    if (mScribbleArea->makeInvisible() == true)
                    {
                        pen.setWidth(0);
                        pen.setStyle(Qt::DotLine);
                    }
                    else
                    {
                        pen.setWidth(properties.width * mEditor->view()->scaling());
                    }
                }
            }

            mScribbleArea->drawPolyline(tempPath, pen, true);
        }
    }
}


void PolylineTool::cancelPolyline()
{
    // Clear the in-progress polyline from the bitmap buffer.
    mScribbleArea->clearBitmapBuffer();
    mScribbleArea->updateCurrentFrame();
}

void PolylineTool::endPolyline(QList<QPointF> points)
{
    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::VECTOR)
    {
        mScribbleArea->clearBitmapBuffer();
        BezierCurve curve = BezierCurve(points, properties.bezier_state);
        if (mScribbleArea->makeInvisible() == true)
        {
            curve.setWidth(0);
        }
        else
        {
            curve.setWidth(properties.width);
        }
        curve.setColorNumber(mEditor->color()->frontColorNumber());
        curve.setVariableWidth(false);
        curve.setInvisibility(mScribbleArea->makeInvisible());

        static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0)->addCurve(curve, mEditor->view()->scaling());
    }
    if (layer->type() == Layer::BITMAP)
    {
        drawPolyline(points, points.last());
        mScribbleArea->prepareForDrawing();
    }
    mScribbleArea->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());

    endStroke();
}
