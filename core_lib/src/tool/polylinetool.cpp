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

bool PolylineTool::leavingThisTool()
{
    if (mPoints.size() > 0)
    {
        cancelPolyline();
        clearToolData();
    }
    return true;
}

bool PolylineTool::isActive()
{
    return !mPoints.isEmpty();
}

QCursor PolylineTool::cursor()
{
    return QCursor(QPixmap(":icons/cross.png"), 10, 10);
}

void PolylineTool::clearToolData()
{
    mPoints.clear();
    emit isActiveChanged(POLYLINE, false);
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

    emit isActiveChanged(POLYLINE, true);
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

void PolylineTool::pointerPressOnBitmap(PointerEvent* event)
{
    mScribbleArea->mMyPaint->clearSurface();

    mEditor->backup(typeName());
    mScribbleArea->handleDrawingOnEmptyFrame();

    if (!mPoints.isEmpty()) {

        for(int i=0; i < mPoints.size(); i++) {
            drawStroke(mPoints[i], calculateDeltaTime(event->timeStamp()));
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

void PolylineTool::pointerMoveOnBitmap(PointerEvent * event)
{
    mScribbleArea->mMyPaint->clearSurface();
    mScribbleArea->mMyPaint->startStroke();

    // TODO: can we get the old bezier functionality back when using mypaint?
    for(int i=0; i<mPoints.size(); i++) {

        drawStroke(mPoints[i], calculateDeltaTime(event->timeStamp()));
    }
    drawStroke(getCurrentPoint(), calculateDeltaTime(event->timeStamp()));

    mScribbleArea->updateCurrentFrame();
}

double PolylineTool::calculateDeltaTime(quint64)
{
    // FIXME: Variable dt won't work, will probably have to rework how the polyline is drawn
    return 1;
}

void PolylineTool::pointerReleaseEvent(PointerEvent *)
{
}

void PolylineTool::pointerDoubleClickEvent(PointerEvent* event)
{
    // include the current point before ending the line.
    mPoints << getCurrentPoint();

    endPolyline(mPoints, event->timeStamp());
    clearToolData();
}


bool PolylineTool::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Return:
        if (mPoints.size() > 0)
        {
            endPolyline(mPoints, calculateDeltaTime(event->timestamp()));
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
        break;
    }

    return BaseTool::keyPressEvent(event);
}

void PolylineTool::drawPolyline(QList<QPointF> points, QPointF endPoint, quint64 timeStamp)
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (points.size() > 0)
    {

        if (layer->type() == Layer::BITMAP) {
            mScribbleArea->mMyPaint->clearSurface();
            mScribbleArea->mMyPaint->startStroke();

            for(int i=0; i<points.size(); i++) {
                drawStroke(points[i], calculateDeltaTime(timeStamp));
            }
            drawStroke(endPoint, calculateDeltaTime(timeStamp));
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

void PolylineTool::endPolyline(QList<QPointF> points, quint64 timeStamp)
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
        for(int i=0; i<mPoints.size(); i++) {

            drawStroke(mPoints[i], calculateDeltaTime(timeStamp));
        }
        drawStroke(getCurrentPoint(), calculateDeltaTime(timeStamp));
    }

    mScribbleArea->clearBitmapBuffer();
    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());

    endStroke();
}
