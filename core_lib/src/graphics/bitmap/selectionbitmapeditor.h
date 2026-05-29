#ifndef SELECTIONBITMAPEDITOR_H
#define SELECTIONBITMAPEDITOR_H

#include "selectiontransformeditor.h"
#include "selectionbitmapstate.h"
#include "selectionpatchsystem.h"

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

    bool belongsTo(int keyPos) const;

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

    void onPatchReady(int jobId);

    // Connect to this to trigger repaints when patch is ready
    static SelectionPatchSystem& patchSystem() {
        static SelectionPatchSystem system;
        return system;
    }

private:

    // When this value is valid, is means that all state should memory wise be intact
    // for example when the Editor has been created with a valid BitmapImage ptr.
    bool mIsValid = false;
    bool mCacheInvalidated = true;

    SelectionTransformEditor mTransformEditor;

    /// Creates a copy of the editor based on the selection that was set
    void createImageCache();

    int mPatchJobId = -1;
    SelectionBitmapState* mState = nullptr;
    BitmapImage* mBitmapImage = nullptr;
};

#endif // SELECTIONBITMAPEDITOR_H
