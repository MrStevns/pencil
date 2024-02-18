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
#include "selectionmanager.h"

#include "editor.h"
#include "vectorimage.h"
#include "bitmapimage.h"
#include "keyframe.h"
#include "layer.h"

#include "mathutils.h"

#include <QVector2D>


SelectionManager::SelectionManager(Editor* editor) : BaseManager(editor, __FUNCTION__)
{
}

SelectionManager::~SelectionManager()
{
}

bool SelectionManager::init()
{
    return true;
}

Status SelectionManager::load(Object*)
{
    resetSelectionProperties();
    return Status::OK;
}

Status SelectionManager::save(Object*)
{
    return Status::OK;
}

void SelectionManager::workingLayerChanged(Layer* workingLayer)
{
    mWorkingLayer = workingLayer;
}

bool SelectionManager::selectionBeganOnCurrentFrame() const
{
    if (!mWorkingLayer) {
        return false;
    }

    const KeyFrame* currentFrame = mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame());
    return mActiveKeyFrame != nullptr && currentFrame == mActiveKeyFrame;
}

void SelectionManager::commitChanges()
{
    if (somethingSelected())
    {
        const QTransform& selectionTransform = mSelectionTransform;
        const QRectF& selectionRect = mySelectionRect();
        if (selectionRect.isEmpty()) { return; }
        
        if (!mActiveKeyFrame) { return; }

        BitmapImage* currentBitmapImage = dynamic_cast<BitmapImage*>(mActiveKeyFrame);
        VectorImage* currentVectorImage = dynamic_cast<VectorImage*>(mActiveKeyFrame);
        if (currentBitmapImage)
        {
            const QRect& alignedSelectionRect = selectionRect.toAlignedRect();
            if (currentBitmapImage == nullptr) { return; }

            const QImage& floatingImage = currentBitmapImage->temporaryImage();
            if (!floatingImage.isNull()) {
                const QRect& transformedSelectionRect = selectionTransform.mapRect(alignedSelectionRect);
                const QImage& transformedFloatingImage = floatingImage.transformed(selectionTransform, Qt::SmoothTransformation);

                auto floatingBitmapImage = BitmapImage(transformedSelectionRect.topLeft(), transformedFloatingImage);
                currentBitmapImage->paste(&floatingBitmapImage, QPainter::CompositionMode_SourceOver);
                currentBitmapImage->clearTemporaryImage();
            } else {
                BitmapImage transformedImage = currentBitmapImage->transformed(alignedSelectionRect, selectionTransform, true);
                currentBitmapImage->clear(selectionRect);
                currentBitmapImage->paste(&transformedImage, QPainter::CompositionMode_SourceOver);
            }
            // When the selection has been applied, a new rect is applied based on the bounding box.
            // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
            adjustCurrentSelection(mapToSelection(QPolygonF(mySelectionRect())).boundingRect(), true);
        }
        else if (currentVectorImage)
        {
            if (currentVectorImage->temporaryImage()) {
                currentVectorImage->paste(*currentVectorImage->temporaryImage());
                currentVectorImage->resetTemporaryImage();
            }
            currentVectorImage->applySelectionTransformation();
            currentVectorImage->deselectAll();
        }
        editor()->setModified(editor()->currentLayerIndex(), editor()->currentFrame());
    }
    mActiveKeyFrame = nullptr;
}


void SelectionManager::discardChanges()
{
    KeyFrame* activeKeyFrame = mActiveKeyFrame;
    if (activeKeyFrame == nullptr) { return; }

    BitmapImage* activeBitmapImage = dynamic_cast<BitmapImage*>(activeKeyFrame);
    VectorImage* activeVectorImage = dynamic_cast<VectorImage*>(activeKeyFrame);
    if (activeBitmapImage) {

        if (!activeBitmapImage->temporaryImage().isNull()) {
            activeBitmapImage->clearTemporaryImage();
        }
    } else if (activeVectorImage) {
        activeVectorImage->resetSelectionTransform();

        if (activeVectorImage->temporaryImage()) {
            activeVectorImage->resetTemporaryImage();
        }
    }

    mActiveKeyFrame = nullptr;
    resetSelectionProperties();
    editor()->setModified(editor()->currentLayerIndex(), editor()->currentFrame());
}

void SelectionManager::resetSelectionTransformProperties()
{
    mRotatedAngle = 0;
    mTranslation = QPointF(0, 0);
    mScaleX = 1;
    mScaleY = 1;
    mAnchorPoint = QPoint();
    mSelectionTransform.reset();
}

void SelectionManager::resetSelectionTransform()
{
    mSelectionTransform.reset();
}

