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
#include "movetool.h"

#include "editor.h"
#include "selectionbitmapeditor.h"

#include "pointerevent.h"

#include "selectionmanager.h"
#include "scribblearea.h"
#include "layermanager.h"
#include "overlaymanager.h"

#include "layercamera.h"

void MoveTool::bitmapToolPressEvent(PointerEvent* event, BitmapTool& tool)
{
    auto selectionEditor = mEditor->select()->activeBitmapEditor();
    const QPointF& canvasPos = event->canvasPos();
    const Qt::KeyboardModifiers keyMod = event->modifiers();

    if (!selectionEditor->selectionRect().isNull())
    {
        tool.undoSaveState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);
        mEditor->backup(typeName());
    }

    const qreal handleTolerance = mEditor->select()->selectionTolerance();

    if (keyMod != Qt::ShiftModifier)
    {
        if (selectionEditor->isOutsideSelectionArea(canvasPos, handleTolerance))
        {
            applyTransformation();
            mEditor->deselectAll();
        }
    }

    selectionEditor->setTransformAnchor(selectionEditor->resolveAnchorPoint(canvasPos, handleTolerance));

    bitmapToolSetDragState(event, *selectionEditor, tool);
    setTransformMode(event, tool.dragState.dragHandle, selectionEditor->transformEditor(), tool.transformState);
}

void MoveTool::bitmapToolMoveEvent(PointerEvent *event, BitmapTool& tool)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    auto selectMan = mEditor->select();
    if (mScribbleArea->isPointerInUse())   // the user is also pressing the mouse (dragging)
    {
        if (mEditor->overlays()->anyOverlayEnabled())
        {
            LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
            Q_ASSERT(layerCam);
            mEditor->overlays()->updatePerspective(layerCam->getViewAtFrame(mEditor->currentFrame()).map(event->canvasPos()));
        }
        if (selectMan->somethingSelected())
        {
            bitmapToolTransformSelection(event, tool);
        }
    }
    else
    {
        // update cursor to reflect selection corner interaction
        tool.dragState.dragHandle = selectMan->resolveHandleMode(event->canvasPos(), selectMan->selectionTolerance());

        if (mEditor->overlays()->anyOverlayEnabled())
        {
            LayerCamera *layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
            Q_ASSERT(layerCam);
            mPerspectiveTool.perspectiveMode = mEditor->overlays()->getMoveModeForPoint(event->canvasPos(), layerCam->getViewAtFrame(mEditor->currentFrame()));
        }

        if (selectMan->somethingSelected() || mEditor->overlays()->anyOverlayEnabled()) {
            mCursorCacheInvalid = true;
            mScribbleArea->updateToolCursor();
        }
    }
}

void MoveTool::bitmapToolReleaseEvent(PointerEvent*, BitmapTool& tool)
{
    mEditor->undoRedo()->record(tool.undoSaveState, typeName());

    tool.dragState = DragState();

    if (mEditor->overlays()->anyOverlayEnabled())
    {
        mEditor->overlays()->setPerspectiveMode(PerspectiveMode::NONE);
        mPerspectiveTool.perspectiveMode = PerspectiveMode::NONE;
    }

    auto selectMan = mEditor->select();
    if (!selectMan->somethingSelected())
        return;

    mCursorCacheInvalid = true;
    mScribbleArea->updateToolCursor();
    emit mEditor->frameModified(mEditor->currentFrame());
}

void MoveTool::bitmapToolSetDragState(PointerEvent* event, const SelectionBitmapEditor& selectionEditor, BitmapTool& tool)
{
    const qreal handleTolerance = mEditor->select()->selectionTolerance();

    tool.dragState.dragHandle = selectionEditor.resolveHandleMode(event->canvasPos(), handleTolerance);
    tool.dragState.startPos = event->canvasPos();
    tool.dragState.dx = selectionEditor.translation().x();
    tool.dragState.dy = selectionEditor.translation().y();
}

void MoveTool::bitmapToolTransformSelection(const PointerEvent* event, const BitmapTool& tool)
{
    auto selectionEditor = mEditor->select()->activeBitmapEditor();
    if (selectionEditor->somethingSelected())
    {
        const Qt::KeyboardModifiers keyMod = event->modifiers();

        selectionEditor->maintainAspectRatio(keyMod == Qt::ShiftModifier);
        selectionEditor->lockMovementToAxis(keyMod == Qt::ShiftModifier);

        switch (tool.transformState.transformMode)
        {
        case TransformMode::TRANSLATE: {
            translateSelection(event, tool.dragState, selectionEditor->editTransformEditor());
            break;
        }
        case TransformMode::SCALE: {
            scaleAroundAnchorPoint(event, tool.dragState, selectionEditor->selectionPolygon(), selectionEditor->editTransformEditor());
            break;
        }
        case TransformMode::ROTATE: {
            rotateSelection(event, tool.transformState, selectionEditor->editTransformEditor());
            break;
        }
        default:
            break;
        }
        selectionEditor->calculateSelectionTransformation();
    }
}
