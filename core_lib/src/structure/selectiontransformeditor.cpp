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
#include "selectiontransformeditor.h"

#include "mathutils.h"

#include <QVector2D>
#include <QDebug>

#include <QPolygon>

SelectionTransformEditor::SelectionTransformEditor()
{
    mIsValid = false;
}

SelectionTransformEditor::SelectionTransformEditor(SelectionTransformState* state) : mState(state)
{
    if (!state) {
        invalidate(); return;
    }

    // The Editor is valid when it has been created with a SelectionState and the ptr is valid
    mIsValid = true;
}

SelectionTransformEditor::~SelectionTransformEditor()
{
    qDebug() << "SelectionEditor destroyed";

    invalidate();
}

void SelectionTransformEditor::invalidate()
{
    mIsValid = false;
    mState = nullptr;
}

void SelectionTransformEditor::resetState()
{
    if (!mState) { return; }
    *mState = SelectionTransformState();
}

void SelectionTransformEditor::resetTransformation()
{
    mState->selectionTransform.reset();
}

void SelectionTransformEditor::setTransform(const QTransform& transform)
{
    mState->selectionTransform = transform;
}

DragHandle SelectionTransformEditor::resolveHandleMode(const QPointF &point, const QPolygonF& polygon, qreal selectionTolerance) const
{
    if (polygon.count() < 4)
    {
        return DragHandle::NONE;
    }

    QPolygonF projectedPolygon = mapToSelection(polygon);

    DragHandle moveMode = DragHandle::NONE;
    if (QLineF(point, projectedPolygon[0]).length() < selectionTolerance)
    {
        moveMode = DragHandle::TOP_LEFT;
    }
    else if (QLineF(point, projectedPolygon[1]).length() < selectionTolerance)
    {
        moveMode = DragHandle::TOP_RIGHT;
    }
    else if (QLineF(point, projectedPolygon[2]).length() < selectionTolerance)
    {
        moveMode = DragHandle::BOTTOM_RIGHT;
    }
    else if (QLineF(point, projectedPolygon[3]).length() < selectionTolerance)
    {
        moveMode = DragHandle::BOTTOM_LEFT;
    }
    else if (projectedPolygon.containsPoint(point, Qt::WindingFill))
    {
        moveMode = DragHandle::CENTER;
    }

    return moveMode;
}

bool SelectionTransformEditor::isHandleInRange(const QPointF& currentPoint, const QPolygonF& selectionPolygon, qreal tolerance) const
{
    if (!mIsValid) { return false; }

    QPolygonF projectedPolygon = mapToSelection(selectionPolygon);
    for (QPointF point : projectedPolygon)
    {
        if (QLineF(currentPoint, point).length() < tolerance) {
            return true;
        }
    }
    return projectedPolygon.containsPoint(currentPoint, Qt::FillRule::WindingFill);
}

QPointF SelectionTransformEditor::resolveAnchorPoint(const QPointF& currentPoint, const QPolygonF& selectionPolygon, qreal tolerance) const
{
    QPointF anchorPoint;
    if (selectionPolygon.count() < 3) { return anchorPoint; }

    QPolygonF projectedPolygon = mapToSelection(selectionPolygon);

    if (QLineF(currentPoint, projectedPolygon[0]).length() < tolerance)
    {
        anchorPoint = selectionPolygon[2];
    }
    else if (QLineF(currentPoint, projectedPolygon[1]).length() < tolerance)
    {
        anchorPoint = selectionPolygon[3];
    }
    else if (QLineF(currentPoint, projectedPolygon[2]).length() < tolerance)
    {
        anchorPoint = selectionPolygon[0];
    }
    else if (QLineF(currentPoint, projectedPolygon[3]).length() < tolerance)
    {
        anchorPoint = selectionPolygon[1];
    } else {
        anchorPoint = selectionPolygon.boundingRect().center();
    }

    return anchorPoint;
}

bool SelectionTransformEditor::isOutsideSelection(const QPointF &point, const QPolygonF& polygon, qreal threshold) const
{
    return !isHandleInRange(point, polygon, threshold);
}

void SelectionTransformEditor::scaleAroundAnchorPoint(DragHandle handle, const QPolygonF& polygon, const QPointF& currentPoint)
{
    QPolygonF projectedPolygon = mapToSelection(polygon);
    QVector2D currentPVec = QVector2D(currentPoint);

    qreal originWidth = polygon[1].x() - polygon[0].x();
    qreal originHeight = polygon[3].y() - polygon[0].y();

    QVector2D staticXAnchor;
    QVector2D staticYAnchor;
    QVector2D movingAnchor;
    if (handle == DragHandle::TOP_LEFT) {
        movingAnchor = QVector2D(projectedPolygon[0]);
        staticXAnchor = QVector2D(projectedPolygon[1]);
        staticYAnchor = QVector2D(projectedPolygon[3]);
    } else if (handle == DragHandle::TOP_RIGHT) {
        movingAnchor = QVector2D(projectedPolygon[1]);
        staticXAnchor = QVector2D(projectedPolygon[0]);
        staticYAnchor = QVector2D(projectedPolygon[2]);
    } else if (handle == DragHandle::BOTTOM_RIGHT) {
        movingAnchor = QVector2D(projectedPolygon[2]);
        staticXAnchor = QVector2D(projectedPolygon[3]);
        staticYAnchor = QVector2D(projectedPolygon[1]);
    } else { // BOTTOM_LEFT
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
}

void SelectionTransformEditor::translate(QPointF newPos)
{
    if (mLockAxis) {
        newPos = alignedPositionToAxis(newPos);
    }

    mState->translation += newPos;
}

void SelectionTransformEditor::rotate(qreal angle, qreal angleIncrement)
{
    if (angleIncrement > 0) {
        mState->rotatedAngle = constrainRotationToAngle(angle, angleIncrement);
    } else {
        mState->rotatedAngle = angle;
    }
}

void SelectionTransformEditor::scale(qreal sX, qreal sY)
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

int SelectionTransformEditor::constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const
{
    return qRound(rotatedAngle / rotationIncrement) * rotationIncrement;
}

qreal SelectionTransformEditor::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    return qRadiansToDegrees(MathUtils::getDifferenceAngle(mState->selectionTransform.map(anchorPoint), point));
}

void SelectionTransformEditor::deselect()
{
    resetState();
    mIsValid = false;
    mAspectRatioFixed = false;
    mLockAxis = false;
}

void SelectionTransformEditor::setTransformAnchor(const QPointF& point)
{
    const QPointF& oldAnchorPoint = mState->anchorPoint;
    QPointF newPos = mapToSelection(point);
    QPointF oldPos = mapToSelection(oldAnchorPoint);

    // Adjust translation based on anchor point to avoid moving the selection
    mState->translation = mState->translation - oldPos + newPos;
    mState->anchorPoint = point;
}

void SelectionTransformEditor::calculateSelectionTransformation()
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
}

QPointF SelectionTransformEditor::alignedPositionToAxis(QPointF currentPoint) const
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
void SelectionTransformEditor::flipSelection(bool flipVertical)
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
}
