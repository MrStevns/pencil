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
#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include "basemanager.h"
#include "movemode.h"
#include "vertexref.h"
#include "vectorselection.h"

#include "selectionbitmapeditor.h"

#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QTransform>

class Editor;

class SelectionManager : public BaseManager
{
    Q_OBJECT
public:
    explicit SelectionManager(Editor* editor);
    ~SelectionManager() override;

    bool init() override;
    Status load(Object*) override;
    Status save(Object*) override;
    void workingLayerChanged(Layer*workingLayer) override;

    void flipSelection(bool flipVertical);

    void setSelection(const QRectF& rect);

    void translate(QPointF point);
    void rotate(qreal angle, qreal lockedAngle);
    void scale(qreal sX, qreal sY);
    void maintainAspectRatio(bool state);

    /** @brief Locks movement either horizontally or vertically depending on drag direction
     *  @param state */
    void lockMovementToAxis(bool state);

    void setMoveModeForAnchorInRange(const QPointF& point);

    MoveMode getMoveMode() const;
    void setMoveMode(const MoveMode moveMode);

    bool somethingSelected() const;

    void adjustSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement = 0);

    void setSelectionTransform(const QTransform& transform);
    void resetSelectionTransform();

    /** @brief SelectionManager::resetSelectionTransformProperties
     * should be used whenever translate, rotate, transform, scale
     * has been applied to a selection, but don't want to reset size nor position
     */
    void resetSelectionTransformProperties();

    void resetSelectionProperties();
    void deleteSelection();

    bool isOutsideSelectionArea(const QPointF& point) const;

    qreal selectionTolerance() const;

    QPointF currentTransformAnchor() const;
    QPointF getSelectionAnchorPoint() const;

    void setTransformAnchor(const QPointF& point);

    MoveMode resolveMoveModeForPoint(const QPointF& point) const;

    QRectF mySelectionRect() const;
    qreal myRotation() const;
    qreal myScaleX() const;
    qreal myScaleY() const;
    QPointF myTranslation() const;
    QTransform selectionTransform() const;

    void setTranslation(const QPointF& translation);
    void setRotation(qreal rotation);
    void setScale(qreal scaleX, qreal scaleY);

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    QPointF mapToSelection(const QPointF& point) const;
    QPointF mapFromLocalSpace(const QPointF& point) const;
    QPolygonF mapToSelection(const QPolygonF& polygon) const;
    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const;

    QPolygonF getSelectionPolygon() const;

    /// The point from where the dragging will be based of inside the selection area.
    /// Not to be confused with the selection origin
    void setDragOrigin(const QPointF& point);

    /// This should be called to update the selection transform
    void calculateSelectionTransformation();

    // SelectionBitmapEditor bitmapEditor() { return bitmapSelection; }
    // SelectionVectorEditor vectorEditor() { return vectorSelection; }

    // Vector methods
    VectorSelection vectorSelection;

    void setCurves(const QList<int>& curves) { mClosestCurves = curves; }
    void setVertices(const QList<VertexRef>& vertices) { mClosestVertices = vertices; }

    void clearCurves() { mClosestCurves.clear(); }
    void clearVertices() { mClosestVertices.clear(); }

    const QList<int> closestCurves() const { return mClosestCurves; }
    const QList<VertexRef> closestVertices() const { return mClosestVertices; }

signals:
    void selectionChanged();
    void selectionReset();
    void needDeleteSelection();

private:
    QList<int> mClosestCurves;
    QList<VertexRef> mClosestVertices;

    qreal mSelectionTolerance = 10.0;

    // TODO: implement
    // SelectionVectorEditor vectorSelection;
    SelectionBitmapEditor bitmapSelection;

    Layer* mWorkingLayer = nullptr;
};

#endif // SELECTIONMANAGER_H
