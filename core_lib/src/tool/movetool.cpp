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
#include "viewmanager.h"
#include "strokemanager.h"
#include "selectionmanager.h"
#include "overlaymanager.h"
#include "clipboardmanager.h"
#include "scribblearea.h"
#include "layervector.h"
#include "layermanager.h"
#include "layercamera.h"
#include "mathutils.h"
#include "vectorimage.h"
#include "layerbitmap.h"

// TODO list:
// - Re-implement vector move/commit/discard behavior
// - Figure out how to do clipboard handling
// - Figure out how to access Scribblearea::handleDrawingOnEmptyFrame, should we move it into StrokeTool?
// - Move ScribbleArea::selectionForKeyEvents into MoveTool and Select tool
MoveTool::MoveTool(QObject* parent) : BaseTool(parent)
{
}

ToolType MoveTool::type()
{
    return MOVE;
}

void MoveTool::loadSettings()
{
    properties.width = -1;
    properties.feather = -1;
    properties.useFeather = false;
    properties.stabilizerLevel = -1;
    properties.useAA = -1;
    mRotationIncrement = mEditor->preference()->getInt(SETTING::ROTATION_INCREMENT);
    QSettings settings(PENCIL2D, PENCIL2D);
    properties.showSelectionInfo = settings.value("ShowSelectionInfo").toBool();
    mPropertyEnabled[SHOWSELECTIONINFO] = true;

    connect(mEditor->preference(), &PreferenceManager::optionChanged, this, &MoveTool::updateSettings);
}

QCursor MoveTool::cursor()
{
    MoveMode mode = MoveMode::NONE;
    SelectionManager* selectMan = mEditor->select();
    if (selectMan->somethingSelected())
    {
        mode = mEditor->select()->getMoveMode();
    }
    else if (mEditor->overlays()->anyOverlayEnabled())
    {
        LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
        Q_ASSERT(layerCam);
        mode = mEditor->overlays()->getMoveModeForPoint(getCurrentPoint(), layerCam->getViewAtFrame(mEditor->currentFrame()));
        mPerspMode = mode;
    }

    return cursor(mode);
}

