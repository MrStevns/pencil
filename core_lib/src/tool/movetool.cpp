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

#include "pointerevent.h"
#include "editor.h"
#include "toolmanager.h"
#include "viewmanager.h"
#include "strokemanager.h"
#include "selectionmanager.h"
#include "scribblearea.h"
#include "layervector.h"
#include "layermanager.h"
#include "layercamera.h"
#include "mathutils.h"
#include "vectorimage.h"
#include "pencilsettings.h"

MoveTool::MoveTool(QObject* parent) : BaseTool(parent)
{
}

ToolType MoveTool::type()
{
    return MOVE;
}

void MoveTool::loadSettings()
{
    mPropertyEnabled[CAMERAPATH] = true;
    properties.width = -1;
    properties.feather = -1;
    properties.useFeather = false;
    properties.stabilizerLevel = -1;
    properties.useAA = -1;

    QSettings settings(PENCIL2D, PENCIL2D);

    properties.showCameraPath = settings.value(SETTING_CAMERA_SHOWPATH).toBool();
    mRotationIncrement = mEditor->preference()->getInt(SETTING::ROTATION_INCREMENT);

    connect(mEditor->preference(), &PreferenceManager::optionChanged, this, &MoveTool::updateSettings);
    connect(mEditor->layers(), &LayerManager::currentLayerChanged, this, &MoveTool::updateTool);
}

void MoveTool::updateTool()
{
    setShowCameraPath(properties.showCameraPath);
}

QCursor MoveTool::cursor()
{
    MoveMode mode = MoveMode::NONE;
    if (mEditor->select()->somethingSelected())
    {
        mode = mEditor->select()->getMoveModeForSelectionAnchor(getCurrentPoint());
        return mScribbleArea->currentTool()->selectMoveCursor(mode, type());
    }
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::CAMERA && layer->keyExists(mEditor->currentFrame()))
    {
        LayerCamera* cam = static_cast<LayerCamera*>(layer);
        mode = cam->getMoveModeForCamera(mEditor->currentFrame(),
                                         getCurrentPoint(),
                                         mEditor->select()->selectionTolerance() * mEditor->view()->scaling());
        mCamMoveMode = mode;
        return mScribbleArea->currentTool()->selectMoveCursor(mode, type());
    }
    else if (layer->type() == Layer::CAMERA)
    {
        LayerCamera* cam = static_cast<LayerCamera*>(layer);
        for ( int i = cam->firstKeyFramePosition(); i <= cam->getMaxKeyFramePosition(); i = cam->getNextKeyFramePosition(i))
        {
            mode = cam->getMoveModeForCameraPath(i,
                                                 getCurrentPoint(),
                                                 mEditor->select()->selectionTolerance() * mEditor->view()->scaling());
            mCamPathMoveMode = mode;
            if (mode != MoveMode::NONE)
            {
                if (!cam->hasSameTranslation(i, cam->getPreviousKeyFramePosition(i)))
                {
                    mDragPathFrame = i;
                    break;
                }
            }
            if (i == cam->getMaxKeyFramePosition())
                break;
        }
        return mScribbleArea->currentTool()->selectMoveCursor(mode, type());
    }

    return mScribbleArea->currentTool()->selectMoveCursor(mode, type());
}

void MoveTool::setShowCameraPath(const bool showCameraPath)
{
    LayerCamera* layer = static_cast<LayerCamera*>(editor()->layers()->currentLayer());

    if (layer->type() != Layer::CAMERA) { return; }
    layer->setShowCameraPath(showCameraPath);

    properties.showCameraPath = showCameraPath;
    QSettings settings(PENCIL2D, PENCIL2D);

    // Should we save a setting per layer?
    settings.setValue(SETTING_CAMERA_SHOWPATH, showCameraPath);
    settings.sync();
}

void MoveTool::setPathDotColor(const int pathDotColor)
{
    LayerCamera* layer = static_cast<LayerCamera*>(editor()->layers()->currentLayer());
    if (layer->type() != Layer::CAMERA) { return; }

    DotColor color = static_cast<DotColor>(pathDotColor);
    layer->setDotColor(color);
}

void MoveTool::resetCameraPath()
{
    LayerCamera* layer = static_cast<LayerCamera*>(editor()->layers()->currentLayer());
    if (layer->type() != Layer::CAMERA) { return; }

    layer->centerMidPoint(mEditor->currentFrame());
}

