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
}

// SelectionEditor::SelectionEditor(SelectionState& state)
// {
//     mCommonState = editor.mCommonState;

//     mAspectRatioFixed = editor.mAspectRatioFixed;
//     mLockAxis = editor.mLockAxis;
//     mMoveMode = editor.mMoveMode;
//     mMoveMode = editor.mMoveMode;
//     mDragOrigin = editor.mDragOrigin;
// }

SelectionEditor::~SelectionEditor()
{
    qDebug() << "SelectionEditor destroyed";

    if (mObservers.count() > 0) {
        mObservers.clear();
    }
}

void SelectionEditor::resetState(SelectionState& state)
{
    state = SelectionState();
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
        anchorPoint = selectionPolygon.boundingRect().center();
    }
    return anchorPoint;
}

void SelectionEditor::adjustCurrentSelection(SelectionState& state, const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement)
{
    switch (mMoveMode)
    {
    case MoveMode::MIDDLE: {
        QPointF newOffset = currentPoint - mDragOrigin;

        if (mLockAxis) {
            state.translation = offset + alignedPositionToAxis(newOffset);
        } else {
            state.translation = offset + newOffset;
        }
        break;
    }
    case MoveMode::TOPLEFT:
    case MoveMode::TOPRIGHT:
    case MoveMode::BOTTOMRIGHT:
    case MoveMode::BOTTOMLEFT: {

        QPolygonF projectedPolygon = mapToSelection(state, selectionPolygon);
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

        scale(state, scaleX, scaleY);

        break;
    }
    case MoveMode::ROTATION: {
        rotate(state, rotationOffset, rotationIncrement);
        break;
    }
    default:
        break;
    }
    calculateSelectionTransformation(state);
}

void SelectionEditor::translate(SelectionState& state, QPointF newPos)
{
    state.translation += newPos;
}

void SelectionEditor::rotate(SelectionState& state, qreal angle, qreal lockedAngle)
{
    if (lockedAngle > 0) {
        state.rotatedAngle = constrainRotationToAngle(angle, lockedAngle);
    } else {
        state.rotatedAngle = angle;
    }
}

void SelectionEditor::scale(SelectionState& state, qreal sX, qreal sY)
{
    // Enforce negative scaling when
    // deliberately trying to transform in negative space
    if (state.scaleX < 0) {
        sX = -sX;
    }
    if (qFuzzyIsNull(sX)) {
        // Scale must not become 0
        sX = 0.0001;
    }

    // Enforce negative scaling when
    // deliberately trying to transform in negative space
    if (state.scaleY < 0) {
        sY = -sY;
    }
    if (qFuzzyIsNull(sY)) {
        // Scale must not become 0
        sY = 0.0001;
    }

    state.scaleX = sX;
    state.scaleY = sY;
}

int SelectionEditor::constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const
{
    return qRound(rotatedAngle / rotationIncrement) * rotationIncrement;
}

qreal SelectionEditor::angleFromPoint(SelectionState& state, const QPointF& point, const QPointF& anchorPoint) const
{
    return qRadiansToDegrees(MathUtils::getDifferenceAngle(state.selectionTransform.map(anchorPoint), point));
}

// void SelectionEditor::setSelection(const QRectF& rect)
// {
//     Q_UNUSED(rect)
//     onEvent(SelectionEvent::CHANGED);
// }

// void SelectionEditor::setSelection(const QPolygonF &polygon)
// {
//     Q_UNUSED(polygon)
//     onEvent(SelectionEvent::CHANGED);
// }

void SelectionEditor::deselect(SelectionState& state)
{
    resetState(state);
    mMoveMode = MoveMode::NONE;
    mIsValid = false;
    mAspectRatioFixed = false;
    mLockAxis = false;
    onEvent(SelectionEvent::CHANGED);
}

void SelectionEditor::setTransformAnchor(SelectionState& state, const QPointF& point)
{
    const QPointF& oldAnchorPoint = state.anchorPoint;
    QPointF newPos = mapToSelection(state, point);
    QPointF oldPos = mapToSelection(state, oldAnchorPoint);

    // Adjust translation based on anchor point to avoid moving the selection
    state.translation = state.translation - oldPos + newPos;
    state.anchorPoint = point;
}

void SelectionEditor::calculateSelectionTransformation(SelectionState& state)
{
    QTransform t;
    t.translate(-state.anchorPoint.x(), -state.anchorPoint.y());
    QTransform t2;
    t2.translate(state.translation.x(), state.translation.y());

    QTransform r;
    r.rotate(state.rotatedAngle);
    QTransform s;
    s.scale(state.scaleX, state.scaleY);
    state.selectionTransform = t * s * r * t2;
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
void SelectionEditor::flipSelection(SelectionState& state, bool flipVertical)
{
    if (flipVertical)
    {
        state.scaleY = -state.scaleY;
    }
    else
    {
        state.scaleX = -state.scaleX;
    }
    // TODO (MrStevns): Why is this needed, The transform anchor shouldn't be any different?
    // setTransformAnchor(mOriginalRect.center());
    calculateSelectionTransformation(state);
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