bool SelectionManager::isOutsideSelectionArea(const QPointF& point) const
{
    return (!mSelectionTransform.map(mSelectionPolygon).containsPoint(point, Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
}

void SelectionManager::deleteSelection()
{
    emit needDeleteSelection();
}

qreal SelectionManager::selectionTolerance() const
{
    return qAbs(mSelectionTolerance * editor()->viewScaleInversed());
}

QPointF SelectionManager::getSelectionAnchorPoint() const
{
    QPointF anchorPoint;
    if (mSelectionPolygon.count() < 3) { return anchorPoint; }

    if (mMoveMode == MoveMode::BOTTOMRIGHT)
    {
        anchorPoint = mSelectionPolygon[0];
    }
    else if (mMoveMode == MoveMode::BOTTOMLEFT)
    {
        anchorPoint = mSelectionPolygon[1];
    }
    else if (mMoveMode == MoveMode::TOPLEFT)
    {
        anchorPoint = mSelectionPolygon[2];
    }
    else if (mMoveMode == MoveMode::TOPRIGHT)
    {
        anchorPoint = mSelectionPolygon[3];
    } else {
        anchorPoint = QLineF(mSelectionPolygon[0], mSelectionPolygon[2]).pointAt(.5);
    }
    return anchorPoint;
}


void SelectionManager::setMoveModeForAnchorInRange(const QPointF& point)
{
    if (mSelectionPolygon.count() < 4)
    {
        mMoveMode = MoveMode::NONE;
        return;
    }

    if (!selectionBeganOnCurrentFrame()) { return; }

    QPolygonF projectedPolygon = mapToSelection(mSelectionPolygon);

    const double calculatedSelectionTol = selectionTolerance();

    if (QLineF(point, projectedPolygon[0]).length() < calculatedSelectionTol)
    {
        mMoveMode = MoveMode::TOPLEFT;
    }
    else if (QLineF(point, projectedPolygon[1]).length() < calculatedSelectionTol)
    {
        mMoveMode = MoveMode::TOPRIGHT;
    }
    else if (QLineF(point, projectedPolygon[2]).length() < calculatedSelectionTol)
    {
        mMoveMode = MoveMode::BOTTOMRIGHT;
    }
    else if (QLineF(point, projectedPolygon[3]).length() < calculatedSelectionTol)
    {
        mMoveMode = MoveMode::BOTTOMLEFT;
    }
    else if (projectedPolygon.containsPoint(point, Qt::WindingFill))
    {
        mMoveMode = MoveMode::MIDDLE;
    }
    else
    {
        mMoveMode = MoveMode::NONE;
    }
}

void SelectionManager::adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement)
{
    switch (mMoveMode)
    {
    case MoveMode::MIDDLE: {
        QPointF newOffset = currentPoint - mDragOrigin;

        if (mLockAxis) {
            mTranslation = offset + alignPositionToAxis(newOffset);
        } else {
            mTranslation = offset + newOffset;
        }
        break;
    }
    case MoveMode::TOPLEFT:
    case MoveMode::TOPRIGHT:
    case MoveMode::BOTTOMRIGHT:
    case MoveMode::BOTTOMLEFT: {

        QPolygonF projectedPolygon = mapToSelection(mSelectionPolygon);
        QVector2D currentPVec = QVector2D(currentPoint);

        qreal originWidth = mSelectionPolygon[1].x() - mSelectionPolygon[0].x();
        qreal originHeight = mSelectionPolygon[3].y() - mSelectionPolygon[0].y();

        QVector2D staticXAnchor;
        QVector2D staticYAnchor;
        QVector2D movingAnchor;
        if (mMoveMode == MoveMode::TOPLEFT) {
            movingAnchor = QVector2D(projectedPolygon[0]);
            staticXAnchor = QVector2D(projectedPolygon[1]);
            staticYAnchor = QVector2D(projectedPolygon[3]);
        } else if (mMoveMode == MoveMode::TOPRIGHT) {
            movingAnchor = QVector2D(projectedPolygon[1]);
            staticXAnchor = QVector2D(projectedPolygon[0]);
            staticYAnchor = QVector2D(projectedPolygon[2]);
        } else if (mMoveMode == MoveMode::BOTTOMRIGHT) {
            movingAnchor = QVector2D(projectedPolygon[2]);
            staticXAnchor = QVector2D(projectedPolygon[3]);
            staticYAnchor = QVector2D(projectedPolygon[1]);
        } else {
            movingAnchor = QVector2D(projectedPolygon[3]);
            staticXAnchor = QVector2D(projectedPolygon[2]);
            staticYAnchor = QVector2D(projectedPolygon[0]);
        }

        QVector2D directionVecX = staticXAnchor - currentPVec;
        QVector2D directionVecY = staticYAnchor - currentPVec;

        // Calculates the signed distance
        qreal distanceX = QVector2D::dotProduct(directionVecX, (staticXAnchor - movingAnchor).normalized());
        qreal distanceY = QVector2D::dotProduct(directionVecY, (staticYAnchor - movingAnchor).normalized());

        qreal scaleX = distanceX / originWidth;
        qreal scaleY = distanceY / originHeight;
        if (mAspectRatioFixed) {
            scaleY = scaleX;
        }

        scale(scaleX, scaleY);

        break;
    }
    case MoveMode::ROTATION: {
        rotate(rotationOffset, rotationIncrement);
        break;
    }
    default:
        break;
    }
    calculateSelectionTransformation();
}

void SelectionManager::translate(QPointF newPos)
{
    mTranslation += newPos;
}

void SelectionManager::rotate(qreal angle, qreal lockedAngle)
{
    if (lockedAngle > 0) {
        mRotatedAngle = constrainRotationToAngle(angle, lockedAngle);
    } else {
        mRotatedAngle = angle;
    }
}

void SelectionManager::scale(qreal sX, qreal sY)
{
    // Enforce negative scaling when
    // deliberately trying to transform in negative space
    if (mScaleX < 0) {
        sX = -sX;
    }
    if (qFuzzyIsNull(sX)) {
        // Scale must not become 0
        sX = 0.0001;
    }

    // Enforce negative scaling when
    // deliberately trying to transform in negative space
    if (mScaleY < 0) {
        sY = -sY;
    }
    if (qFuzzyIsNull(sY)) {
        // Scale must not become 0
        sY = 0.0001;
    }

    mScaleX = sX;
    mScaleY = sY;
}

int SelectionManager::constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const
{
    return qRound(rotatedAngle / rotationIncrement) * rotationIncrement;
}

qreal SelectionManager::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    return qRadiansToDegrees(MathUtils::getDifferenceAngle(mSelectionTransform.map(anchorPoint), point));
}

