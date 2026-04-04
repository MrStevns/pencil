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

#include <cassert>
#include <QMessageBox>
#include <QSettings>

#include "pointerevent.h"
#include "editor.h"
#include "toolmanager.h"
#include "strokeinterpolator.h"
#include "selectionmanager.h"
#include "overlaymanager.h"
#include "undoredomanager.h"
#include "scribblearea.h"
#include "layervector.h"
#include "layermanager.h"
#include "layercamera.h"
#include "mathutils.h"
#include "vectorimage.h"

MoveTool::MoveTool(QObject* parent) : TransformTool(parent)
{
}

ToolType MoveTool::type() const
{
    return MOVE;
}

void MoveTool::loadSettings()
{
    mRotationIncrementPref = mEditor->preference()->getInt(SETTING::ROTATION_INCREMENT);
    QSettings pencilSettings(PENCIL2D, PENCIL2D);

    mPropertyUsed[TransformToolProperties::SHOWSELECTIONINFO_ENABLED] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[TransformToolProperties::ANTI_ALIASING_ENABLED] = { Layer::BITMAP };
    QHash<int, PropertyInfo> info;

    info[TransformToolProperties::SHOWSELECTIONINFO_ENABLED] = false;
    info[TransformToolProperties::ANTI_ALIASING_ENABLED] = true;

    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    if (toolProperties().requireMigration(pencilSettings, ToolProperties::VERSION_1)) {
        toolProperties().setBaseValue(TransformToolProperties::SHOWSELECTIONINFO_ENABLED, pencilSettings.value("ShowSelectionInfo", false).toBool());
        toolProperties().setBaseValue(TransformToolProperties::ANTI_ALIASING_ENABLED, pencilSettings.value("moveAA", true).toBool());
    }

    connect(mEditor->preference(), &PreferenceManager::optionChanged, this, &MoveTool::updateSettings);
}

QCursor MoveTool::cursor()
{
    if (mCursorCacheInvalid) {
        if (mEditor->select()->somethingSelected()) {
            mCursorCache = cursorForDragHandle(mBitmapTool.dragState.dragHandle);
        } else if (mEditor->overlays()->anyOverlayEnabled()) {
            mCursorCache = cursorForPerspective(mPerspectiveTool.perspectiveMode);
        }
        mCursorCacheInvalid = false;
    }
    return mCursorCache;
}

void MoveTool::updateSettings(const SETTING setting)
{
    switch (setting)
    {
    case SETTING::ROTATION_INCREMENT:
        mRotationIncrementPref = mEditor->preference()->getInt(SETTING::ROTATION_INCREMENT);
        break;
    case SETTING::OVERLAY_PERSPECTIVE1:
        mEditor->overlays()->settingsUpdated(setting, mEditor->preference()->isOn(setting));
        break;
    case SETTING::OVERLAY_PERSPECTIVE2:
        mEditor->overlays()->settingsUpdated(setting, mEditor->preference()->isOn(setting));
        break;
    case SETTING::OVERLAY_PERSPECTIVE3:
        mEditor->overlays()->settingsUpdated(setting, mEditor->preference()->isOn(setting));
        break;
    default:
        break;
    }
}

MoveTool::InteractionMode MoveTool::resolveInteractionMode() const
{
    if (mEditor->select()->somethingSelected())
    {
        return InteractionMode::SELECTION;
    } else if (mEditor->overlays()->anyOverlayEnabled()) {
        return InteractionMode::PERSPECTIVE_OVERLAY;
    }

    return InteractionMode::NONE;
}

void MoveTool::pointerPressEvent(PointerEvent* event)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    mEditor->select()->setSmoothTransform(transformSettings().antiAliasingEnabled());

    if (mEditor->select()->somethingSelected())
    {
        switch (currentLayer->type())
        {
            case Layer::BITMAP:
                pressEventBitmapTool(event, mBitmapTool);
                break;
            case Layer::VECTOR:
                pressEventVectorTool(event, mVectorTool);
                break;
            case Layer::CAMERA:
                // pressEventCameraTool(event, mCameraTool);
                // TODO:
                break;
            default:
                break;
        }
    }
    else if (mEditor->overlays()->anyOverlayEnabled())
    {
        pressEventPerspectiveTool(event, mPerspectiveTool);
    }

    mEditor->updateFrame();
}

void MoveTool::pressEventBitmapTool(PointerEvent* event, BitmapTool& tool)
{
    auto selectionEditor = mEditor->select()->currentSelectionBitmapEditor();
    const QPointF& canvasPos = event->canvasPos();
    const Qt::KeyboardModifiers keyMod = event->modifiers();

    if (!selectionEditor->mySelectionRect().isNull())
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

    setDragStateBitmapTool(event, tool, *selectionEditor);
}