void MoveTool::updateSettings(const SETTING setting)
{
    switch (setting)
    {
    case SETTING::ROTATION_INCREMENT:
        mRotationIncrement = mEditor->preference()->getInt(SETTING::ROTATION_INCREMENT);
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

bool MoveTool::keyPressEvent(QKeyEvent* keyEvent)
{
    auto selectMan = mEditor->select();
    if (!selectMan->somethingSelected()) { return false; }

    switch (keyEvent->key())
    {
    case Qt::Key_Right:
        selectMan->translate(QPointF(1, 0));
        selectMan->calculateSelectionTransformation();
        emit mEditor->frameModified(mEditor->currentFrame());
        return true;
    case Qt::Key_Left:
        selectMan->translate(QPointF(-1, 0));
        selectMan->calculateSelectionTransformation();
        emit mEditor->frameModified(mEditor->currentFrame());
        return true;
    case Qt::Key_Up:
        selectMan->translate(QPointF(0, -1));
        selectMan->calculateSelectionTransformation();
        emit mEditor->frameModified(mEditor->currentFrame());
        return true;
    case Qt::Key_Down:
        selectMan->translate(QPointF(0, 1));
        selectMan->calculateSelectionTransformation();
        emit mEditor->frameModified(mEditor->currentFrame());
        return true;
    case Qt::Key_Return:
        applyTransformation();
        mEditor->deselectAll();
        return true;
    case Qt::Key_Escape:
        cancelTransformation();
        mEditor->deselectAll();
        return true;
    case Qt::Key_Backspace:
        mScribbleArea->deleteSelection();
        discardChanges();
        mEditor->deselectAll();
        return true;
    default:
        break;
    }

    keyEvent->ignore();

    return keyEvent->isAccepted();
}

void MoveTool::pointerPressEvent(PointerEvent* event)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    if (mEditor->select()->somethingSelected())
    {
        beginInteraction(event->modifiers(), currentLayer);
    }
    if (mEditor->overlays()->anyOverlayEnabled())
    {
        mEditor->overlays()->setMoveMode(mPerspMode);

        LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
        Q_ASSERT(layerCam);

        QPoint mapped = layerCam->getViewAtFrame(mEditor->currentFrame()).map(getCurrentPoint()).toPoint();
        mEditor->overlays()->updatePerspective(mapped);
    }

    mEditor->updateFrame();
}

void MoveTool::pointerMoveEvent(PointerEvent* event)
{
    Layer* currentLayer = currentPaintableLayer();
    if (currentLayer == nullptr) return;

    if (mScribbleArea->isPointerInUse())   // the user is also pressing the mouse (dragging)
    {
        transformSelection(event->modifiers());

        if (mEditor->overlays()->anyOverlayEnabled())
        {
            LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
            Q_ASSERT(layerCam);
            mEditor->overlays()->updatePerspective(layerCam->getViewAtFrame(mEditor->currentFrame()).map(getCurrentPoint()));
        }
        if (mEditor->select()->somethingSelected())
        {
            transformSelection(event->modifiers());
        }
    }
    else
    {
        // the user is moving the mouse without pressing it
        // update cursor to reflect selection corner interaction
        mEditor->select()->setMoveModeForAnchorInRange(getCurrentPoint());
        mScribbleArea->updateToolCursor();

        if (currentLayer->type() == Layer::VECTOR)
        {
            storeClosestVectorCurve(currentLayer);
        }
    }
    mEditor->updateFrame();
}

void MoveTool::pointerReleaseEvent(PointerEvent*)
{
    if (mEditor->overlays()->anyOverlayEnabled())
    {
        mEditor->overlays()->setMoveMode(MoveMode::NONE);
        mPerspMode = MoveMode::NONE;
    }

    auto selectMan = mEditor->select();
    if (!selectMan->somethingSelected())
        return;

    mScribbleArea->updateToolCursor();
    mEditor->frameModified(mEditor->currentFrame());
}

void MoveTool::transformSelection(Qt::KeyboardModifiers keyMod)
{
    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())
    {
        int rotationIncrement = 0;
        if (selectMan->getMoveMode() == MoveMode::ROTATION && keyMod & Qt::ShiftModifier)
        {
            rotationIncrement = mRotationIncrement;
        }

        selectMan->maintainAspectRatio(keyMod == Qt::ShiftModifier);
        selectMan->alignPositionToAxis(keyMod == Qt::ShiftModifier);

        qreal newAngle = 0;
        if (selectMan->getMoveMode() == MoveMode::ROTATION) {
            QPointF anchorPoint = selectMan->currentTransformAnchor();
            newAngle = selectMan->angleFromPoint(getCurrentPoint(), anchorPoint) - mRotatedAngle;
            mPreviousAngle = newAngle;
        }

        selectMan->adjustSelection(getCurrentPoint(), mOffset, newAngle, rotationIncrement);
    }
    else // there is nothing selected
    {
        selectMan->setMoveMode(MoveMode::NONE);
    }
}

void MoveTool::beginInteraction(Qt::KeyboardModifiers keyMod, Layer* layer)
{
    auto selectMan = mEditor->select();
    QRectF selectionRect = selectMan->mySelectionRect();
    if (!selectionRect.isNull())
    {
        mEditor->backup(typeName());
    }

    if (keyMod != Qt::ShiftModifier)
    {
        if (selectMan->isOutsideSelectionArea(getCurrentPoint()))
        {
            applyTransformation();
            mEditor->deselectAll();
        }
    }

    if (selectMan->getMoveMode() == MoveMode::MIDDLE)
    {
        if (keyMod == Qt::ControlModifier) // --- rotation
        {
            selectMan->setMoveMode(MoveMode::ROTATION);
        }
    }

    if (layer->type() == Layer::VECTOR)
    {
        createVectorSelection(keyMod, layer);
    }

    selectMan->setTransformAnchor(selectMan->getSelectionAnchorPoint());
    selectMan->setDragOrigin(getCurrentPressPoint());
    mOffset = selectMan->myTranslation();

    if(selectMan->getMoveMode() == MoveMode::ROTATION) {
        mRotatedAngle = selectMan->angleFromPoint(getCurrentPoint(), selectMan->currentTransformAnchor()) - mPreviousAngle;
    }
}