void SelectionManager::setSelection(KeyFrame* keyframe, QRectF rect, bool roundPixels)
{
    resetSelectionTransformProperties();
    if (roundPixels)
    {
        rect = rect.toAlignedRect();
    }
    mActiveKeyFrame = keyframe;
    mSelectionPolygon = rect;
    mOriginalRect = rect;

    emit selectionChanged();
}

void SelectionManager::adjustCurrentSelection(QRectF rect, bool roundPixels)
{
    resetSelectionTransformProperties();
    if (roundPixels)
    {
        rect = rect.toAlignedRect();
    }
    mSelectionPolygon = rect;
    mOriginalRect = rect;
    emit selectionChanged();
}

void SelectionManager::setTransformAnchor(const QPointF& point)
{
    QPointF newPos = mapToSelection(point);
    QPointF oldPos = mapToSelection(mAnchorPoint);

    // Adjust translation based on anchor point to avoid moving the selection
    mTranslation = mTranslation - oldPos + newPos;
    mAnchorPoint = point;
}

void SelectionManager::calculateSelectionTransformation()
{
    QTransform t;
    t.translate(-mAnchorPoint.x(), -mAnchorPoint.y());
    QTransform t2;
    t2.translate(mTranslation.x(), mTranslation.y());

    QTransform r;
    r.rotate(mRotatedAngle);
    QTransform s;
    s.scale(mScaleX, mScaleY);
    mSelectionTransform = t * s * r * t2;
    emit selectionChanged();
}

QPointF SelectionManager::alignPositionToAxis(QPointF currentPoint) const
{
    if (qAbs(currentPoint.y()) > qAbs(currentPoint.x())) {
        // Align to y axis
        return QPointF(0, currentPoint.y());
    }

    // Align to x axis
    return QPointF(currentPoint.x(), 0);
}

/**
 * @brief ScribbleArea::flipSelection
 * flip selection along the X or Y axis
*/
void SelectionManager::flipSelection(bool flipVertical)
{
    if (flipVertical)
    {
        mScaleY = -mScaleY;
    }
    else
    {
        mScaleX = -mScaleX;
    }
    setTransformAnchor(mOriginalRect.center());
    calculateSelectionTransformation();
    emit selectionChanged();
}

void SelectionManager::resetSelectionProperties()
{
    resetSelectionTransformProperties();
    mSelectionPolygon = QPolygonF();
    mOriginalRect = QRectF();
    emit selectionChanged();
}

