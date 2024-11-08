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

#include "layermanager.h"
#include "colormanager.h"
#include "viewmanager.h"
#include "undoredomanager.h"
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
    StrokeTool::loadSettings();

    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[BEZIER] = true;
    mPropertyEnabled[CLOSEDPATH] = true;
    mPropertyEnabled[ANTI_ALIASING] = true;

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.width = settings.value("polyLineWidth", 8.0).toDouble();
    properties.feather = -1;
    properties.pressure = false;
    properties.invisibility = OFF;
    properties.preserveAlpha = OFF;
    properties.closedPolylinePath = settings.value("closedPolylinePath").toBool();
    properties.stabilizerLevel = -1;

    mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
}

void PolylineTool::resetToDefault()
{
    setWidth(8.0);
    setBezier(false);
    setClosedPath(false);
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

void PolylineTool::setClosedPath(const bool closed)
{
    BaseTool::setClosedPath(closed);

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("closedPolylinePath", closed);
    settings.sync();
}

bool PolylineTool::leavingThisTool()
{
    StrokeTool::leavingThisTool();
    if (mPoints.size() > 0)
    {
        cancelPolyline();
    }
    return true;
}

bool PolylineTool::isActive() const
{
    return !mPoints.isEmpty();
}

QCursor PolylineTool::cursor()
{
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

void PolylineTool::clearToolData()
{
    if (mPoints.empty()) {
        return;
    }

    mPoints.clear();
    emit isActiveChanged(POLYLINE, false);

    // Clear the in-progress polyline from the bitmap buffer.
    mScribbleArea->clearDrawingBuffer();
    mScribbleArea->updateFrame();
}

void PolylineTool::pointerPressEvent(PointerEvent* event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

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

    StrokeTool::pointerPressEvent(event);
}

void PolylineTool::pointerPressOnVector(PointerEvent*)
{
    Layer* layer = mEditor->layers()->currentLayer();

    mEditor->backup(typeName());
    mScribbleArea->handleDrawingOnEmptyFrame();

    mScribbleArea->clearDrawingBuffer();
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

    mScribbleArea->paintBitmapBuffer();
    mScribbleArea->clearDrawingBuffer();

}

void PolylineTool::pointerMoveEvent(PointerEvent* event)
{
    mInterpolator.pointerMoveEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    drawPolyline(mPoints, getCurrentPoint(), event->timeStamp());
    previousPoint = getCurrentPoint();

    StrokeTool::pointerMoveEvent(event);
}

double PolylineTool::calculateDeltaTime(quint64)
{
    // FIXME: Variable dt won't work, will probably have to rework how the polyline is drawn
    return 1;
}

void PolylineTool::pointerReleaseEvent(PointerEvent* event)
{
    mInterpolator.pointerReleaseEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    StrokeTool::pointerReleaseEvent(event);
}

void PolylineTool::pointerDoubleClickEvent(PointerEvent* event)
{
    mInterpolator.pointerPressEvent(event);
    // include the current point before ending the line.
    mPoints << getCurrentPoint();

    const UndoSaveState* saveState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);
    mEditor->backup(typeName());

    endPolyline(mPoints, event->timeStamp());
    mEditor->undoRedo()->record(saveState, typeName());
}

void PolylineTool::removeLastPolylineSegment()
{
    if (!isActive()) return;

    if (mPoints.size() > 1)
    {
        mPoints.removeLast();
        drawPolyline(mPoints, getCurrentPoint(), calculateDeltaTime(0));
    }
    else if (mPoints.size() == 1)
    {
        cancelPolyline();
        clearToolData();
    }
}

bool PolylineTool::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Control:
        mClosedPathOverrideEnabled = true;
        drawPolyline(mPoints, getCurrentPoint(), calculateDeltaTime(event->timestamp()));
        return true;
        break;

    case Qt::Key_Return:
        if (mPoints.size() > 0)
        {
            const UndoSaveState* saveState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);
            endPolyline(mPoints, calculateDeltaTime(event->timestamp()));
            mEditor->undoRedo()->record(saveState, typeName());
            return true;
        }
        break;

    case Qt::Key_Escape:
        if (mPoints.size() > 0)
        {
            cancelPolyline();
            return true;
        }
        break;

    default:
        break;
    }

    return StrokeTool::keyPressEvent(event);
}

bool PolylineTool::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Control:
        mClosedPathOverrideEnabled = false;
        drawPolyline(mPoints, getCurrentPoint(), event->timestamp());
        return true;
        break;

    default:
        break;
    }

    return BaseTool::keyReleaseEvent(event);
}

void PolylineTool::drawPolyline(QList<QPointF> points, QPointF endPoint, quint64 timeStamp)
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (points.size() > 0)
    {
        if (layer->type() == Layer::BITMAP) {
            BlitRect previousTilesRect = mScribbleArea->mTilesBlitRect;
            mScribbleArea->clearDrawingBuffer();
            mScribbleArea->startStroke();

            for(int i=0; i<points.size(); i++) {
                drawStroke(points[i], calculateDeltaTime(timeStamp));
            }
            drawStroke(endPoint, calculateDeltaTime(timeStamp));

            // In order to clear the polyline overall dirty bounds, we need to do an additional update, otherwise there will be residue
            // of the previous stroke in some cases.
            updateDirtyRect(points, previousTilesRect);
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

            // Ctrl key inverts closed behavior while held (XOR)
            if ((properties.closedPolylinePath == !mClosedPathOverrideEnabled) && points.size() > 1)
            {
                tempPath.closeSubpath();
            }

            // Vector otherwise
            if (layer->type() == Layer::VECTOR)
            {
                if (mScribbleArea->makeInvisible() == true)
                {
                    pen.setWidth(0);
                    pen.setStyle(Qt::DotLine);
                }
                else
                {
                    pen.setWidth(properties.width);
                }
            }

            mScribbleArea->drawPolyline(tempPath, pen, true);
        }
    }
}

void PolylineTool::updateDirtyRect(QList<QPointF> linePoints, BlitRect dirtyRect)
{
    BlitRect blitRect;

    // In order to clear what was previously dirty, we need to include the previous buffer bound
    // this ensures that we won't see stroke artifacts
    blitRect.extend(mEditor->view()->mapCanvasToScreen(dirtyRect).toRect());

    BlitRect lineBounds;

    for (QPointF point : linePoints) {
        lineBounds.extend(point.toPoint());
    }
    QRect updateRect = mEditor->view()->mapCanvasToScreen(lineBounds).toRect();

    // Now extend with the new path bounds mapped to the local coordinate
    blitRect.extend(updateRect);

    // And update only the affected area
    mScribbleArea->update(blitRect.adjusted(-1, -1, 1, 1));
}

void PolylineTool::cancelPolyline()
{
    clearToolData();
}

void PolylineTool::endPolyline(QList<QPointF> points, quint64 timeStamp)
{
    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::VECTOR)
    {
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

        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
        if (vectorImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing
        vectorImage->addCurve(curve, mEditor->view()->scaling());
    }
    if (layer->type() == Layer::BITMAP)
    {
        drawPolyline(mPoints, mPoints.last(), calculateDeltaTime(timeStamp));
    }
    
    endStroke();
    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());

    clearToolData();
}
