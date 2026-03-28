#include "selectionbitmapeditor.h"

#include "bitmapimage.h"
#include "tile.h"

#include <QRectF>
#include <QDebug>
#include <QImage>
#include <QPainterPath>

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
    mCommonEditor = SelectionEditor(&mBitmapImage->mSelectionState.commonState);
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
    mCommonEditor.invalidate();
    mIsValid = false;
}

void SelectionBitmapEditor::invalidateBitmapCache()
{
    if (!mState || mState->transformedImage.isNull()) { return; }

    mState->transformedImage = QImage();
    mState->selectionImage = QImage();
    mCacheInvalidated = true;
}

void SelectionBitmapEditor::setSelection(const QRect& rect)
{
    setSelection(QPolygon(rect));
}

void SelectionBitmapEditor::setSelection(const QPolygon& polygon)
{
    mState->selectionPolygon = polygon;
    mState->clipPolygon = polygon;

    // QRect's right() and bottom() are slightly different from QRectF,
    // because they always return left+width-1
    // and top+height-1 for historical reasons.
    // as such in order to get the same bound, we need to subtract from the right and bottom
    mState->selectionRect = polygon.boundingRect().adjusted(0, 0,-1,-1);
    mState->originalRect = mState->selectionRect;

    qDebug() << "set Selection";

    createImageCache();
}

QRect SelectionBitmapEditor::mySelectionRect() const
{
    if (!mIsValid) { return QRect(); }
    return mState->selectionRect;
}

QPolygonF SelectionBitmapEditor::mySelectionPolygon() const
{
    if (!mIsValid) { return QPolygon(); }

    return mState->selectionPolygon;
}

qreal SelectionBitmapEditor::myRotation() const
{
    if (!mIsValid) { return 0; }
    return mCommonEditor.myRotation();
}

qreal SelectionBitmapEditor::myScaleX() const
{
    if (!mIsValid) { return 1; }
    return mCommonEditor.myScaleX();
}

qreal SelectionBitmapEditor::myScaleY() const
{
    if (!mIsValid) { return 1; }
    return mCommonEditor.myScaleY();
}

QPointF SelectionBitmapEditor::myTranslation() const
{
    if (!mIsValid) { return QPointF(); }

    return mCommonEditor.myTranslation().toPoint();
}

QTransform SelectionBitmapEditor::myTransform() const
{
    if (!mIsValid) { return QTransform(); }
    return mCommonEditor.myTransform();
}

void SelectionBitmapEditor::setTranslation(const QPointF& point)
{
    if (!mIsValid) { return; }
    mCommonEditor.setTranslation(point);
}

void SelectionBitmapEditor::setRotation(qreal rotationAngle)
{
    if (!mIsValid) { return; }
    mCommonEditor.setRotation(rotationAngle);
}

void SelectionBitmapEditor::setScale(qreal scaleX, qreal scaleY)
{
    if (!mIsValid) { return; }
    mCommonEditor.setScale(scaleX, scaleY);
}

void SelectionBitmapEditor::setTransform(const QTransform& transform)
{
    if (!mIsValid) { return; }
    mCommonEditor.setTransform(transform);
}

void SelectionBitmapEditor::setSmoothTransform(bool smooth)
{
    if (!mIsValid) { return; }
    mSmoothTransform = smooth;

    // updateTransformedSelectionState();
}

MoveMode SelectionBitmapEditor::moveMode() const
{
    if (!mIsValid) { return MoveMode::NONE; }
    return mCommonEditor.getMoveMode();
}

void SelectionBitmapEditor::setMoveMode(MoveMode mode)
{
    if (!mIsValid) { return; }
    mCommonEditor.setMoveMode(mode);
}

MoveMode SelectionBitmapEditor::resolveMoveModeForAnchorInRange(const QPointF& point, qreal selectionTolerance) const
{
    if (!mIsValid) { return MoveMode::NONE; }
    return mCommonEditor.resolveMoveModeForAnchorInRange(point, mState->selectionPolygon, selectionTolerance);
}

void SelectionBitmapEditor::setDragOrigin(const QPointF& point)
{
    if (!mIsValid) { return; }
    mCommonEditor.setDragOrigin(point.toPoint());
}

QPointF SelectionBitmapEditor::currentAnchorPoint() const
{
    if (!mIsValid) { return QPointF(); }
    return mCommonEditor.currentAnchorPoint().toPoint();
}

void SelectionBitmapEditor::setTransformAnchor(const QPointF& anchorPoint)
{
    if (!mIsValid) { return; }
    mCommonEditor.setTransformAnchor(anchorPoint.toPoint());
}

void SelectionBitmapEditor::translate(const QPointF& point)
{
    if (!mIsValid) { return; }
    mCommonEditor.translate(point.toPoint());
}

void SelectionBitmapEditor::rotate(qreal rotationAngle, qreal lockedAngle)
{
    if (!mIsValid) { return; }
    mCommonEditor.rotate(rotationAngle, lockedAngle);
}