void MoveTool::setDragStateBitmapTool(PointerEvent* event, BitmapTool& tool, const SelectionBitmapEditor& selectionEditor)
{
    const qreal handleTolerance = mEditor->select()->selectionTolerance();
    const Qt::KeyboardModifiers keyMod = event->modifiers();

    const QPointF& canvasPos = event->canvasPos();
    tool.dragState.dragHandle = selectionEditor.resolveHandleMode(event->canvasPos(), handleTolerance);
    tool.dragState.startPos = event->canvasPos();
    tool.dragState.dx = selectionEditor.myTranslation().x();
    tool.dragState.dy = selectionEditor.myTranslation().y();

    if (tool.dragState.dragHandle == DragHandle::NONE) {
        return;
    }

    if (tool.dragState.dragHandle == DragHandle::CENTER) {

        if (keyMod == Qt::ControlModifier) {
            tool.rotatedAngle = selectionEditor.angleFromPoint(canvasPos, selectionEditor.currentAnchorPoint()) - selectionEditor.myRotation();
            tool.dragState.transformMode = TransformMode::ROTATE;
        }
        else
        {
            tool.dragState.transformMode = TransformMode::TRANSLATE;
        }
    } else {
        tool.dragState.transformMode = TransformMode::SCALE;
    }
}

void MoveTool::pressEventVectorTool(PointerEvent* event, VectorTool& tool)
{

}

void MoveTool::pressEventPerspectiveTool(PointerEvent* event, PerspectiveOverlayTool& tool)
{
    LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
    Q_ASSERT(layerCam);

    tool.perspectiveMode = mEditor->overlays()->getMoveModeForPoint(event->canvasPos(), layerCam->getViewAtFrame(mEditor->currentFrame()));
    mEditor->overlays()->setPerspectiveMode(tool.perspectiveMode);
    QPoint mapped = layerCam->getViewAtFrame(mEditor->currentFrame()).map(event->canvasPos()).toPoint();
    mEditor->overlays()->updatePerspective(mapped);
}

void MoveTool::pointerMoveEvent(PointerEvent* event)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            moveEventBitmapTool(event, mBitmapTool);
            break;
        case Layer::VECTOR:
            break;
        default:
            break;
    }

    mScribbleArea->updateFrame();
}

void MoveTool::moveEventBitmapTool(PointerEvent *event, BitmapTool& tool)
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
            transformSelection(event, tool);
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

void MoveTool::pointerReleaseEvent(PointerEvent* event)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            releaseEventBitmapTool(event, mBitmapTool);
            break;
        default:
            break;
    }
}

void MoveTool::releaseEventBitmapTool(PointerEvent*, BitmapTool& tool)
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

    mScribbleArea->updateToolCursor();
    emit mEditor->frameModified(mEditor->currentFrame());
}

void MoveTool::beginInteraction(const QPointF& pos, Qt::KeyboardModifiers keyMod, Layer* layer)
{
    // auto selectMan = mEditor->select();
    // QRectF selectionRect = selectMan->mySelectionRect();
    // if (!selectionRect.isNull())
    // {
    //     mUndoSaveState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);
    //     mEditor->backup(typeName());
    // }

    // if (keyMod != Qt::ShiftModifier)
    // {
    //     if (selectMan->isOutsideSelectionArea(pos))
    //     {
    //         applyTransformation();
    //         mEditor->deselectAll();
    //     }
    // }

    // if (selectMan->getMoveMode() == PerspectiveMode::MIDDLE)
    // {
    //     if (keyMod == Qt::ControlModifier) // --- rotation
    //     {
    //         selectMan->setMoveMode(PerspectiveMode::ROTATION);
    //     }
    // }

    // if (layer->type() == Layer::VECTOR)
    // {
    //     createVectorSelection(pos, keyMod, layer);
    // }

    // selectMan->setTransformAnchor(selectMan->getSelectionAnchorPoint());
    // // mOffset = selectMan->myTranslation();

    // if(selectMan->getMoveMode() == PerspectiveMode::ROTATION) {
    //     mRotatedAngle = selectMan->angleFromPoint(pos, selectMan->currentTransformAnchor()) - selectMan->myRotation();
    // }
}

void MoveTool::transformSelection(PointerEvent* event, BitmapTool& tool)
{
    auto selectionEditor = mEditor->select()->currentSelectionBitmapEditor();
    if (selectionEditor->somethingSelected())
    {
        const Qt::KeyboardModifiers keyMod = event->modifiers();

        selectionEditor->maintainAspectRatio(keyMod == Qt::ShiftModifier);
        selectionEditor->lockMovementToAxis(keyMod == Qt::ShiftModifier);

        switch (tool.dragState.transformMode)
        {
        case TransformMode::TRANSLATE: {
            translateSelection(event, tool.dragState, selectionEditor->transformEditor());
            break;
        }
        case TransformMode::SCALE: {
            selectionEditor->scaleAroundAnchorPoint(tool.dragState.dragHandle, event->canvasPos());
            break;
        }
        case TransformMode::ROTATE: {
            rotateSelection(event, tool.dragState, tool.rotatedAngle, selectionEditor->transformEditor());
            break;
        }
        default:
            break;
        }
        selectionEditor->calculateSelectionTransformation();
    }
}

void MoveTool::translateSelection(PointerEvent* event, const DragState& dragState, SelectionEditor& selectionEditor)
{
    QPointF delta = event->canvasPos() - dragState.startPos;
    QPointF newPos = QPointF(dragState.dx, dragState.dy) + delta;
    selectionEditor.setTranslation(newPos);
}

