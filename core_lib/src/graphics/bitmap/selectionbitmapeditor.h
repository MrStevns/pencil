#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectioneditor.h"

class SelectionBitmapEditor : public SelectionEditor
{
public:
    explicit SelectionBitmapEditor();
    ~SelectionBitmapEditor() override;

    void setSelection(const QRectF& rect) override;

    void commitChanges(KeyFrame* keyframe) override;
    void discardChanges(KeyFrame* keyframe) override;
    void deleteSelection() override;

    void setDragOrigin(const QPointF& point) override { mDragOrigin = point.toPoint(); }

    void setSelectionRect(const QRectF& selectionRect) override { mOriginalRect = selectionRect.toAlignedRect(); }
    QPointF getSelectionAnchorPoint() const override;

    void resetSelectionProperties() override;
    void setMoveModeForAnchorInRange(const QPointF& point) override;

    void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement) override;

    const QRectF mySelectionRect() const override { return mOriginalRect; }

    bool somethingSelected() const override { return mOriginalRect.isValid(); }
    bool isOutsideSelectionArea(const QPointF& point) const override;

private:
    QPolygon mSelectionPolygon;
    QRect mOriginalRect;
};

#endif // SELECTIONBITMAPEDITOR_H