/**
 * @brief MoveTool::createVectorSelection
 * In vector the selection rectangle is based on the bounding box of the curves
 * We can therefore create a selection just by clicking near/on a curve
 */
void MoveTool::createVectorSelection(Qt::KeyboardModifiers keyMod, Layer* layer)
{
    assert(layer->type() == Layer::VECTOR);
    LayerVector* vecLayer = static_cast<LayerVector*>(layer);
    VectorImage* vectorImage = vecLayer->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (vectorImage == nullptr) { return; }

    if (!mEditor->select()->closestCurves().empty()) // the user clicks near a curve
    {
        setCurveSelected(vectorImage, keyMod);
    }
    else if (vectorImage->getLastAreaNumber(getLastPoint()) > -1)
    {
        setAreaSelected(vectorImage, keyMod);
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
        selectMan->setSelection(vectorImage->getSelectionRect(), false);
    }
}

void MoveTool::setAreaSelected(VectorImage* vectorImage, Qt::KeyboardModifiers keyMod)
{
    int areaNumber = vectorImage->getLastAreaNumber(getLastPoint());
    if (!vectorImage->isAreaSelected(areaNumber))
    {
        if (keyMod != Qt::ShiftModifier)
        {
            applyTransformation();
        }
        vectorImage->setAreaSelected(areaNumber, true);
        mEditor->select()->setSelection(vectorImage->getSelectionRect(), false);
    }
}

/**
 * @brief MoveTool::storeClosestVectorCurve
 * stores the curves closest to the mouse position in mClosestCurves
 */
void MoveTool::storeClosestVectorCurve(Layer* layer)
{
    auto selectMan = mEditor->select();
    auto layerVector = static_cast<LayerVector*>(layer);
    VectorImage* pVecImg = layerVector->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
    if (pVecImg == nullptr) { return; }
    selectMan->setCurves(pVecImg->getCurvesCloseTo(getCurrentPoint(), selectMan->selectionTolerance()));
}

void MoveTool::applyTransformation()
{
    SelectionManager* selectMan = mEditor->select();
    commitChanges();

    // When the selection has been applied, a new rect is applied based on the bounding box.
    // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
    selectMan->setSelection(selectMan->mapToSelection(QPolygonF(selectMan->mySelectionRect())).boundingRect());
    mRotatedAngle = 0;
    mPreviousAngle = 0;
}

void MoveTool::cancelTransformation()
{
    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (layer == nullptr) { return; }

        discardChanges();

        mEditor->select()->setSelection(selectMan->mySelectionRect(), false);

        selectMan->resetSelectionProperties();

        mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    }
}

void MoveTool::setFloatingImage(BitmapImage& bitmapImage)
{
    Layer* layer = mEditor->layers()->currentLayer();
    int currentFrameNumber = mEditor->currentFrame();
    Q_ASSERT(layer->type() == Layer::BITMAP);

    mEditor->select()->setSelection(bitmapImage.bounds(), true);
    BitmapImage* currentKeyFrame = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(currentFrameNumber);
    currentKeyFrame->setTemporaryImage(*bitmapImage.image());

    emit mEditor->frameModified(currentFrameNumber);
}

void MoveTool::setFloatingImage(VectorImage& vectorImage)
{
    int currentFrameNumber = mEditor->currentFrame();
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    vectorImage.calculateSelectionRect();
    mEditor->select()->setSelection(vectorImage.getSelectionRect());

    VectorImage* currentKeyFrame = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(currentFrameNumber, 0);
    currentKeyFrame->setTemporaryImage(vectorImage.clone());

    emit mEditor->frameModified(currentFrameNumber);
}

