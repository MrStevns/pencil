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
#ifndef SELECTIONEDITOR_H
#define SELECTIONEDITOR_H

#include "movemode.h"
#include "selectionstate.h"

#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QTransform>
#include <QList>

enum class SelectionEvent {
    NONE,
    CHANGED,
    RESET
};

class SelectionEditor
{
public:

    SelectionEditor();
    SelectionEditor(SelectionState* state);
    ~SelectionEditor();

    void flipSelection(bool flipVertical);
    void deselect();

    void resetState();

    // QTransform selectionTransform() const { return mCommonState.selectionTransform; }
    // void setSelectionTransform(const QTransform& transform) { mCommonState.selectionTransform = transform; }
    void resetTransformation();
    void setTransform(const QTransform& transform);

    void translate(QPointF point);
    void rotate(qreal angle, qreal lockedAngle);
    void scale(qreal sX, qreal sY);
    void maintainAspectRatio(bool state) { mAspectRatioFixed = state; }

    /** @brief Locks movement either horizontally or vertically depending on drag direction
     *  @param state */
    void lockMovementToAxis(const bool state) { mLockAxis = state; }
    /** @brief Aligns the input position to the nearest axis.
     *  Eg. draggin along the x axis, will keep the selection to that axis.
     * @param currentPosition the position of the cursor
     * @return A point that is either horizontally or vertically aligned with the current position.
     */
    QPointF alignedPositionToAxis(QPointF currentPoint) const;

    MoveMode getMoveMode() const { return mMoveMode; }
    void setMoveMode(const MoveMode moveMode) { mMoveMode = moveMode; }

    MoveMode resolveMoveModeForAnchorInRange(const QPointF &point, const QPolygonF& polygon, qreal selectionTolerance) const;

    QPointF currentAnchorPoint() const { return mState->anchorPoint; }
    void setTransformAnchor(const QPointF& point);

    void setDragOrigin(const QPointF& point) { mDragOrigin = point; }

    bool isOutsideSelection(const QPointF& point, const QPolygonF& polygon) const;

    qreal myRotation() const { return mState->rotatedAngle; }
    qreal myScaleX() const { return mState->scaleX; }
    qreal myScaleY() const { return mState->scaleY; }
    QPointF myTranslation() const { return mState->translation; }
    QTransform myTransform() const { return mState->selectionTransform; }


    void setRotation(const qreal& rotation) { mState->rotatedAngle = rotation; }
    void setScale(const qreal scaleX, const qreal scaleY) { mState->scaleX = scaleX; mState->scaleY = scaleY; }
    void setTranslation(const QPointF& translation) { mState->translation = translation; }

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    QPointF mapToSelection(const QPointF& point) const { return mState->selectionTransform.map(point); }
    QPointF mapFromLocalSpace(const QPointF& point) const { return mState->selectionTransform.inverted().map(point); }
    QPolygonF mapToSelection(const QPolygonF& polygon) const { return mState->selectionTransform.map(polygon); }
    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const { return mState->selectionTransform.inverted().map(polygon); }

    /// This should be called to update the selection transform
    void calculateSelectionTransformation();

    typedef std::function<void(SelectionEvent)> SelectionEventCallback;

    void subscribe(SelectionEventCallback callback);
    void onEvent(SelectionEvent event) const;
    void notify(SelectionEvent event) const;

    void adjustCurrentSelection(const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement);
    QPointF getSelectionAnchorPoint(const QPolygonF& selectionPolygon) const;

    void invalidate();
    bool isValid() { return mIsValid && mState != nullptr; }

private:
    int constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const;

private:
    SelectionState* mState = nullptr;
    QList<SelectionEventCallback> mObservers;

    MoveMode mMoveMode = MoveMode::NONE;
    QPointF mDragOrigin;

    bool mIsValid = false;
    bool mAspectRatioFixed = false;
    bool mLockAxis = false;
};

#endif // SELECTIONEDITOR_H