void MoveTool::updateSettings(const SETTING setting)
{
    switch (setting)
    {
    case SETTING::ROTATION_INCREMENT:
    {
        mRotationIncrement = mEditor->preference()->getInt(SETTING::ROTATION_INCREMENT);
        break;
    }
    default:
        break;

    }
}

void MoveTool::pointerPressEvent(PointerEvent* event)
{
    if (mCurrentLayer->type() == Layer::CAMERA &&
             mCurrentLayer->keyExists(mEditor->currentFrame()))
    {
        mDragPathFrame = mEditor->currentFrame();
        LayerCamera* camera = static_cast<LayerCamera*>(mCurrentLayer);
        camera->setOffsetPoint(getCurrentPoint());
        return;
    }

    mCurrentLayer = currentPaintableLayer();
    if (mCurrentLayer == nullptr) return;

    mEditor->select()->updatePolygons();

    setAnchorToLastPoint();
    beginInteraction(event->modifiers(), mCurrentLayer);
}

void MoveTool::pointerMoveEvent(PointerEvent* event)
{
    mCurrentLayer = currentPaintableLayer();
    if (mCurrentLayer == nullptr) return;

    mEditor->select()->updatePolygons();

    if (mScribbleArea->isPointerInUse())   // the user is also pressing the mouse (dragging)
    {
        if (mEditor->select()->somethingSelected())
        {
            transformSelection(event->modifiers(), mCurrentLayer);
        }
        else if (mEditor->layers()->currentLayer()->type() == Layer::CAMERA &&
                 mCurrentLayer->keyExists(mEditor->currentFrame()))
        {
            transformCamera();
        }
        else if (mEditor->layers()->currentLayer()->type() == Layer::CAMERA &&
                 mCamPathMoveMode == MoveMode::MIDDLE)
        {
            transformCameraPath();
        }
    }
    else
    {
        // the user is moving the mouse without pressing it
        // update cursor to reflect selection corner interaction
        mScribbleArea->updateToolCursor();

        if (mCurrentLayer->type() == Layer::VECTOR)
        {
            storeClosestVectorCurve(mCurrentLayer);
        }
    }
    mScribbleArea->updateCurrentFrame();
}

void MoveTool::pointerReleaseEvent(PointerEvent*)
{
    if (mEditor->layers()->currentLayer()->type() == Layer::CAMERA &&
             mCurrentLayer->keyExists(mEditor->currentFrame()))
    {
        transformCamera();
        mEditor->view()->updateViewTransforms();
        mScribbleArea->invalidateCacheForFrame(mEditor->currentFrame());
        return;
    }
    else if (mEditor->layers()->currentLayer()->type() == Layer::CAMERA &&
             mCamPathMoveMode == MoveMode::MIDDLE)
    {
        transformCameraPath();
        mEditor->view()->updateViewTransforms();
        mScribbleArea->invalidateCacheForFrame(mEditor->currentFrame());
        return;
    }

    auto selectMan = mEditor->select();
    if (!selectMan->somethingSelected())
        return;

    mRotatedAngle = selectMan->myRotation();
    updateTransformation();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::VECTOR) {
        applyTransformation();
    }

    selectMan->updatePolygons();

    mScribbleArea->updateToolCursor();
    mScribbleArea->updateCurrentFrame();
}

void MoveTool::updateTransformation()
{
    auto selectMan = mEditor->select();
    selectMan->updateTransformedSelection();

    // make sure transform is correct
    selectMan->calculateSelectionTransformation();

    // paint the transformation
    paintTransformedSelection();
}

void MoveTool::transformSelection(Qt::KeyboardModifiers keyMod, Layer* layer)
{
    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())
    {

        QPointF offset = offsetFromPressPos();

        // maintain aspect ratio
        if (keyMod == Qt::ShiftModifier)
        {
            offset = selectMan->offsetFromAspectRatio(offset.x(), offset.y());
        }

        int rotationIncrement = 0;
        if (selectMan->getMoveMode() == MoveMode::ROTATION && keyMod & Qt::ShiftModifier)
        {
            rotationIncrement = mRotationIncrement;
        }

        if(layer->type() == Layer::BITMAP)
        {
            offset = offset.toPoint();
        }

        selectMan->adjustSelection(getCurrentPoint(), offset.x(), offset.y(), mRotatedAngle, rotationIncrement);

        selectMan->calculateSelectionTransformation();
        paintTransformedSelection();

    }
    else // there is nothing selected
    {
        selectMan->setMoveMode(MoveMode::NONE);
    }
}

