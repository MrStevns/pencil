#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectioneditor.h"

#include <QImage>

class BitmapEditor;

class SelectionBitmapEditor : public SelectionEditor
{
public:
    explicit SelectionBitmapEditor(BitmapEditor* bitmapEditor);
    SelectionBitmapEditor(SelectionBitmapEditor& editor, BitmapEditor* bitmapEditor);
    ~SelectionBitmapEditor() override;

    void setSelection(const QRectF& rect) override;

    void commitChanges() override;
    void discardChanges() override;
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

    // void setFloatingEditor(BitmapEditor* bitmapEditor);
    // SelectionBitmapImage floatingImage() const { return mFloatingImage; }
    // void setFloatingImage(const QImage& floatingImage, const QRect& bounds);
    // void clearFloatingImage() { mFloatingImage = SelectionBitmapImage(); }

private:
    QPolygon mSelectionPolygon;
    QRect mOriginalRect;

    BitmapEditor* mBitmapEditor = nullptr;
};

#endif // SELECTIONBITMAPEDITOR_H
