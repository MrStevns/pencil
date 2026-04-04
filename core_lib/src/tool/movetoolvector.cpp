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

#include "layervector.h"
#include "editor.h"
#include "vectorimage.h"
#include "selectionmanager.h"
#include "layermanager.h"

#include "pointerevent.h"

void MoveTool::vectorToolPressEvent(PointerEvent* event, VectorTool& tool)
{
    auto selectionEditor = mEditor->select()->currentSelectionBitmapEditor();
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

    vectorToolCreateSelection(canvasPos, keyMod);

    // vectorToolSetDragState(event, *selectionEditor, tool);
    setTransformMode(event, tool.dragState.dragHandle, selectionEditor->transformEditor(), tool.transformState);
}

// void MoveTool::bitmapToolSetDragState(PointerEvent* event, const SelectionVectorEditor& selectionEditor, VectorTool& tool)
// {
//     const qreal handleTolerance = mEditor->select()->selectionTolerance();

//     tool.dragState.dragHandle = selectionEditor.resolveHandleMode(event->canvasPos(), handleTolerance);
//     tool.dragState.startPos = event->canvasPos();
//     tool.dragState.dx = selectionEditor.myTranslation().x();
//     tool.dragState.dy = selectionEditor.myTranslation().y();
// }

/**
 * @brief MoveTool::vectorToolCreateSelection
 * In vector the selection rectangle is based on the bounding box of the curves
 * We can therefore create a selection just by clicking near/on a curve
 */
void MoveTool::vectorToolCreateSelection(const QPointF& pos, Qt::KeyboardModifiers keyMod)
{
    auto layer = mEditor->layers()->currentLayer();
    assert(layer->type() == Layer::VECTOR);
    LayerVector* vecLayer = static_cast<LayerVector*>(layer);
    VectorImage* vectorImage = vecLayer->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (vectorImage == nullptr) { return; }

    if (!mEditor->select()->closestCurves().empty()) // the user clicks near a curve
    {
        vectorToolSetCurveSelected(vectorImage, keyMod);
    }
    else if (vectorImage->getLastAreaNumber(pos) > -1)
    {
        vectorToolSetAreaSelected(pos, vectorImage, keyMod);
    }
}

void MoveTool::vectorToolSetCurveSelected(VectorImage* vectorImage, Qt::KeyboardModifiers keyMod)
{
    auto selectMan = mEditor->select();
    if (!vectorImage->isSelected(selectMan->closestCurves()))
    {
        if (keyMod != Qt::ShiftModifier)
        {
            applyTransformation();
        }
        vectorImage->setSelected(selectMan->closestCurves(), true);
        selectMan->setSelection(vectorImage->getSelectionRect());
    }
}

void MoveTool::vectorToolSetAreaSelected(const QPointF& pos, VectorImage* vectorImage, Qt::KeyboardModifiers keyMod)
{
    int areaNumber = vectorImage->getLastAreaNumber(pos);
    if (!vectorImage->isAreaSelected(areaNumber))
    {
        if (keyMod != Qt::ShiftModifier)
        {
            applyTransformation();
        }
        vectorImage->setAreaSelected(areaNumber, true);
        mEditor->select()->setSelection(vectorImage->getSelectionRect());
    }
}

/**
 * @brief MoveTool::vectorToolStoreClosestCurve
 * stores the curves closest to the mouse position in mClosestCurves
 */
void MoveTool::vectorToolStoreClosestCurve(const QPointF& pos, Layer* layer)
{
    auto selectMan = mEditor->select();
    auto layerVector = static_cast<LayerVector*>(layer);
    VectorImage* pVecImg = layerVector->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (pVecImg == nullptr) { return; }
    selectMan->setCurves(pVecImg->getCurvesCloseTo(pos, selectMan->selectionTolerance()));
}
