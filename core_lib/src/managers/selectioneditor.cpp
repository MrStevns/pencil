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
#include "selectioneditor.h"

#include "editor.h"
#include "vectorimage.h"
#include "bitmapimage.h"
#include "keyframe.h"
#include "layer.h"

#include "mathutils.h"

#include <QVector2D>
#include <QDebug>

#include <QPolygon>


SelectionEditor::SelectionEditor()
{
}

SelectionEditor::SelectionEditor(SelectionEditor& editor)
{
    mScaleX = editor.mScaleX;
    mScaleY = editor.mScaleY;
    mAnchorPoint = editor.mAnchorPoint;
    mAspectRatioFixed = editor.mAspectRatioFixed;
    mLockAxis = editor.mLockAxis;
    mMoveMode = editor.mMoveMode;
    mTranslation = editor.mTranslation;
    mRotatedAngle = editor.mRotatedAngle;
}

SelectionEditor::~SelectionEditor()
{
    qDebug() << "SelectionEditor destroyed";
}

void SelectionEditor::cleanupCallbacks()
{
    selectionChanged = nullptr;
    selectionReset = nullptr;
}

void SelectionEditor::resetSelectionTransformProperties()
{
    mRotatedAngle = 0;
    mTranslation = QPointF(0, 0);
    mScaleX = 1;
    mScaleY = 1;
    mSelectionTransform.reset();
    mAnchorPoint = QPointF();
}

void SelectionEditor::resetSelectionTransform()
{
    mSelectionTransform.reset();
}

qreal SelectionEditor::selectionTolerance() const
{
    return qAbs(mSelectionTolerance/* * editor()->viewScaleInversed()*/);
}

QPointF SelectionEditor::getSelectionAnchorPoint(const QPolygonF& selectionPolygon) const
{
    QPointF anchorPoint;
    if (selectionPolygon.count() < 3) { return anchorPoint; }

    if (mMoveMode == MoveMode::BOTTOMRIGHT)
    {
        anchorPoint = selectionPolygon[0];
    }
    else if (mMoveMode == MoveMode::BOTTOMLEFT)
    {
        anchorPoint = selectionPolygon[1];
    }
    else if (mMoveMode == MoveMode::TOPLEFT)
    {
        anchorPoint = selectionPolygon[2];
    }
    else if (mMoveMode == MoveMode::TOPRIGHT)
    {
        anchorPoint = selectionPolygon[3];
    } else {
        anchorPoint = QLineF(selectionPolygon[0], selectionPolygon[2]).pointAt(.5);
    }
    return anchorPoint;
}


void SelectionEditor::setMoveModeForAnchorInRange(const QPolygonF& selectionPolygon, const QPointF& point)
{
    if (selectionPolygon.count() < 4)
    {
        mMoveMode = MoveMode::NONE;
        return;
    }

    QPolygonF projectedPolygon = mapToSelection(selectionPolygon);

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

void SelectionEditor::adjustCurrentSelection(const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement)
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

        QPolygonF projectedPolygon = mapToSelection(selectionPolygon);
        QVector2D currentPVec = QVector2D(currentPoint);

        qreal originWidth = selectionPolygon[1].x() - selectionPolygon[0].x();
        qreal originHeight = selectionPolygon[3].y() - selectionPolygon[0].y();

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

void SelectionEditor::translate(QPointF newPos)
{
    mTranslation += newPos;
}

void SelectionEditor::rotate(qreal angle, qreal lockedAngle)
{
    if (lockedAngle > 0) {
        mRotatedAngle = constrainRotationToAngle(angle, lockedAngle);
    } else {
        mRotatedAngle = angle;
    }
}

void SelectionEditor::scale(qreal sX, qreal sY)
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

int SelectionEditor::constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const
{
    return qRound(rotatedAngle / rotationIncrement) * rotationIncrement;
}

qreal SelectionEditor::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    return qRadiansToDegrees(MathUtils::getDifferenceAngle(mSelectionTransform.map(anchorPoint), point));
}

void SelectionEditor::setSelection(const QRectF& rect)
{
    Q_UNUSED(rect)
    selectionChanged();
}

void SelectionEditor::deselect()
{
    resetSelectionProperties();
    mMoveMode = MoveMode::NONE;
    selectionChanged();
}

void SelectionEditor::setTransformAnchor(const QPointF& point)
{
    QPointF newPos = mapToSelection(point);
    QPointF oldPos = mapToSelection(mAnchorPoint);

    // Adjust translation based on anchor point to avoid moving the selection
    mTranslation = mTranslation - oldPos + newPos;
    mAnchorPoint = point;
}

void SelectionEditor::calculateSelectionTransformation()
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
    selectionChanged();
}

QPointF SelectionEditor::alignPositionToAxis(QPointF currentPoint) const
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
void SelectionEditor::flipSelection(bool flipVertical)
{
    if (flipVertical)
    {
        mScaleY = -mScaleY;
    }
    else
    {
        mScaleX = -mScaleX;
    }
    // TODO (MrStevns): Why is this needed, The transform anchor shouldn't be any different?
    // setTransformAnchor(mOriginalRect.center());
    calculateSelectionTransformation();
    selectionChanged();
}

void SelectionEditor::resetSelectionProperties()
{
    // resetSelectionTransformProperties();
    // mSelectionPolygon = QPolygonF();
    // mOriginalRect = QRectF();
    selectionChanged();
}


