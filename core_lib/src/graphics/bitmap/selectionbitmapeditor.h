#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectiontransformeditor.h"

#include <QImage>

class BitmapImage;

class SelectionBitmapEditor
{
public:
    SelectionBitmapEditor();
    SelectionBitmapEditor(BitmapImage* bitmapImage);
    ~SelectionBitmapEditor();

    void setSelection(const QRect& rect);
    void setSelection(const QPolygon& polygon);

    void setTranslation(const QPointF& point);
    void setRotation(qreal rotationAngle);
    void setScale(qreal scaleX, qreal scaleY);
    void setTransform(const QTransform& transform);

    void translate(const QPointF& point);
    void rotate(qreal rotationAngle, qreal angleIncrement);
    void scale(qreal scaleX, qreal scaleY);
    void scaleAroundAnchorPoint(DragHandle handle, QPointF position);

    QPointF mapToSelection(const QPointF& point) const;

    QPointF mapFromLocalSpace(const QPointF& point) const;

    QPolygonF mapToSelection(const QPolygonF& polygon) const;

    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const;

    void maintainAspectRatio(const bool state);
    void lockMovementToAxis(const bool state);

    QRect selectionRect() const;
    QPolygon selectionPolygon() const;
    qreal rotation() const;
    qreal scaleX() const;
    qreal scaleY() const;
    QPointF translation() const;
    QTransform transform() const;

    void flipSelection(bool flipVertical);

    DragHandle resolveHandleMode(const QPointF& point, qreal selectionTolerance) const;

    QPointF resolveAnchorPoint(const QPointF& currentPoint, const qreal tolerance) const;
    QPointF currentAnchorPoint() const;
    void setTransformAnchor(const QPointF& anchorPoint);

    QPointF alignedPositionToAxis(QPointF currentPoint) const;

    void updateTransformedSelectionState();

    void commitChanges();
    void discardChanges();
    void deleteSelection();

    void calculateSelectionTransformation();

    QPointF getSelectionAnchorPoint() const;

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    void resetTransformation();
    void resetSelectionProperties();

    bool somethingSelected() const;
    bool isSelectionValid() const;
    bool isOutsideSelectionArea(const QPointF& point, qreal tolerance) const;

    void setSmoothTransform(bool smooth);

    bool isValid() const { return mIsValid; }
    void invalidate();
    void invalidateBitmapCache();

    SelectionTransformEditor& editTransformEditor() { return mTransformEditor; }
    const SelectionTransformEditor& transformEditor() const { return mTransformEditor; }

private:
    /// Computes two rectangles, a rectangle for the aligned bounds of the image used to create the image
    /// and a second bound used to allow smooth sub pixel transformation
    ///
    /// @param sourceBounds The bound of the source, ie. the image boundingbox
    /// @param tranform The transform used to transform the image
    /// @param outAlignedRect the bound required to store the transformed image.
    /// @param outPreciseRect the bound required to calculate the sub pixel position of the image
    void computeTransformedImageBounds(const QRect& sourceBounds,
                                       const QTransform& transform,
                                       QRect& outAlignedRect, QRectF& outPreciseRect) const;
    QImage transformedImage(const QImage& src,
                            const QTransform& transform,
                            const QRect& alignedRect,
                            const QRectF& preciseRect,
                            bool smooth) const;

    // When this value is valid, is means that all state should memory wise be intact
    // for example when the Editor has been created with a valid BitmapImage ptr.
    bool mSmoothTransform = true;
    bool mIsValid = false;
    bool mCacheInvalidated = true;
    SelectionTransformEditor mTransformEditor;

    /// Creates a copy of the editor based on the selection that was set
    void createImageCache();

    SelectionBitmapState* mState = nullptr;
    BitmapImage* mBitmapImage = nullptr;
};

#endif // SELECTIONBITMAPEDITOR_H
