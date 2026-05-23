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
            Layer* layer = currentPaintableLayer();
            if (layer == nullptr) { return Qt::ArrowCursor; }

            switch (layer->type())
            {
                case Layer::BITMAP:
                    mCursorCache = createCursorForDragHandle(mBitmapTool.dragState.dragHandle);
                    break;
                case Layer::VECTOR:
                    mCursorCache = createCursorForDragHandle(mVectorTool.dragState.dragHandle);
                    break;
                default:
                    mCursorCache = Qt::ArrowCursor;
                    break;
            }

        } else if (mEditor->overlays()->anyOverlayEnabled()) {
            mCursorCache = perspectiveToolCreateCursor(mPerspectiveTool.perspectiveMode);
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

    if (mEditor->select()->somethingSelected())
    {
        mEditor->select()->setSmoothTransform(transformSettings().antiAliasingEnabled());
        switch (currentLayer->type())
        {
            case Layer::BITMAP:
                bitmapToolPressEvent(event, mBitmapTool);
                break;
            case Layer::VECTOR:
                vectorToolPressEvent(event, mVectorTool);
                break;
            default:
                break;
        }
    }
    else if (mEditor->overlays()->anyOverlayEnabled())
    {
        perspectiveToolPressEvent(event, mPerspectiveTool);
    }

    mEditor->updateFrame();
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

 void MoveTool::setTransformMode(const PointerEvent* event, const DragHandle& dragHandle, const SelectionTransformEditor& selectionEditor, TransformState& transformState)
{
    const Qt::KeyboardModifiers keyMod = event->modifiers();
    const QPointF& canvasPos = event->canvasPos();
    if (dragHandle == DragHandle::NONE) {
        return;
    }

    if (dragHandle == DragHandle::CENTER) {

        if (keyMod == Qt::ControlModifier) {
            transformState.rotatedAngle = selectionEditor.angleFromPoint(canvasPos, selectionEditor.currentAnchorPoint()) - selectionEditor.rotationAngle();
            transformState.transformMode = TransformMode::ROTATE;
        }
        else
        {
            transformState.transformMode = TransformMode::TRANSLATE;
        }
    } else {
        transformState.transformMode = TransformMode::SCALE;
    }
}

void MoveTool::translateSelection(const PointerEvent* event, const DragState& dragState, SelectionTransformEditor& selectionEditor)
{
    QPointF delta = (event->canvasPos() - dragState.startPos).toPoint();
    QPointF newPos = QPointF(dragState.dx, dragState.dy) + delta;

    selectionEditor.setTranslation(newPos);
}

void MoveTool::rotateSelection(const PointerEvent* event, const TransformState& transformState, SelectionTransformEditor& selectionEditor)
{
    if (transformState.transformMode != TransformMode::ROTATE) {
        return;
    }

    int rotationIncrement = 0;
    if (event->modifiers() & Qt::ShiftModifier)
    {
        rotationIncrement = mRotationIncrementPref;
    }

    QPointF anchorPoint = selectionEditor.currentAnchorPoint();
    qreal newAngle = selectionEditor.angleFromPoint(event->canvasPos(), anchorPoint) - transformState.rotatedAngle;

    selectionEditor.rotate(newAngle, rotationIncrement);
}

void MoveTool::scaleAroundAnchorPoint(const QPointF& canvasPoint, const DragState& dragState, const QPolygonF& selectionPolygon, SelectionTransformEditor& selectionEditor)
{
    selectionEditor.scaleAroundAnchorPoint(dragState.dragHandle, selectionPolygon, canvasPoint);
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
}

bool MoveTool::leavingThisTool()
{
    TransformTool::leavingThisTool();

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

QCursor MoveTool::createCursorForDragHandle(DragHandle handle) const
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
