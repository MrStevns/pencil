#include "selectionbitmapeditor.h"

#include "bitmapimage.h"

#include <QRectF>
#include <QDebug>
#include <QImage>

SelectionBitmapEditor::SelectionBitmapEditor()
{
    mIsValid = false;
}

SelectionBitmapEditor::SelectionBitmapEditor(BitmapImage* bitmapImage)
{
    if (!bitmapImage) {
        invalidate(); return;
    }

    mBitmapImage = bitmapImage;
    mState = &mBitmapImage->mSelectionState;
    mTransformEditor = SelectionTransformEditor(&mBitmapImage->mSelectionState.transformState);
    mIsValid = true;
}

SelectionBitmapEditor::~SelectionBitmapEditor()
{
    qDebug() << "deinit SelectionBitmapEditor";
    invalidate();
}

void SelectionBitmapEditor::invalidate()
{
    invalidateBitmapCache();
    if (mBitmapImage) {
        mBitmapImage->mSelectionState = SelectionBitmapState();
        mBitmapImage = nullptr;
    }
    mState = nullptr;
    mTransformEditor.invalidate();
    mIsValid = false;
}

void SelectionBitmapEditor::invalidateBitmapCache()
{
    if (!mState || mState->transformedImage.isNull()) { return; }

    mState->transformedImage = QImage();
    mState->baseImageCache = QImage();
    mCacheInvalidated = true;
}

void SelectionBitmapEditor::setSelection(const QRect& rect)
{
    if (!mIsValid) { return; }

    setSelection(QPolygon(rect));
}

void SelectionBitmapEditor::setSelection(const QPolygon& polygon)
{
    mState->selectionGeometry = polygon;

    // Polygon bounds are geometric, where as image bounds are raster-based.
    // As such we adjust the bounds.
    mState->selectionImageBounds = polygon.boundingRect().adjusted(0, 0,-1,-1);
    mState->baseImageBounds = mState->selectionImageBounds;

    mTransformEditor.setTransformAnchor(mTransformEditor.resolveAnchorPoint(QPoint(), mState->selectionGeometry, 0));

    createImageCache();
}

QRect SelectionBitmapEditor::selectionRect() const
{
    if (!mIsValid) { return QRect(); }
    return mState->selectionImageBounds;
}

QPolygon SelectionBitmapEditor::selectionPolygon() const
{
    if (!mIsValid) { return QPolygon(); }

    return mState->selectionGeometry;
}

qreal SelectionBitmapEditor::rotation() const
{
    if (!mIsValid) { return 0; }
    return mTransformEditor.rotationAngle();
}

qreal SelectionBitmapEditor::scaleX() const
{
    if (!mIsValid) { return 1; }
    return mTransformEditor.scaleX();
}

qreal SelectionBitmapEditor::scaleY() const
{
    if (!mIsValid) { return 1; }
    return mTransformEditor.scaleY();
}

QPointF SelectionBitmapEditor::translation() const
{
    if (!mIsValid) { return QPointF(); }

    return mTransformEditor.translation().toPoint();
}

QTransform SelectionBitmapEditor::transform() const
{
    if (!mIsValid) { return QTransform(); }
    return mTransformEditor.transform();
}

void SelectionBitmapEditor::scaleAroundAnchorPoint(DragHandle handle, QPointF position)
{
    if (!mIsValid) { return; }
    mTransformEditor.scaleAroundAnchorPoint(handle, mState->selectionGeometry, position);
}

void SelectionBitmapEditor::setTranslation(const QPointF& point)
{
    if (!mIsValid) { return; }
    mTransformEditor.setTranslation(point);
}

void SelectionBitmapEditor::setRotation(qreal rotationAngle)
{
    if (!mIsValid) { return; }
    mTransformEditor.setRotation(rotationAngle);
}

void SelectionBitmapEditor::setScale(qreal scaleX, qreal scaleY)
{
    if (!mIsValid) { return; }
    mTransformEditor.setScale(scaleX, scaleY);
}

void SelectionBitmapEditor::setTransform(const QTransform& transform)
{
    if (!mIsValid) { return; }
    mTransformEditor.setTransform(transform);
}

