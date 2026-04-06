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
#include "viewmanager.h"
#include "selectionmanager.h"

void SelectTool::vectorToolPressEvent(PointerEvent* event, VectorTool& tool)
{
    auto selectMan = mEditor->select();
    const QPointF canvasPos = event->canvasPos();

    tool.undoState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);

    if (selectMan->somethingSelected() && tool.selectionSet) // there is something selected
    {
        tool.dragState.selectionRect = mEditor->select()->mapToSelection(mEditor->select()->mySelectionRect()).boundingRect();
        tool.dragState.dragHandle = mEditor->select()->resolveHandleMode(canvasPos, selectMan->selectionTolerance());
    }
    else
    {
        tool.dragState.dragHandle = DragHandle::TOP_LEFT;
        tool.selectionSet = false;
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
        if (!tool.selectionSet || tool.dragState.dragHandle == DragHandle::NONE) {
            // When there's no existing selection, create one based on the anchor
            tool.dragState.selectionRect = QRectF(canvasPos, tool.dragState.dragFromPoint);
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
    QPointF canvasPos = event->canvasPos();

    Layer* currentLayer = mEditor->layers()->currentLayer();
    if (currentLayer == nullptr) { return; }
    VectorImage *vectorImage = static_cast<VectorImage*>(currentLayer->getLastKeyFrameAtPosition(mEditor->currentFrame()));
    if (vectorImage == nullptr) { return; }

    if (mEditor->select()->somethingSelected() && !vectorImage->intersects(tool.dragState.selectionRect))
    {
        tool.selectionSet = false;
        mEditor->select()->resetSelectionProperties();
    }
    else
    {
        tool.selectionSet = true;
        mEditor->select()->setSelection(tool.dragState.selectionRect);
    }
    tool.dragState = DragState();

    mEditor->undoRedo()->record(tool.undoState, typeName());

    mScribbleArea->updateToolCursor();
    mScribbleArea->updateFrame();
}

void SelectTool::vectorToolPaintEvent(QPainter& painter, const QRect, const VectorTool& tool)
{
    if (tool.dragState.selectionRect.isNull()) { return; }

    Object* object = mEditor->object();
    auto selectMan = mEditor->select();

    TransformParameters params = { tool.dragState.selectionRect, tool.dragState.selectionRect.center(), mEditor->view()->getView(), selectMan->selectionTransform() };

    mSelectionPainter.paint(painter,
                            object,
                            mEditor->currentLayerIndex(),
                            transformSettings(),
                            params);
}