void SelectionBitmapEditor::scale(qreal scaleX, qreal scaleY)
{
    if (!mIsValid) { return; }
    mCommonEditor.scale(scaleX, scaleY);
}

QPointF SelectionBitmapEditor::mapToSelection(const QPointF& point) const
{
    if (!mIsValid) { return QPointF(); }
    return mCommonEditor.mapToSelection(point);
}

QPointF SelectionBitmapEditor::mapFromLocalSpace(const QPointF& point) const
{
    if (!mIsValid) { return QPointF(); }
    return mCommonEditor.mapFromLocalSpace(point);
}

QPolygonF SelectionBitmapEditor::mapToSelection(const QPolygonF& polygon) const
{
    if (!mIsValid) { return QPolygonF(); }
    return mCommonEditor.mapToSelection(polygon);
}

QPolygonF SelectionBitmapEditor::mapFromLocalSpace(const QPolygonF& polygon) const
{
    if (!mIsValid) { return QPolygonF(); }
    return mCommonEditor.mapFromLocalSpace(polygon);
}

void SelectionBitmapEditor::calculateSelectionTransformation()
{
    if (!mIsValid) { return; }
    mCommonEditor.calculateSelectionTransformation();
    updateTransformedSelectionState();
}

qreal SelectionBitmapEditor::angleFromPoint(const QPointF &point, const QPointF &anchorPoint) const
{
    if (!mIsValid) { return 0; }
    return mCommonEditor.angleFromPoint(point, anchorPoint);
}

void SelectionBitmapEditor::lockMovementToAxis(const bool state)
{
    if (!mIsValid) { return; }
    mCommonEditor.lockMovementToAxis(state);
}

QPointF SelectionBitmapEditor::alignedPositionToAxis(QPointF currentPoint) const
{
    if (!mIsValid) { return QPointF(); }
    return mCommonEditor.alignedPositionToAxis(currentPoint);
}

void SelectionBitmapEditor::maintainAspectRatio(const bool state)
{
    if (!mIsValid) { return; }
    mCommonEditor.maintainAspectRatio(state);
}

void SelectionBitmapEditor::createImageCache()
{
    if (!mIsValid) { return; }

    // Make sure the selection is valid before creating a cache
    if (!mState->selectionRect.isValid()) {
        return;
    }

    if (!mCacheInvalidated) {
        invalidateBitmapCache();
    }
    mState->selectionImage = *mBitmapImage->copy(mState->selectionRect, mState->selectionPolygon).image();
    mCacheInvalidated = false;

    updateTransformedSelectionState();
}

void SelectionBitmapEditor::resetTransformation()
{
    if (!mIsValid) { return; }
    mCommonEditor.resetState();
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    if (!mIsValid) { return; }

    invalidate();
}

bool SelectionBitmapEditor::somethingSelected() const
{
    if (!mIsValid) { return false; }

    return mState->selectionPolygon.count() > 0;
}

bool SelectionBitmapEditor::isSelectionValid() const
{
    if (!mIsValid) { return false; }

    return somethingSelected() && (mState->selectionRect.width() >= 1 && mState->selectionRect.height() >= 1);
}

void SelectionBitmapEditor::flipSelection(bool flipVertical)
{
    if (!mIsValid) { return; }
    mCommonEditor.flipSelection(flipVertical);
}

void SelectionBitmapEditor::commitChanges()
{
    if (!mIsValid) { return; }
    if (!somethingSelected()) { return; }
    // BitmapImage* currentBitmapImage = static_cast<BitmapImage*>(keyframe);

    auto state = mState;
    // TODO: replace with qpolygon
    // const QPolygon& alignedSelection = mState->selectionPolygon;

    // if (!mTransformCopyImage) {
    //     return;
    // }
    // if (currentBitmapImage == nullptr) { return; }

    // BitmapImage* floatingImage = static_cast<SelectionBitmapEditor*>(currentBitmapImage->selectionEditor())->floatingImage();
    // if (mFloatingImage.selection.isValid()) {
    //     const QRect& transformedSelectionRect = mSelectionTransform.mapRect(alignedSelectionRect);
    //     const QImage& transformedFloatingImage = mFloatingImage.image.transformed(mSelectionTransform, Qt::SmoothTransformation);

    // //     auto floatingBitmapImage = BitmapImage(transformedSelectionRect.topLeft(), transformedFloatingImage);
    // //     currentBitmapImage->paste(&floatingBitmapImage, QPainter::CompositionMode_SourceOver);
    // //     // TODO: figure out how we clear the temporary image without destroying the editor as well
    // //     // currentBitmapImage->clearTemporaryImage();
    // } else {
        // BitmapEditor transformedImage = mTransformCopyEditor.get()->transformed(mSelectionTransform, true);

        // mBitmapEditor->clear(alignedSelection);
        // mBitmapEditor->paste(transformedImage, QPainter::CompositionMode_SourceOver);
    // }
    // // When the selection has been applied, a new rect is applied based on the bounding box.
    // // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
    // setSelection(mapToSelection(QPolygonF(alignedSelection)).boundingRect());
}