void MoveTool::discardChanges()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    int currentFrameNumber = mEditor->currentFrame();
    if (layer->type() == Layer::BITMAP) {
        BitmapImage* currentKeyFrame = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(currentFrameNumber);

        if (!currentKeyFrame->temporaryImage().isNull()) {
            currentKeyFrame->clearTemporaryImage();
        }
    } else if (layer->type() == Layer::VECTOR) {
        VectorImage* currentFrameImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(currentFrameNumber, 0);
        currentFrameImage->resetSelectionTransform();
    }
}

void MoveTool::commitChanges()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    auto selectMan = mEditor->select();
    const int currentFrameNumber = mEditor->currentFrame();
    if (selectMan->somethingSelected())
    {
        const QTransform& selectionTransform = selectMan->selectionTransform();
        const QRectF& selectionRect = selectMan->mySelectionRect();
        if (selectionRect.isEmpty()) { return; }

        if (layer->type() == Layer::BITMAP)
        {
            // TODO: re-implement: handleDrawingOnEmptyFrame();
            const QRect& alignedSelectionRect = selectionRect.toAlignedRect();
            BitmapImage* currentKeyFrame = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(currentFrameNumber);
            if (currentKeyFrame == nullptr) { return; }

            const QImage& floatingImage = currentKeyFrame->temporaryImage();
            if (!floatingImage.isNull()) {
                const QRect& transformedSelectionRect = selectionTransform.mapRect(alignedSelectionRect);
                const QImage& transformedFloatingImage = floatingImage.transformed(selectionTransform, Qt::SmoothTransformation);

                auto floatingBitmapImage = BitmapImage(transformedSelectionRect.topLeft(), transformedFloatingImage);
                currentKeyFrame->paste(&floatingBitmapImage, QPainter::CompositionMode_SourceOver);
                currentKeyFrame->clearTemporaryImage();
            } else {
                BitmapImage transformedImage = currentKeyFrame->transformed(alignedSelectionRect, selectionTransform, true);
                currentKeyFrame->clear(selectionRect);
                currentKeyFrame->paste(&transformedImage, QPainter::CompositionMode_SourceOver);
            }
        }
        else if (layer->type() == Layer::VECTOR)
        {
            // Unfortunately this doesn't work right currently so vector transforms
            // will always be applied on the previous keyframe when on an empty frame
            // TODO: re-implement: handleDrawingOnEmptyFrame() and figure out why it does not work as intended;
            VectorImage* currentKeyFrame = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(currentFrameNumber, 0);

            if (currentKeyFrame->temporaryImage()) {
                currentKeyFrame->paste(*currentKeyFrame->temporaryImage());
                currentKeyFrame->resetTemporaryImage();
            }
            currentKeyFrame->applySelectionTransformation();
        }

        mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    }
}

bool MoveTool::leavingThisTool()
{
    if (currentPaintableLayer())
    {
        applyTransformation();
    }
    return true;
}

void MoveTool::resetToDefault()
{
    setShowSelectionInfo(false);
}

void MoveTool::setShowSelectionInfo(const bool b)
{
    properties.showSelectionInfo = b;

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("ShowSelectionInfo", b);

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

QCursor MoveTool::cursor(MoveMode mode) const
{
    QPixmap cursorPixmap = QPixmap(24, 24);

    cursorPixmap.fill(QColor(255, 255, 255, 0));
    QPainter cursorPainter(&cursorPixmap);
    cursorPainter.setRenderHint(QPainter::Antialiasing);

    switch(mode)
    {
    case MoveMode::PERSP_LEFT:
    case MoveMode::PERSP_RIGHT:
    case MoveMode::PERSP_MIDDLE:
    case MoveMode::PERSP_SINGLE:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-move.svg"));
        break;
    }
    case MoveMode::TOPLEFT:
    case MoveMode::BOTTOMRIGHT:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-diagonal-left.svg"));
        break;
    }
    case MoveMode::TOPRIGHT:
    case MoveMode::BOTTOMLEFT:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-diagonal-right.svg"));
        break;
    }
    case MoveMode::ROTATIONLEFT:
    case MoveMode::ROTATIONRIGHT:
    case MoveMode::ROTATION:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-rotate.svg"));
        break;
    }
    case MoveMode::MIDDLE:
    case MoveMode::CENTER:
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
