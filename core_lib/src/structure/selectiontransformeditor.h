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
#ifndef SELECTIONTRANSFORMEDITOR_H
#define SELECTIONTRANSFORMEDITOR_H

#include "perspectivemode.h"
#include "selectionstate.h"

#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QTransform>
#include <QList>

#include "pencildef.h"

class SelectionTransformEditor
{
public:

    SelectionTransformEditor();
    SelectionTransformEditor(SelectionTransformState* state);
    ~SelectionTransformEditor();

    void flipSelection(bool flipVertical);
    void deselect();

    void resetState();

    void resetTransformation();
    void setTransform(const QTransform& transform);

    void scaleAroundAnchorPoint(DragHandle handle, const QPolygonF& polygon, const QPointF& currentPoint);
    void adjustTranslation(const QPointF& currentPoint, const QPointF& offset);
    void translate(QPointF point);
    void rotate(qreal angle, qreal angleIncrement);
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

    bool isHandleInRange(const QPointF& currentPoint, const QPolygonF& selectionPolygon, qreal tolerance) const;

    DragHandle resolveHandleMode(const QPointF &point, const QPolygonF& polygon, qreal selectionTolerance) const;

    QPointF currentAnchorPoint() const { return mState->anchorPoint; }
    void setTransformAnchor(const QPointF& point);

    bool isOutsideSelection(const QPointF& point, const QPolygonF& polygon, qreal threshold) const;

    qreal rotationAngle() const { return mState->rotatedAngle; }
    qreal scaleX() const { return mState->scaleX; }
    qreal scaleY() const { return mState->scaleY; }
    QPointF translation() const { return mState->translation; }
    QTransform transform() const { return mState->selectionTransform; }

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

    QPointF resolveAnchorPoint(const QPointF& currentPoint, const QPolygonF& selectionPolygon, qreal tolerance) const;

    void invalidate();
    bool isValid() { return mIsValid && mState != nullptr; }

private:
    int constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const;

private:
    SelectionTransformState* mState = nullptr;

    bool mIsValid = false;
    bool mAspectRatioFixed = false;
    bool mLockAxis = false;
};

#endif // SELECTIONTRANSFORMEDITOR_H