void SelectionBitmapEditor::setSmoothTransform(bool smooth)
{
    if (!mIsValid) { return; }
    mState->smoothTransform = smooth;

    updateTransformedSelectionState();
}

bool SelectionBitmapEditor::belongsTo(int keyPos) const
{
    if (!mIsValid) { return false; }

    return keyPos == mBitmapImage->pos();
}

DragHandle SelectionBitmapEditor::resolveHandleMode(const QPointF& point, qreal selectionTolerance) const
{
    if (!mIsValid) { return DragHandle::NONE; }
    return mTransformEditor.resolveHandleMode(point, mState->selectionGeometry, selectionTolerance);
}

QPointF SelectionBitmapEditor::currentAnchorPoint() const
{
    if (!mIsValid) { return QPointF(); }
    return mTransformEditor.currentAnchorPoint();
}

void SelectionBitmapEditor::setTransformAnchor(const QPointF& anchorPoint)
{
    if (!mIsValid) { return; }
    mTransformEditor.setTransformAnchor(anchorPoint);
}

void SelectionBitmapEditor::translate(const QPointF& point)
{
    if (!mIsValid) { return; }
    mTransformEditor.translate(point.toPoint());
}

void SelectionBitmapEditor::rotate(qreal rotationAngle, qreal angleIncrement)
{
    if (!mIsValid) { return; }
    mTransformEditor.rotate(rotationAngle, angleIncrement);
}

void SelectionBitmapEditor::scale(qreal scaleX, qreal scaleY)
{
    if (!mIsValid) { return; }
    mTransformEditor.scale(scaleX, scaleY);
}

QPointF SelectionBitmapEditor::mapToSelection(const QPointF& point) const
{
    if (!mIsValid) { return QPointF(); }
    return mTransformEditor.mapToSelection(point);
}

QPointF SelectionBitmapEditor::mapFromLocalSpace(const QPointF& point) const
{
    if (!mIsValid) { return QPointF(); }
    return mTransformEditor.mapFromLocalSpace(point);
}

QPolygonF SelectionBitmapEditor::mapToSelection(const QPolygonF& polygon) const
{
    if (!mIsValid) { return QPolygonF(); }
    return mTransformEditor.mapToSelection(polygon);
}

QPolygonF SelectionBitmapEditor::mapFromLocalSpace(const QPolygonF& polygon) const
{
    if (!mIsValid) { return QPolygonF(); }
    return mTransformEditor.mapFromLocalSpace(polygon);
}

void SelectionBitmapEditor::calculateSelectionTransformation()
{
    if (!mIsValid) { return; }
    mTransformEditor.calculateSelectionTransformation();
    updateTransformedSelectionState();
}

qreal SelectionBitmapEditor::angleFromPoint(const QPointF &point, const QPointF &anchorPoint) const
{
    if (!mIsValid) { return 0; }
    return mTransformEditor.angleFromPoint(point, anchorPoint);
}

void SelectionBitmapEditor::lockMovementToAxis(const bool state)
{
    if (!mIsValid) { return; }
    mTransformEditor.lockMovementToAxis(state);
}

QPointF SelectionBitmapEditor::alignedPositionToAxis(QPointF currentPoint) const
{
    if (!mIsValid) { return QPointF(); }
    return mTransformEditor.alignedPositionToAxis(currentPoint);
}

void SelectionBitmapEditor::maintainAspectRatio(const bool state)
{
    if (!mIsValid) { return; }
    mTransformEditor.maintainAspectRatio(state);
}

void SelectionBitmapEditor::createImageCache()
{
    if (!mIsValid) { return; }

    // Make sure the selection is valid before creating a cache
    if (!mState->selectionImageBounds.isValid()) {
        return;
    }

    if (!mCacheInvalidated) {
        invalidateBitmapCache();
    }
    mState->baseImageCache = *mBitmapImage->copy(mState->selectionImageBounds, mState->selectionGeometry).image();
    mCacheInvalidated = false;

    updateTransformedSelectionState();
}

void SelectionBitmapEditor::resetTransformation()
{
    if (!mIsValid) { return; }
    mTransformEditor.resetState();
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    if (!mIsValid) { return; }

    invalidate();
}

