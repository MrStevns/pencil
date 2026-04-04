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
                bitmapToolPressEvent(event, mBitmapTool);
                break;
            case Layer::VECTOR:
                vectorToolPressEvent(event, mVectorTool);
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

void MoveTool::vectorToolPressEvent(PointerEvent* event, VectorTool& tool)
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
            bitmapToolMoveEvent(event, mBitmapTool);
            break;
        case Layer::VECTOR:
            break;
        default:
            break;
    }

    mScribbleArea->updateFrame();
}

void MoveTool::pointerReleaseEvent(PointerEvent* event)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            bitmapToolReleaseEvent(event, mBitmapTool);
            break;
        default:
            break;
    }
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

void MoveTool::translateSelection(const PointerEvent* event, const DragState& dragState, SelectionEditor& selectionEditor)
{
    QPointF delta = event->canvasPos() - dragState.startPos;
    QPointF newPos = QPointF(dragState.dx, dragState.dy) + delta;
    selectionEditor.setTranslation(newPos);
}

void MoveTool::rotateSelection(const PointerEvent* event, const DragState& dragState, qreal previousRotation, SelectionEditor& selectionEditor)
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

void MoveTool::scaleAroundAnchorPoint(const PointerEvent* event, const DragState& dragState, const QPolygonF& selectionPolygon, SelectionEditor& selectionEditor)
{
    selectionEditor.scaleAroundAnchorPoint(dragState.dragHandle, selectionPolygon, event->canvasPos());
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
