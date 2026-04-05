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

#include "selecttool.h"

#include "editor.h"
#include "pointerevent.h"

#include "layer.h"
#include "layervector.h"
#include "vectorimage.h"

#include "scribblearea.h"

#include "layermanager.h"
#include "selectionmanager.h"

void SelectTool::vectorToolPressEvent(PointerEvent* event, VectorTool& tool)
{
    auto selectMan = mEditor->select();
    const QPointF canvasPos = event->canvasPos();

    tool.undoState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);

    if (selectMan->somethingSelected() && tool.selectionSet) // there is something selected
    {
        Layer* currentLayer = mEditor->layers()->currentLayer();
        VectorImage* vectorImage = static_cast<LayerVector*>(currentLayer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
        if (vectorImage != nullptr) {
            vectorImage->deselectAll();
        }
        tool.selectionRect = mEditor->select()->mapToSelection(mEditor->select()->mySelectionRect()).boundingRect();
        tool.dragState.dragHandle = mEditor->select()->resolveHandleMode(canvasPos, selectMan->selectionTolerance());
    }
    else
    {
        tool.dragState.dragHandle = DragHandle::TOP_LEFT;
        tool.anchorOriginPoint = canvasPos;
        selectMan->setSelection(QRectF(canvasPos.x(), canvasPos.y(), 0, 0));
    }

    tool.dragState.dragFromPoint = canvasPos;

    mScribbleArea->updateFrame();
    mScribbleArea->updateToolCursor();
}

void SelectTool::vectorToolMoveEvent(PointerEvent* event, VectorTool& tool)
{
    QPointF canvasPos = event->canvasPos();
    auto selectMan = mEditor->select();

    if (mScribbleArea->isPointerInUse())
    {
        // QRectF
        if (!tool.selectionSet) {
            // When there's no existing selection, create one based on the anchor
            tool.selectionRect = QRectF(canvasPos, tool.dragState.anchorOriginPoint);
            selectMan->setSelection(tool.selectionRect);
        }

        // Layer* currentLayer = mEditor->layers()->currentLayer();
        // VectorImage* vectorImage = static_cast<LayerVector*>(currentLayer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
        // if (vectorImage != nullptr) {
        //     vectorImage->select(selectMan->mapToSelection(QPolygonF(selectMan->mySelectionRect())).boundingRect());
        // }
    } else {
        tool.dragState.dragHandle = selectMan->resolveHandleMode(canvasPos, selectMan->selectionTolerance());
    }

    mScribbleArea->updateToolCursor();
    mScribbleArea->updateFrame();
}

void SelectTool::vectorToolReleaseEvent(PointerEvent *event, VectorTool &tool)
{
    tool.dragState = DragState();

    QPointF canvasPos = event->canvasPos();

    // if there's a small very small distance between current and last point
    // discard the selection...
    // TODO: improve by adding a timer to check if the user is deliberately selecting
    if (QLineF(tool.anchorOriginPoint, canvasPos).length() < 1.0)
    {
        tool.selectionSet = false;
        mEditor->deselectAll();
    }
    else if (mEditor->select()->isOutsideSelectionArea(canvasPos, mEditor->select()->selectionTolerance()))
    {
        tool.selectionSet = false;
        mEditor->deselectAll();
    }
    else
    {
        tool.selectionSet = true;
        tool.selectionRect = mEditor->select()->mapToSelection(mEditor->select()->mySelectionRect()).boundingRect();

        vectorToolSetSelection();
    }

    mEditor->undoRedo()->record(tool.undoState, typeName());

    mScribbleArea->updateToolCursor();
    mScribbleArea->updateFrame();
}

/**
 * @brief SelectTool::vectorToolSetSelection
 * Keep selection rect and normalize if invalid
 */
void SelectTool::vectorToolSetSelection()
{
    Layer* currentLayer = mEditor->layers()->currentLayer();
    if (currentLayer == nullptr) { return; }

    VectorImage* vectorImage = static_cast<LayerVector*>(currentLayer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (vectorImage == nullptr) { return; }
    auto selectMan = mEditor->select();
    selectMan->setSelection(vectorImage->getSelectionRect());
}
