#ifndef SELECTIONVECTOREDITOR_H
#define SELECTIONVECTOREDITOR_H

#include "selectioneditor.h"

class SelectionVectorEditor : public SelectionEditor
{
    explicit SelectionVectorEditor();
    ~SelectionVectorEditor() override;

    void setSelection(const QRectF& rect) override;

    void commitChanges() override;
    void discardChanges() override;
    void deleteSelection() override;

    void setDragOrigin(const QPointF& point) override { mDragOrigin = point; }

    void resetSelectionProperties() override;
    void setMoveModeForAnchorInRange(const QPointF& point) override;

    void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement) override;

    void setSelectionRect(const QRectF&) override { /* Note: We can't modify the rect of a vector selection directly*/ }
    QPointF getSelectionAnchorPoint() const override;

    const QRectF mySelectionRect() const override;

    bool somethingSelected() const override { return false; /* TODO: Should check whether the vectorImage has any selections;*/ }
    bool isOutsideSelectionArea(const QPointF& point) const override;

private:
    QPolygonF mSelectionPolygon;

    QList<int> mClosestCurves;
    QList<VertexRef> mClosestVertices;
};

#endif // SELECTIONVECTOREDITOR_H