// BitmapEditor SelectionBitmapEditor::transformedEditor()
// {
//     if (!mTransformCopyEditor) {
//         return BitmapEditor();
//     }
//     if (mCacheInvalidated) {
//         createImageCache();
//     }
//     return mTransformCopyEditor.get()->transformed(mSelectionTransform, true);
// }

void SelectionBitmapEditor::discardChanges()
{
    // if (!keyframe) { return; }

    // BitmapImage* bitmapImage = static_cast<BitmapImage*>(keyframe);

    // if (bitmapImage->temporaryImage()) {
    //     bitmapImage->clearTemporaryImage();
    // }

    resetSelectionProperties();
}

// void SelectionBitmapEditor::setFloatingImage(const QImage &floatingImage, const QRect &bounds)
// {
//     // mFloatingImage = SelectionBitmapImage();
//     // mFloatingImage.selection = bounds;
//     // mFloatingImage.image = floatingImage;
// }

void SelectionBitmapEditor::deleteSelection()
{
    if (!mIsValid) { return; }
    if (somethingSelected())
    {
        mBitmapImage->clear(mState->selectionRect);
    }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    if (!mIsValid) { return QPointF(); }
    if (!somethingSelected()) { return QPointF(); }

    return mCommonEditor.getSelectionAnchorPoint(mState->selectionPolygon);
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    if (!mIsValid) { return true; }
    if (!somethingSelected()) { return true; }

    return mCommonEditor.isOutsideSelection(point, mState->selectionPolygon);
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    if (!mIsValid) { return; }
    auto selectionState = mBitmapImage->mSelectionState;
    mCommonEditor.adjustCurrentSelection(selectionState.selectionPolygon, currentPoint.toPoint(), offset.toPoint(), rotationOffset, rotationIncrement);
    updateTransformedSelectionState();
}

void SelectionBitmapEditor::computeTransformedImageBounds(const QRect& sourceBounds,
                                   const QTransform& transform,
                                   QRect& outAlignedRect, QRectF& outPreciseRect) const
{
    QPolygon boundsPolygon(sourceBounds);

    QRectF boundingRect = transform.map(boundsPolygon).boundingRect();

    QPolygonF mappedPolygon = transform.map(QPolygonF(boundsPolygon));
    outPreciseRect = mappedPolygon.boundingRect();

    outAlignedRect = mappedPolygon.boundingRect().toRect();
}

void SelectionBitmapEditor::paste(TiledBuffer& tiledBuffer)
{
    if (!mIsValid) { return; }

    QRect alignedRect;
    QRectF preciseRect;
    computeTransformedImageBounds(mState->selectionRect, mState->commonState.selectionTransform, alignedRect, preciseRect);

    QPainter painter(&mState->transformedImage);
    auto const tiles = tiledBuffer.tiles();

    // Account for pixel offset
    painter.translate(-preciseRect.topLeft() + QPoint(1,1));

    QPainterPath path;
    path.addPolygon(mState->clipPolygon);
    painter.setClipPath(path);
    painter.setClipping(true);
    for (const Tile* item : tiles) {
        const QPixmap& tilePixmap = item->pixmap();
        const QPoint& tilePos = item->pos();
        painter.drawPixmap(tilePos, tilePixmap);
    }
    painter.end();

    mState->selectionImage = mState->transformedImage;
    mState->selectionRect = alignedRect;

    mState->selectionPolygon = QPolygon(QRect(mState->selectionRect));

    mCommonEditor.resetState();

    mCacheInvalidated = false;

    // to update the image on the canvas instantly
    updateTransformedSelectionState();
}


void SelectionBitmapEditor::updateTransformedSelectionState()
{
    if (!mIsValid) { return; }
    const QRect& originalBounds = mState->selectionRect;

    QRect transformedImageBounds;
    QRectF bRectF;
    QTransform transform = mState->commonState.selectionTransform;

    // if (mCacheInvalidated) {
    //     createImageCache();
    // }

    computeTransformedImageBounds(originalBounds, transform, transformedImageBounds, bRectF);

    mState->transformedImage = transformedImage(mState->selectionImage, transform, transformedImageBounds, bRectF, mSmoothTransform);
    int padding = mState->boundsPadding * 0.5;
    mState->transformedRect = transformedImageBounds.adjusted(-padding,
                                                              -padding,
                                                              padding,
                                                              padding);

    QTransform selectionT = mState->commonState.selectionTransform;
    QTransform deltaT = selectionT * mState->commonState.prevSelectionTransform.inverted();
    mState->clipTransform = deltaT * mState->clipTransform;
    mState->clipPolygon = QPolygonF(mState->clipTransform.map(QPolygonF(QRectF(mState->originalRect)))).toPolygon();
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
