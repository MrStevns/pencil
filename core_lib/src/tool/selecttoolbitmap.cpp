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

#include "selectionmanager.h"
#include "undoredomanager.h"

#include "scribblearea.h"

void SelectTool::bitmapToolPressEvent(PointerEvent* event, BitmapTool& tool)
{
    auto selectMan = mEditor->select();
    const QPointF canvasPos = event->canvasPos();

    tool.undoState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);

    if (selectMan->somethingSelected() && tool.selectionSet)
    {
        // there is something selected
        tool.selectionRect = mEditor->select()->mapToSelection(mEditor->select()->mySelectionRect()).boundingRect();
        tool.dragState.dragHandle = mEditor->select()->resolveHandleMode(canvasPos, selectMan->selectionTolerance());
    }
    else
    {
        tool.selectionRect = QRectF();
        tool.dragState.dragHandle = DragHandle::NONE;
        tool.dragState.anchorOriginPoint = canvasPos;
        tool.selectionSet = false;
    }

    tool.dragState.dragFromPoint = canvasPos;

    mScribbleArea->updateFrame();
    mScribbleArea->updateToolCursor();
}

void SelectTool::bitmapToolMoveEvent(PointerEvent* event, BitmapTool& tool)
{
    QPointF canvasPos = event->canvasPos();
    auto selectMan = mEditor->select();

    if (mScribbleArea->isPointerInUse())
    {
        QRectF newSelection;
        if (!tool.selectionSet) {
            // When there's no existing selection, create one based on the anchor
            newSelection = QRectF(canvasPos, tool.dragState.anchorOriginPoint);
        } else {
            newSelection = bitmapToolDragSelection(tool.selectionRect, canvasPos, tool.dragState);
        }
        newSelection = newSelection.normalized();
        selectMan->setSelection(newSelection);

    } else
    {
        tool.dragState.dragHandle = selectMan->resolveHandleMode(canvasPos, selectMan->selectionTolerance());
    }

    mScribbleArea->updateToolCursor();
    mScribbleArea->updateFrame();
}


void SelectTool::bitmapToolReleaseEvent(PointerEvent*, BitmapTool& tool) const
{
    QRectF selectionRect = mEditor->select()->mapToSelection(mEditor->select()->mySelectionRect()).boundingRect();;

    if (selectionRect.toRect().isEmpty())
    {
        // If there's less than a pixel between the anchor and current point
        // discard the selection.
        mEditor->select()->resetSelectionProperties();
        tool.selectionSet = false;
    }
    else if (tool.selectionSet && tool.dragState.dragHandle == DragHandle::NONE)
    {
        // The user clicked outside the selection, so discard it
        mEditor->select()->resetSelectionProperties();
        tool.selectionSet = false;
    }
    else
    {
        tool.selectionSet = true;
        tool.selectionRect = selectionRect;
    }
    tool.dragState = DragState();

    mEditor->undoRedo()->record(tool.undoState, typeName());

    mScribbleArea->updateToolCursor();
    mScribbleArea->updateFrame();
}

QRectF SelectTool::bitmapToolDragSelection(const QRectF& selection, const QPointF& currentPoint, const DragState& dragState) const
{
    QRectF newSelection = selection;
    DragHandle dragHandle = dragState.dragHandle;

    QPointF offset = currentPoint - dragState.dragFromPoint;
    if (dragHandle == DragHandle::TOP_LEFT) {
        newSelection.adjust(offset.x(), offset.y(), 0, 0);
    } else if (dragHandle == DragHandle::TOP_RIGHT) {
        newSelection.adjust(0, offset.y(), offset.x(), 0);
    } else if (dragHandle == DragHandle::BOTTOM_RIGHT) {
        newSelection.adjust(0, 0, offset.x(), offset.y());
    } else if (dragHandle == DragHandle::BOTTOM_LEFT) {
        newSelection.adjust(offset.x(), 0, 0, offset.y());
    } else if (dragHandle == DragHandle::CENTER) {
        newSelection.translate(offset.x(), offset.y());
    }

    return newSelection;
}