void MoveTool::beginInteraction(Qt::KeyboardModifiers keyMod, Layer* layer)
{
    auto selectMan = mEditor->select();
    QRectF selectionRect = selectMan->myTransformedSelectionRect();
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

    if (selectMan->validateMoveMode(getLastPoint()) == MoveMode::MIDDLE)
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

    if(selectMan->getMoveMode() == MoveMode::ROTATION) {
        QPointF curPoint = getCurrentPoint();
        QPointF anchorPoint = selectionRect.center();
        mRotatedAngle = qRadiansToDegrees(MathUtils::getDifferenceAngle(anchorPoint, curPoint)) - selectMan->myRotation();
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
    mScribbleArea->update();
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

void MoveTool::transformCamera()
{
    LayerCamera* layer = static_cast<LayerCamera*>(mCurrentLayer);
    layer->transformCameraView(mCamMoveMode, getCurrentPoint(), mEditor->currentFrame());
    mScribbleArea->invalidateLayerPixmapCache();
}

void MoveTool::transformCameraPath()
{
    LayerCamera* layer = static_cast<LayerCamera*>(mCurrentLayer);
    layer->dragCameraPath(mCamPathMoveMode, getCurrentPoint(), mDragPathFrame);
    mScribbleArea->invalidateLayerPixmapCache();
}

void MoveTool::setAnchorToLastPoint()
{
    anchorOriginPoint = getLastPoint();
}

void MoveTool::cancelChanges()
{
    auto selectMan = mEditor->select();
    mScribbleArea->cancelTransformedSelection();
    selectMan->resetSelectionProperties();
    mEditor->deselectAll();
}

void MoveTool::applySelectionChanges()
{
    mEditor->select()->setRotation(0);
    mRotatedAngle = 0;

    mScribbleArea->applySelectionChanges();
}

void MoveTool::applyTransformation()
{
    mScribbleArea->applyTransformedSelection();
}

void MoveTool::paintTransformedSelection()
{
    mScribbleArea->paintTransformedSelection();
}

bool MoveTool::leavingThisTool()
{
    if (mCurrentLayer)
    {
        switch (mCurrentLayer->type())
        {
        case Layer::BITMAP: applySelectionChanges(); break;
        case Layer::VECTOR: applyTransformation(); break;
        default: break;
        }
    }
    return true;
}

bool MoveTool::switchingLayer()
{
    auto selectMan = mEditor->select();
    if (!selectMan->transformHasBeenModified())
    {
        mEditor->deselectAll();
        return true;
    }

    int returnValue = showTransformWarning();

    if (returnValue == QMessageBox::Yes)
    {
        if (mCurrentLayer->type() == Layer::BITMAP)
        {
            applySelectionChanges();
        }
        else if (mCurrentLayer->type() == Layer::VECTOR)
        {
            applyTransformation();
        }

        mEditor->deselectAll();
        return true;
    }
    else if (returnValue == QMessageBox::No)
    {
        cancelChanges();
        return true;
    }
    else if (returnValue == QMessageBox::Cancel)
    {
        return false;
    }
    return true;
}

int MoveTool::showTransformWarning()
{
    int returnValue = QMessageBox::warning(nullptr,
                                           tr("Layer switch", "Windows title of layer switch pop-up."),
                                           tr("You are about to switch away, do you want to apply the transformation?"),
                                           QMessageBox::No | QMessageBox::Cancel | QMessageBox::Yes,
                                           QMessageBox::Yes);
    return returnValue;
}

Layer* MoveTool::currentPaintableLayer()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr)
        return nullptr;
    if (layer->type() == Layer::CAMERA)
        return layer;   // ONLY for movetool!
    if (!layer->isPaintable())
        return nullptr;
    return layer;
}

QPointF MoveTool::offsetFromPressPos()
{
    return getCurrentPoint() - getCurrentPressPoint();
}
