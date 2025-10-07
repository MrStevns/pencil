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

    explicit SelectionEditor();
    ~SelectionEditor();

    void flipSelection(SelectionState& state, bool flipVertical);
    void deselect(SelectionState& state);

    // /** @brief SelectionManager::resetSelectionTransformProperties
    //  * should be used whenever translate, rotate, transform, scale
    //  * has been applied to a selection, but don't want to reset size nor position
    //  */
    // void resetSelectionTransformProperties();
    // /// The point from where the dragging will be based of inside the selection area.
    // /// Not to be confused with the selection origin
    // void setDragOrigin(const QPointF& point) = 0;

    // QPointF getSelectionAnchorPoint() const = 0;
    // bool somethingSelected() const = 0;

    // Replaces resetSelectionProperties();
    void resetState(SelectionState& state);
    // bool isOutsideSelectionArea(const QPointF& point) const = 0;

    // void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement) = 0;

    // void deleteSelection() = 0;

    // void commitChanges() = 0;
    // void discardChanges() = 0;

    // QPointF currentTransformAnchor() const { return mCommonState.anchorPoint; }

    void translate(SelectionState& state, QPointF point);
    void rotate(SelectionState& state, qreal angle, qreal lockedAngle);
    void scale(SelectionState& state, qreal sX, qreal sY);
    void maintainAspectRatio(bool state) { mAspectRatioFixed = state; }

    /** @brief Locks movement either horizontally or vertically depending on drag direction
     *  @param state */
    void alignedPositionToAxis(bool state) { mLockAxis = state; }
    /** @brief Aligns the input position to the nearest axis.
     *  Eg. draggin along the x axis, will keep the selection to that axis.
     * @param currentPosition the position of the cursor
     * @return A point that is either horizontally or vertically aligned with the current position.
     */
    QPointF alignedPositionToAxis(QPointF currentPoint) const;

    MoveMode getMoveMode() const { return mMoveMode; }
    void setMoveMode(const MoveMode moveMode) { mMoveMode = moveMode; }

    // QTransform selectionTransform() const { return mCommonState.selectionTransform; }
    // void setSelectionTransform(const QTransform& transform) { mCommonState.selectionTransform = transform; }
    // void resetSelectionTransform();

    qreal selectionTolerance() const;

    void setTransformAnchor(SelectionState& state, const QPointF& point);

    // SelectionState selectionState() { return mCommonState; }

    void setRotation(SelectionState& state, const qreal& rotation) { state.rotatedAngle = rotation; }
    void setScale(SelectionState& state, const qreal scaleX, const qreal scaleY) { state.scaleX = scaleX; state.scaleY = scaleY; }
    void setTranslation(SelectionState& state, const QPointF& translation) { state.translation = translation; }

    qreal angleFromPoint(SelectionState& state, const QPointF& point, const QPointF& anchorPoint) const;

    QPointF mapToSelection(const SelectionState& state, const QPointF& point) const { return state.selectionTransform.map(point); }
    QPointF mapFromLocalSpace(const SelectionState& state, const QPointF& point) const { return state.selectionTransform.inverted().map(point); }
    QPolygonF mapToSelection(const SelectionState& state, const QPolygonF& polygon) const { return state.selectionTransform.map(polygon); }
    QPolygonF mapFromLocalSpace(const SelectionState& state, const QPolygonF& polygon) const { return state.selectionTransform.inverted().map(polygon); }

    /// This should be called to update the selection transform
    void calculateSelectionTransformation(SelectionState& state);

    typedef std::function<void(SelectionEvent)> SelectionEventCallback;

    void subscribe(SelectionEventCallback callback);
    void onEvent(SelectionEvent event) const;
    void notify(SelectionEvent event) const;

    void adjustCurrentSelection(SelectionState& state, const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement);
    QPointF getSelectionAnchorPoint(const QPolygonF& selectionPolygon) const;

private:
    int constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const;

private:
    QList<SelectionEventCallback> mObservers;

    MoveMode mMoveMode = MoveMode::NONE;
    QPointF mDragOrigin;

    bool mIsValid = false;
    bool mAspectRatioFixed = false;
    bool mLockAxis = false;

    qreal mSelectionTolerance = 10.0;
};

#endif // SELECTIONEDITOR_H