bool SelectionBitmapEditor::somethingSelected() const
{
    if (!mIsValid) { return false; }

    return mState->selectionGeometry.count() > 0;
}

bool SelectionBitmapEditor::isSelectionValid() const
{
    if (!mIsValid) { return false; }

    return somethingSelected() && (mState->selectionImageBounds.width() >= 1 && mState->selectionImageBounds.height() >= 1);
}

void SelectionBitmapEditor::flipSelection(bool flipVertical)
{
    if (!mIsValid) { return; }
    mTransformEditor.flipSelection(flipVertical);
}

void SelectionBitmapEditor::discardChanges()
{
    resetSelectionProperties();
}

void SelectionBitmapEditor::deleteSelection()
{
    if (!mIsValid) { return; }
    if (somethingSelected())
    {
        mBitmapImage->clear(mState->selectionImageBounds);
    }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    if (!mIsValid) { return QPointF(); }
    if (!somethingSelected()) { return QPointF(); }

    return mTransformEditor.currentAnchorPoint();
}

QPointF SelectionBitmapEditor::resolveAnchorPoint(const QPointF& currentPoint, const qreal tolerance) const
{
    if (!mIsValid) { return QPointF(); }
    return mTransformEditor.resolveAnchorPoint(currentPoint, mState->selectionGeometry, tolerance);
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point, qreal tolerance) const
{
    if (!mIsValid) { return true; }
    if (!somethingSelected()) { return true; }

    return mTransformEditor.isOutsideSelection(point, mState->selectionGeometry, tolerance);
}

void SelectionBitmapEditor::computeTransformedImageBounds(const QRect& sourceBounds,
                                   const QTransform& transform,
                                   QRect& outAlignedRect, QRectF& outPreciseRect) const
{
    QPolygon boundsPolygon(sourceBounds);

    QRectF boundingRect = transform.map(boundsPolygon).boundingRect();

    outPreciseRect = transform.map(QPolygonF(boundsPolygon)).boundingRect();

    outAlignedRect = QRect(
        qFloor(boundingRect.x()),
        qFloor(boundingRect.y()),
        qCeil(boundingRect.width()),
        qCeil(boundingRect.height())
    );
}


void SelectionBitmapEditor::updateTransformedSelectionState()
{
    if (!mIsValid) { return; }
    const QRect& originalBounds = mState->selectionImageBounds;

    QRect outAlignedRect;
    QRectF outPreciseRect;
    QTransform transform = mState->transformState.selectionTransform;

    if (mCacheInvalidated) {
        createImageCache();
    }

    computeTransformedImageBounds(originalBounds, transform, outAlignedRect, outPreciseRect);

    mState->transformedImage = transformedImage(mState->baseImageCache, transform, outAlignedRect, outPreciseRect, mState->smoothTransform);
    int padding = mState->boundsPadding * 0.5;
    mState->transformedImageBounds = outAlignedRect.adjusted(-padding,
                                                              -padding,
                                                              padding,
                                                              padding);
}

QImage SelectionBitmapEditor::transformedImage(const QImage& src,
                                               const QTransform& transform,
                                               const QRect& alignedRect,
                                               const QRectF& preciseRect,
                                               bool smooth) const
{
    int padding = mState->boundsPadding;

    QImage result(QSize(alignedRect.width() + padding, alignedRect.height() + padding),
                  QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    if (smooth) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);
    }

    QPointF preciseCenter(preciseRect.width() * 0.5, preciseRect.height() * 0.5);

    painter.translate(padding * 0.5, padding * 0.5);
    painter.setTransform(transform, true);

    // Calculates the sub pixel position offset in order to account for the image being integer based.
    QPointF pixelCorrectionOffset = preciseRect.topLeft() - alignedRect.topLeft();
    QPointF centerInSource = transform.inverted().map(preciseCenter + pixelCorrectionOffset);
    QPointF copiedCenter(src.width() * 0.5, src.height() * 0.5);
    QPointF drawPoint = centerInSource - copiedCenter;

    painter.drawImage(drawPoint, src);
    painter.end();

    return result;
}
