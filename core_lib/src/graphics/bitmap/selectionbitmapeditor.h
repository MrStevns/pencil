#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectioneditor.h"

#include <QImage>

class BitmapImage;

class SelectionBitmapEditor
{
public:
    SelectionBitmapEditor();
    SelectionBitmapEditor(BitmapImage* bitmapImage);
    // SelectionBitmapEditor(SelectionBitmapEditor& editor, BitmapImage* bitmapImage);
    ~SelectionBitmapEditor();

    void setSelection(const QRect& rect);
    void setSelection(const QPolygon& polygon);

    void setTranslation(const QPointF& point);
    void setRotation(qreal rotationAngle);
    void setScale(qreal scaleX, qreal scaleY);
    void setTransform(const QTransform& transform);

    void translate(const QPointF& point);
    void rotate(qreal rotationAngle, qreal lockedAngle);
    void scale(qreal scaleX, qreal scaleY);

    QPointF mapToSelection(const QPointF& point) const;

    QPointF mapFromLocalSpace(const QPointF& point) const;

    QPolygonF mapToSelection(const QPolygonF& polygon) const;

    QPolygonF mapFromLocalSpace(const QPolygonF& polygon) const;

    void maintainAspectRatio(const bool state);
    void lockMovementToAxis(const bool state);

    QRect mySelectionRect() const;
    QPolygon mySelectionPolygon() const;
    qreal myRotation() const;
    qreal myScaleX() const;
    qreal myScaleY() const;
    QPointF myTranslation() const;
    QTransform myTransform() const;

    void flipSelection(bool flipVertical);

    void setMoveMode(MoveMode mode);
    MoveMode moveMode() const;
    MoveMode resolveMoveModeForAnchorInRange(const QPointF& point, qreal selectionTolerance) const;

    QPointF currentAnchorPoint() const;
    void setTransformAnchor(const QPointF& anchorPoint);

    QPointF alignedPositionToAxis(QPointF currentPoint) const;

    void commitChanges();
    void discardChanges();
    void deleteSelection();

    void calculateSelectionTransformation();

    void setDragOrigin(const QPointF& point);

    QPointF getSelectionAnchorPoint() const;

    qreal angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const;

    // BitmapImage transformedEditor();

    void resetTransformation();
    void resetSelectionProperties();

    void adjustCurrentSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement);

    bool somethingSelected() const;
    bool isOutsideSelectionArea(const QPointF& point) const;

    bool isValid() { return mIsValid; }
    void invalidate();

private:
    bool mIsValid = false;
    bool mCacheInvalidated = true;
    SelectionEditor mCommonEditor;

    /// Creates a copy of the editor based on the selection that was set
    void createImageCache();

    SelectionBitmapState* mState = nullptr;
    BitmapImage* mBitmapImage = nullptr;

    // std::unique_ptr<BitmapImage> mTransformCopyImage;
};

#endif // SELECTIONBITMAPEDITOR_H
