#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectioneditor.h"

#include <QImage>

class BitmapImage;

class SelectionBitmapEditor : public SelectionEditor
{
public:
    explicit SelectionBitmapEditor();
    // SelectionBitmapEditor(SelectionBitmapEditor& editor, BitmapImage* bitmapImage);
    ~SelectionBitmapEditor() override;

    void setSelection(BitmapImage* image, const QRect& rect);
    void setSelection(BitmapImage* image, const QPolygon& polygon);

    void commitChanges() override;
    void discardChanges() override;
    void deleteSelection() override;

    SelectionBitmapState state() const;

    void setDragOrigin(const QPointF& point) override { mDragOrigin = point.toPoint(); }

    // void setSelectionRect(const QRectF& selectionRect) override { mOriginalRect = selectionRect.toAlignedRect(); }
    QPointF getSelectionAnchorPoint() const override;

    // BitmapImage transformedEditor();

    void resetSelectionProperties() override;

    void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement) override;

    // const QRectF mySelectionRect() const override { return mOriginalRect; }

    bool somethingSelected() const override;
    bool isOutsideSelectionArea(const QPointF& point) const override;

    // void setFloatingEditor(BitmapEditor* bitmapEditor);
    // SelectionBitmapImage floatingImage() const { return mFloatingImage; }
    // void setFloatingImage(const QImage& floatingImage, const QRect& bounds);
    // void clearFloatingImage() { mFloatingImage = SelectionBitmapImage(); }

private:
    // SelectionBitmapState& mState;

    /// Creates a copy of the editor based on the selection that was set
    void createImageCache();

    BitmapImage* mBitmapImage = nullptr;

    std::unique_ptr<BitmapImage> mTransformCopyImage;
};

#endif // SELECTIONBITMAPEDITOR_H
