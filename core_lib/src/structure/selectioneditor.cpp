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

#include "mathutils.h"

#include <QVector2D>
#include <QDebug>

#include <QPolygon>

SelectionEditor::SelectionEditor()
{
    mIsValid = false;
}

SelectionEditor::SelectionEditor(SelectionState* state) : mState(state)
{
    // The Editor is valid when it has been created with a SelectionState and the ptr is valid
    mIsValid = true;
}

SelectionEditor::~SelectionEditor()
{
    qDebug() << "SelectionEditor destroyed";

    invalidate();
}

void SelectionEditor::invalidate()
{
    mIsValid = false;
    mState = nullptr;

    if (mObservers.count() > 0) {
        mObservers.clear();
    }
}

void SelectionEditor::resetState()
{
    if (!mState) { return; }
    *mState = SelectionState();
}

void SelectionEditor::resetTransformation()
{
    mState->selectionTransform.reset();
}

void SelectionEditor::setTransform(const QTransform& transform)
{
    mState->selectionTransform = transform;
}

MoveMode SelectionEditor::resolveMoveModeForAnchorInRange(const QPointF &point, const QPolygonF& polygon, qreal selectionTolerance) const
{
    if (polygon.count() < 4)
    {
        return MoveMode::NONE;
    }

    QPolygonF projectedPolygon = mapToSelection(polygon);

    MoveMode moveMode = MoveMode::NONE;
    if (QLineF(point, projectedPolygon[0]).length() < selectionTolerance)
    {
        moveMode = MoveMode::TOPLEFT;
    }
    else if (QLineF(point, projectedPolygon[1]).length() < selectionTolerance)
    {
        moveMode = MoveMode::TOPRIGHT;
    }
    else if (QLineF(point, projectedPolygon[2]).length() < selectionTolerance)
    {
        moveMode = MoveMode::BOTTOMRIGHT;
    }
    else if (QLineF(point, projectedPolygon[3]).length() < selectionTolerance)
    {
        moveMode = MoveMode::BOTTOMLEFT;
    }
    else if (projectedPolygon.containsPoint(point, Qt::WindingFill))
    {
        moveMode = MoveMode::MIDDLE;
    }
    else
    {
        moveMode = MoveMode::NONE;
    }

    return moveMode;
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
        anchorPoint = selectionPolygon.boundingRect().center();
    }
    return anchorPoint;
}

void SelectionEditor::adjustCurrentSelection(const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement)
{
    switch (mMoveMode)
    {
    case MoveMode::MIDDLE: {
        QPointF newOffset = currentPoint - mDragOrigin;

        if (mLockAxis) {
            mState->translation = offset + alignedPositionToAxis(newOffset);
        } else {
            mState->translation = offset + newOffset;
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
    mState->translation += newPos;
}

void SelectionEditor::rotate(qreal angle, qreal lockedAngle)
{
    if (lockedAngle > 0) {
        mState->rotatedAngle = constrainRotationToAngle(angle, lockedAngle);
    } else {
        mState->rotatedAngle = angle;
    }
}

void SelectionEditor::scale(qreal sX, qreal sY)
{
    // Enforce negative scaling when
    // deliberately trying to transform in negative space
    if (mState->scaleX < 0) {
        sX = -sX;
    }
    if (qFuzzyIsNull(sX)) {
        // Scale must not become 0
        sX = 0.0001;
    }

    // Enforce negative scaling when
    // deliberately trying to transform in negative space
    if (mState->scaleY < 0) {
        sY = -sY;
    }
    if (qFuzzyIsNull(sY)) {
        // Scale must not become 0
        sY = 0.0001;
    }

    mState->scaleX = sX;
    mState->scaleY = sY;
}

int SelectionEditor::constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const
{
    return qRound(rotatedAngle / rotationIncrement) * rotationIncrement;
}

qreal SelectionEditor::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    return qRadiansToDegrees(MathUtils::getDifferenceAngle(mState->selectionTransform.map(anchorPoint), point));
}

void SelectionEditor::deselect()
{
    resetState();
    mMoveMode = MoveMode::NONE;
    mIsValid = false;
    mAspectRatioFixed = false;
    mLockAxis = false;
    onEvent(SelectionEvent::CHANGED);
}

void SelectionEditor::setTransformAnchor(const QPointF& point)
{
    const QPointF& oldAnchorPoint = mState->anchorPoint;
    QPointF newPos = mapToSelection(point);
    QPointF oldPos = mapToSelection(oldAnchorPoint);

    // Adjust translation based on anchor point to avoid moving the selection
    mState->translation = mState->translation - oldPos + newPos;
    mState->anchorPoint = point;
}

void SelectionEditor::calculateSelectionTransformation()
{
    QTransform t;
    t.translate(-mState->anchorPoint.x(), -mState->anchorPoint.y());
    QTransform t2;
    t2.translate(mState->translation.x(), mState->translation.y());

    QTransform r;
    r.rotate(mState->rotatedAngle);
    QTransform s;
    s.scale(mState->scaleX, mState->scaleY);
    mState->selectionTransform = t * s * r * t2;
    onEvent(SelectionEvent::CHANGED);
}

QPointF SelectionEditor::alignedPositionToAxis(QPointF currentPoint) const
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
        mState->scaleY = -mState->scaleY;
    }
    else
    {
        mState->scaleX = -mState->scaleX;
    }
    // TODO (MrStevns): Why is this needed, The transform anchor shouldn't be any different?
    // setTransformAnchor(mOriginalRect.center());
    calculateSelectionTransformation();
    onEvent(SelectionEvent::CHANGED);
}

void SelectionEditor::subscribe(SelectionEventCallback callback)
{
    mObservers.append(callback);
}

void SelectionEditor::onEvent(SelectionEvent event) const
{
    notify(event);
}

void SelectionEditor::notify(SelectionEvent event) const {
    for (auto& cb : mObservers) {
        cb(event);
    }
}
