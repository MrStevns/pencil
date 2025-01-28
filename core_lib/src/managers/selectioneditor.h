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

#include "basemanager.h"
#include "movemode.h"
#include "vertexref.h"
#include "vectorselection.h"

#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QTransform>

class Editor;
class KeyFrame;

class SelectionEditor
{
public:

    explicit SelectionEditor();
    SelectionEditor(SelectionEditor& editor);
    virtual ~SelectionEditor();

    void flipSelection(bool flipVertical);

    virtual void setSelection(const QRectF& rect);
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
    virtual const QRectF mySelectionRect() const = 0;
    virtual bool somethingSelected() const = 0;

    virtual void setMoveModeForAnchorInRange(const QPointF& point) = 0;

    virtual void resetSelectionProperties();
    virtual bool isOutsideSelectionArea(const QPointF& point) const = 0;

    virtual void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement) = 0;

    virtual void deleteSelection() = 0;

    virtual void commitChanges() = 0;
    virtual void discardChanges() = 0;

    QPointF currentTransformAnchor() const { return mAnchorPoint; }

    void translate(QPointF point);
    void rotate(qreal angle, qreal lockedAngle);
    void scale(qreal sX, qreal sY);
    void maintainAspectRatio(bool state) { mAspectRatioFixed = state; }

    /** @brief Locks movement either horizontally or vertically depending on drag direction
     *  @param state */
    void alignPositionToAxis(bool state) { mLockAxis = state; }

    MoveMode getMoveMode() const { return mMoveMode; }
    void setMoveMode(const MoveMode moveMode) { mMoveMode = moveMode; }

    QTransform selectionTransform() const { return mSelectionTransform; }
    void setSelectionTransform(const QTransform& transform) { mSelectionTransform = transform; }
    void resetSelectionTransform();

    qreal selectionTolerance() const;

    void setTransformAnchor(const QPointF& point);

    const qreal& myRotation() const { return mRotatedAngle; }
    const qreal& myScaleX() const { return mScaleX; }
    const qreal& myScaleY() const { return mScaleY; }
    const QPointF& myTranslation() const { return mTranslation; }

    void setRotation(const qreal& rotation) { mRotatedAngle = rotation; }
    void setScale(const qreal scaleX, const qreal scaleY) { mScaleX = scaleX; mScaleY = scaleY; }
    void setTranslation(const QPointF& translation) { mTranslation = translation; }
    virtual void setSelectionRect(const QRectF& selectionRect) = 0;

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    QPointF mapToSelection(const QPointF& point) const { return mSelectionTransform.map(point); }
    QPointF mapFromLocalSpace(const QPointF& point) const { return mSelectionTransform.inverted().map(point); }
    QPolygonF mapToSelection(const QPolygonF& polygon) const { return mSelectionTransform.map(polygon); }
    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const { return mSelectionTransform.inverted().map(polygon); }

    // // Vector selection
    // VectorSelection vectorSelection;

    // void setCurves(const QList<int>& curves) { mClosestCurves = curves; }
    // void setVertices(const QList<VertexRef>& vertices) { mClosestVertices = vertices; }

    // void clearCurves() { mClosestCurves.clear(); };
    // void clearVertices() { mClosestVertices.clear(); };

    // const QList<int> closestCurves() const { return mClosestCurves; }
    // const QList<VertexRef> closestVertices() const { return mClosestVertices; }

    /// This should be called to update the selection transform
    void calculateSelectionTransformation();

    void cleanupCallbacks();

    typedef std::function<void()> SelectionChangedCallback;
    SelectionChangedCallback onSelectionChanged;

    typedef std::function<void()> SelectionResetCallback;
    SelectionResetCallback onSelectionReset;

protected:
    void setMoveModeForAnchorInRange(const QPolygonF& selectionPolygon, const QPointF& point);
    void adjustCurrentSelection(const QPolygonF& selectionPolygon, const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement);

    QPointF getSelectionAnchorPoint(const QPolygonF& selectionPolygon) const;

    MoveMode mMoveMode = MoveMode::NONE;
    QTransform mSelectionTransform;
    QPointF mDragOrigin;

private:
    /** @brief Aligns the input position to the nearest axis.
     *  Eg. draggin along the x axis, will keep the selection to that axis.
     * @param currentPosition the position of the cursor
     * @return A point that is either horizontally or vertically aligned with the current position.
     */
    QPointF alignPositionToAxis(QPointF currentPoint) const;
    int constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const;

private:

    qreal mScaleX;
    qreal mScaleY;
    QPointF mTranslation;
    qreal mRotatedAngle = 0.0;
    QPointF mAnchorPoint;

    bool mAspectRatioFixed = false;
    bool mLockAxis = false;
    // QPolygonF mSelectionPolygon;
    // QRectF mOriginalRect;

    // QList<int> mClosestCurves;
    // QList<VertexRef> mClosestVertices;

    // QPointF mDragOrigin;

    const qreal mSelectionTolerance = 10.0;

    // QPointF mAnchorPoint;
};

#endif // SELECTIONEDITOR_H
