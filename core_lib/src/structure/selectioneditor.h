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
    SelectionEditor(SelectionEditor& editor);
    virtual ~SelectionEditor();

    void flipSelection(bool flipVertical);
    void deselect();

    /** @brief SelectionManager::resetSelectionTransformProperties
     * should be used whenever translate, rotate, transform, scale
     * has been applied to a selection, but don't want to reset size nor position
     */
    virtual void resetSelectionTransformProperties();
    /// The point from where the dragging will be based of inside the selection area.
    /// Not to be confused with the selection origin
    virtual void setDragOrigin(const QPointF& point) = 0;

    virtual QPointF getSelectionAnchorPoint() const = 0;
    virtual bool somethingSelected() const = 0;

    virtual void resetSelectionProperties();
    virtual bool isOutsideSelectionArea(const QPointF& point) const = 0;

    virtual void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement) = 0;

    virtual void deleteSelection() = 0;

    virtual void commitChanges() = 0;
    virtual void discardChanges() = 0;

    QPointF currentTransformAnchor() const { return mCommonState.anchorPoint; }

    void translate(QPointF point);
    void rotate(qreal angle, qreal lockedAngle);
    void scale(qreal sX, qreal sY);
    void maintainAspectRatio(bool state) { mAspectRatioFixed = state; }

    /** @brief Locks movement either horizontally or vertically depending on drag direction
     *  @param state */
    void alignPositionToAxis(bool state) { mLockAxis = state; }
    /** @brief Aligns the input position to the nearest axis.
     *  Eg. draggin along the x axis, will keep the selection to that axis.
     * @param currentPosition the position of the cursor
     * @return A point that is either horizontally or vertically aligned with the current position.
     */
    QPointF alignPositionToAxis(QPointF currentPoint) const;

    MoveMode getMoveMode() const { return mMoveMode; }
    void setMoveMode(const MoveMode moveMode) { mMoveMode = moveMode; }

    QTransform selectionTransform() const { return mCommonState.selectionTransform; }
    void setSelectionTransform(const QTransform& transform) { mCommonState.selectionTransform = transform; }
    void resetSelectionTransform();

    qreal selectionTolerance() const;

    void setTransformAnchor(const QPointF& point);

    // SelectionState selectionState() { return mCommonState; }

    void setRotation(const qreal& rotation) { mCommonState.rotatedAngle = rotation; }
    void setScale(const qreal scaleX, const qreal scaleY) { mCommonState.scaleX = scaleX; mCommonState.scaleY = scaleY; }
    void setTranslation(const QPointF& translation) { mCommonState.translation = translation; }

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    QPointF mapToSelection(const QPointF& point) const { return mCommonState.selectionTransform.map(point); }
    QPointF mapFromLocalSpace(const QPointF& point) const { return mCommonState.selectionTransform.inverted().map(point); }
    QPolygonF mapToSelection(const QPolygonF& polygon) const { return mCommonState.selectionTransform.map(polygon); }
    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const { return mCommonState.selectionTransform.inverted().map(polygon); }

    void invalidateCache() { mCacheInvalidated = true; }
    bool isCacheInvalidated() const { return mCacheInvalidated; }

    /// This should be called to update the selection transform
    void calculateSelectionTransformation();

    typedef std::function<void(SelectionEvent)> SelectionEventCallback;

    void subscribe(SelectionEventCallback callback);
    void onEvent(SelectionEvent event) const;
    void notify(SelectionEvent event) const;

protected:
    void adjustCurrentSelection(const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement);

    QPointF getSelectionAnchorPoint(const QPolygonF& selectionPolygon) const;

    MoveMode mMoveMode = MoveMode::NONE;
    QPointF mDragOrigin;
    bool mCacheInvalidated = true;

private:
    int constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const;

private:
    // SelectionState mCommonState;

    QList<SelectionEventCallback> mObservers;

    bool mAspectRatioFixed = false;
    bool mLockAxis = false;

    const qreal mSelectionTolerance = 10.0;
};

#endif // SELECTIONEDITOR_H
