/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang
Copyright (C) 2025-2099 Oliver S. larsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "nodetool.h"

#include <QPixmap>
#include <QSettings>

#include "pointerevent.h"
#include "vectorimage.h"
#include "editor.h"
#include "scribblearea.h"

#include "layermanager.h"
#include "viewmanager.h"
#include "selectionmanager.h"
#include "undoredomanager.h"

#include "layervector.h"
#include "blitrect.h"

NodeTool::NodeTool(QObject* parent) : BaseTool(parent)
{
}

ToolType NodeTool::type() const
{
    return NODE;
}

QCursor NodeTool::cursor()
{
    return Qt::ArrowCursor;
}

void NodeTool::loadSettings()
{
}

void NodeTool::pointerPressEvent(PointerEvent* event)
{
    Layer* layer = mEditor->layers()->currentLayer();
    auto selectMan = mEditor->select();
    if (layer == nullptr) { return; }

    if (layer->type() != Layer::VECTOR) { return; }

    if (event->button() == Qt::LeftButton)
    {
        mPressPoint = event->canvasPos();
        const int currentFrame = mEditor->currentFrame();
        const float distanceFrom = selectMan->selectionTolerance();
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(currentFrame, 0);
        if (vectorImage == nullptr) { return; }
        selectMan->setCurves(vectorImage->getCurvesCloseTo(mPressPoint, distanceFrom));
        selectMan->setVertices(vectorImage->getVerticesCloseTo(mPressPoint, distanceFrom));
        if (selectMan->closestCurves().size() > 0 || selectMan->closestCurves().size() > 0)      // the user clicks near a vertex or a curve
        {
            if (event->modifiers() != Qt::ShiftModifier && !vectorImage->isSelected(selectMan->closestVertices()))
            {
                mEditor->deselectAll();
            }

            vectorImage->setSelected(selectMan->closestVertices(), true);
            selectMan->vectorSelection.add(selectMan->closestCurves());
            selectMan->vectorSelection.add(selectMan->closestVertices());

            emit mEditor->frameModified(mEditor->currentFrame());
        }
        else
        {
            mEditor->deselectAll();
        }
    }

    BaseTool::pointerPressEvent(event);
}

void NodeTool::pointerMoveEvent(PointerEvent* event)
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (layer->type() != Layer::VECTOR)
    {
        return;
    }

    auto selectMan = mEditor->select();
    if (event->buttons() & Qt::LeftButton)   // the user is also pressing the mouse (dragging) {
    {
        if (event->modifiers() != Qt::ShiftModifier)    // (and the user doesn't press shift)
        {
            VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
            if (vectorImage == nullptr) { return; }
            // transforms the selection

            BlitRect blit;

            // Use the previous dirty bound and extend it with the current dirty bound
            // this ensures that we won't get painting artifacts
            blit.extend(vectorImage->getBoundsOfTransformedCurves().toRect());
            QPointF offsetFromPressPosition = offsetFromPressPos(event->canvasPos());
            selectMan->setSelectionTransform(QTransform().translate(offsetFromPressPosition.x(), offsetFromPressPosition.y()));
            vectorImage->setSelectionTransformation(selectMan->selectionTransform());
            blit.extend(vectorImage->getBoundsOfTransformedCurves().toRect());

            // And now tell the widget to update the portion in local coordinates
            mScribbleArea->update(mEditor->view()->mapCanvasToScreen(blit).toRect().adjusted(-1, -1, 1, 1));
        }
    }
    else     // the user is moving the mouse without pressing it
    {
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
        if (vectorImage == nullptr) { return; }

        selectMan->setVertices(vectorImage->getVerticesCloseTo(event->canvasPos(), selectMan->selectionTolerance()));
        mScribbleArea->update();
    }

    BaseTool::pointerMoveEvent(event);
}

void NodeTool::pointerReleaseEvent(PointerEvent *)
{
    Layer* currentLayer = mEditor->layers()->currentLayer();
    if (currentLayer->type() == Layer::VECTOR) {
        VectorImage* vectorImage = static_cast<LayerVector*>(currentLayer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
        if (vectorImage) {
            applyVectorBuffer(vectorImage);
        }
    }
    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}

void NodeTool::applyVectorBuffer(VectorImage *vectorImage)
{
    vectorImage->applySelectionTransformation();

    auto selectMan = mEditor->select();
    selectMan->resetSelectionTransform();
    for (int k = 0; k < selectMan->vectorSelection.curve.size(); k++)
    {
        int curveNumber = selectMan->vectorSelection.curve.at(k);
        vectorImage->curve(curveNumber).smoothCurve();
    }
}

QPointF NodeTool::offsetFromPressPos(const QPointF& point) const
{
    return point - mPressPoint;
}

void NodeTool::paint(QPainter &painter, const QRect &blitRect)
{
    painter.drawRect(blitRect);
}