void MoveTool::rotateSelection(PointerEvent* event, const DragState& dragState, qreal previousRotation, SelectionEditor& selectionEditor)
{
    if (dragState.transformMode != TransformMode::ROTATE) {
        return;
    }

    int rotationIncrement = 0;
    if (event->modifiers() & Qt::ShiftModifier)
    {
        rotationIncrement = mRotationIncrementPref;
    }

    QPointF anchorPoint = selectionEditor.currentAnchorPoint();
    qreal newAngle = selectionEditor.angleFromPoint(event->canvasPos(), anchorPoint) - previousRotation;

    selectionEditor.rotate(newAngle, rotationIncrement);
}

/**
 * @brief MoveTool::createVectorSelection
 * In vector the selection rectangle is based on the bounding box of the curves
 * We can therefore create a selection just by clicking near/on a curve
 */
void MoveTool::createVectorSelection(const QPointF& pos, Qt::KeyboardModifiers keyMod, Layer* layer)
{
    assert(layer->type() == Layer::VECTOR);
    LayerVector* vecLayer = static_cast<LayerVector*>(layer);
    VectorImage* vectorImage = vecLayer->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (vectorImage == nullptr) { return; }

    if (!mEditor->select()->closestCurves().empty()) // the user clicks near a curve
    {
        setCurveSelected(vectorImage, keyMod);
    }
    else if (vectorImage->getLastAreaNumber(pos) > -1)
    {
        setAreaSelected(pos, vectorImage, keyMod);
    }
}

void MoveTool::setCurveSelected(VectorImage* vectorImage, Qt::KeyboardModifiers keyMod)
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

void MoveTool::setAreaSelected(const QPointF& pos, VectorImage* vectorImage, Qt::KeyboardModifiers keyMod)
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
 * @brief MoveTool::storeClosestVectorCurve
 * stores the curves closest to the mouse position in mClosestCurves
 */
void MoveTool::storeClosestVectorCurve(const QPointF& pos, Layer* layer)
{
    auto selectMan = mEditor->select();
    auto layerVector = static_cast<LayerVector*>(layer);
    VectorImage* pVecImg = layerVector->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (pVecImg == nullptr) { return; }
    selectMan->setCurves(pVecImg->getCurvesCloseTo(pos, selectMan->selectionTolerance()));
}

void MoveTool::applyTransformation()
{
    SelectionManager* selectMan = mEditor->select();
    mScribbleArea->applyTransformedSelection();

    // When the selection has been applied, a new rect is applied based on the bounding box.
    // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
    if (selectMan->somethingSelected()) {
        selectMan->setSelection(selectMan->mapToSelection(QPolygonF(selectMan->mySelectionRect())).boundingRect());
    }
    // mRotatedAngle = 0;
}

bool MoveTool::leavingThisTool()
{
    TransformTool::leavingThisTool();

    if (currentPaintableLayer())
    {
        applyTransformation();
    }

    return true;
}

bool MoveTool::isActive() const {
    return mScribbleArea->isPointerInUse() &&
           (mEditor->select()->somethingSelected() || mEditor->overlays()->getPerspectiveMode() != PerspectiveMode::NONE);
}

Layer* MoveTool::currentPaintableLayer()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr)
        return nullptr;
    if (!layer->isPaintable())
        return nullptr;
    return layer;
}

QCursor MoveTool::cursorForDragHandle(DragHandle handle) const
{
    QPixmap cursorPixmap = QPixmap(24, 24);

    cursorPixmap.fill(QColor(255, 255, 255, 0));
    QPainter cursorPainter(&cursorPixmap);
    cursorPainter.setRenderHint(QPainter::Antialiasing);

    switch(handle)
    {
    case DragHandle::TOP_LEFT:
    case DragHandle::BOTTOM_RIGHT:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-diagonal-left.svg"));
        break;
    }
    case DragHandle::TOP_RIGHT:
    case DragHandle::BOTTOM_LEFT:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-diagonal-right.svg"));
        break;
    }
    case DragHandle::CENTER:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-move.svg"));
        break;
    }
    default:
        return Qt::ArrowCursor;
    }
    cursorPainter.end();

    return QCursor(cursorPixmap);
}

QCursor MoveTool::cursorForPerspective(PerspectiveMode mode) const
{
    QPixmap cursorPixmap = QPixmap(24, 24);

    cursorPixmap.fill(QColor(255, 255, 255, 0));
    QPainter cursorPainter(&cursorPixmap);
    cursorPainter.setRenderHint(QPainter::Antialiasing);

    switch(mode)
    {
    case PerspectiveMode::PERSP_LEFT:
    case PerspectiveMode::PERSP_RIGHT:
    case PerspectiveMode::PERSP_MIDDLE:
    case PerspectiveMode::PERSP_SINGLE:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-move.svg"));
        break;
    }
    default:
        return Qt::ArrowCursor;
    }
    cursorPainter.end();

    return QCursor(cursorPixmap);
}
