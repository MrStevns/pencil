#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectioneditor.h"

#include <QImage>

class BitmapImage;

class SelectionBitmapEditor
{
public:
    explicit SelectionBitmapEditor();
    // SelectionBitmapEditor(SelectionBitmapEditor& editor, BitmapImage* bitmapImage);
    ~SelectionBitmapEditor();

    void setSelection(BitmapImage* image, const QRect& rect);
    void setSelection(BitmapImage* image, const QPolygon& polygon);

    void setTranslation(const QPointF& point);
    void setRotation(qreal rotationAngle);
    void setScale(qreal scaleX, qreal scaleY);

    void translate(const QPointF& point);
    void rotate(qreal rotationAngle, qreal lockedAngle);
    void scale(qreal scaleX, qreal scaleY);

    QPointF mapToSelection(const QPointF& point) const;

    QPointF mapFromLocalSpace(const QPointF& point) const;

    QPolygonF mapToSelection(const QPolygonF& polygon) const;

    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const;

    QRect mySelectionRect() const;
    qreal myRotation() const;
    qreal myScaleX() const;
    qreal myScaleY() const;
    QPointF myTranslation() const;

    void flipSelection(bool flipVertical);

    void setMoveMode(MoveMode mode);
    void setTransformAnchor(const QPointF& anchorPoint);

    QPointF alignedPositionToAxis(QPointF currentPoint) const;

    void commitChanges();
    void discardChanges();
    void deleteSelection();

    void calculateSelectionTransformation();

    SelectionBitmapState state() const;

    // void setDragOrigin(const QPointF& point) { mDragOrigin = point.toPoint(); }

    // void setSelectionRect(const QRectF& selectionRect) { mOriginalRect = selectionRect.toAlignedRect(); }
    QPointF getSelectionAnchorPoint() const;

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    // BitmapImage transformedEditor();

    void resetSelectionProperties();

    void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement);

    // const QRectF mySelectionRect() const { return mOriginalRect; }

    bool somethingSelected() const;
    bool isOutsideSelectionArea(const QPointF& point) const;

    // void setFloatingEditor(BitmapEditor* bitmapEditor);
    // SelectionBitmapImage floatingImage() const { return mFloatingImage; }
    // void setFloatingImage(const QImage& floatingImage, const QRect& bounds);
    // void clearFloatingImage() { mFloatingImage = SelectionBitmapImage(); }

private:
    // SelectionBitmapState& mState;
    bool mCacheInvalidated = true;
    SelectionEditor commonEditor;

    /// Creates a copy of the editor based on the selection that was set
    void createImageCache();

    BitmapImage* mBitmapImage = nullptr;

    std::unique_ptr<BitmapImage> mTransformCopyImage;
};

#endif // SELECTIONBITMAPEDITOR_H
