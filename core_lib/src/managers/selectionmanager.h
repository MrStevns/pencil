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
#include "perspectivemode.h"
#include "vertexref.h"
#include "vectorselection.h"
#include "layer.h"

#include "selectionbitmapeditor.h"

#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <QTransform>
#include <QVector>

class Editor;

/**
 * @brief The SelectionManager acts as a convenient wrapper to get access the current active selection editor
 * 
 * It is responsible for:
 * - Handling creation of the underlying selection editor for the current frame. Once a SelectionEditor has been created,
 *   prefer using it directly over going through the manager.
 * - Storing shared preferences and values among multiple layers
 *
 * The manager must not own any layer specific state. All state should belong to the respective layers SelectionXState struct.
 */
class SelectionManager : public BaseManager
{
    Q_OBJECT

    struct BitmapEditorEntry {
        int layerID = -1;
        SelectionBitmapEditor bitmapEditor;

        BitmapEditorEntry(int layerID, const SelectionBitmapEditor& bitmapEditor)
        {
            this->layerID = layerID;
            this->bitmapEditor = bitmapEditor;
        }
    };

public:
    explicit SelectionManager(Editor* editor);
    ~SelectionManager() override;

    bool init() override;
    Status load(Object*) override;
    Status save(Object*) override;
    void workingLayerChanged(Layer* workingLayer) override;
    void scrubberChanged(int framePos);

    void createEditor();
    void invalidateEditor();

    SelectionBitmapEditor* activeBitmapEditor();

    void flipSelection(bool flipVertical);
    
    void setSelection(const QRectF& rect);

    void translate(QPointF point);
    void rotate(qreal angle, qreal lockedAngle);
    void scale(qreal sX, qreal sY);
    void maintainAspectRatio(bool state);

    /** @brief Locks movement either horizontally or vertically depending on drag direction
     *  @param state */
    void lockMovementToAxis(bool state);

    bool somethingSelected() const;
    bool isSelectionValid() const;

    void setSelectionTransform(const QTransform& transform);
    void resetSelectionTransform();

    /** @brief SelectionManager::resetSelectionTransformProperties
     * should be used whenever translate, rotate, transform, scale
     * has been applied to a selection, but don't want to reset size nor position
     */
    void resetSelectionTransformProperties();

    void resetSelectionProperties();
    void deleteSelection();

    qreal selectionTolerance() const;

    QPointF currentTransformAnchor() const;
    QPointF getSelectionAnchorPoint() const;

    void setTransformAnchor(const QPointF& point);

    DragHandle resolveHandleMode(const QPointF& point, qreal tolerance) const;

    QRectF mySelectionRect() const;
    qreal myRotation() const;
    qreal myScaleX() const;
    qreal myScaleY() const;
    QPointF myTranslation() const;
    QTransform selectionTransform() const;

    void setTranslation(const QPointF& translation);
    void setRotation(qreal rotation);
    void setScale(qreal scaleX, qreal scaleY);
    void setSmoothTransform(bool smoothTransform);

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    QPointF mapToSelection(const QPointF& point) const;
    QPointF mapFromLocalSpace(const QPointF& point) const;
    QPolygonF mapToSelection(const QPolygonF& polygon) const;
    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const;

    QPolygonF getSelectionPolygon() const;

    /// This should be called to update the selection transform
    void calculateSelectionTransformation();

    bool isOutsideSelectionArea(const QPointF& point, qreal tolerance) const;

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
    SelectionBitmapEditor* findActiveEditor(int layerId, KeyFrame* keyFrame);
    void setActiveEditor(int layerId, int framePos);

    QList<int> mClosestCurves;
    QList<VertexRef> mClosestVertices;

    qreal mSelectionTolerance = 10.0;

    QVector<BitmapEditorEntry> mBitmapEditors;

    // TODO: implement
    // SelectionVectorEditor vectorSelection;
    SelectionBitmapEditor mNullBitmapEditor;
    SelectionBitmapEditor* mActiveBitmapEditor = nullptr;

    Layer* mWorkingLayer = nullptr;
};

#endif // SELECTIONMANAGER_H
