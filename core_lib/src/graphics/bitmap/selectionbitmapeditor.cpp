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
    mBitmapImage->mSelectionState = SelectionBitmapState();
    mBitmapImage = nullptr;
    mState = nullptr;
    mCommonEditor.invalidate();
    mIsValid = false;
}

void SelectionBitmapEditor::invalidateBitmapCache()
{
    if (mState->transformedImage.isNull()) { return; }

    // delete mState->transformedImage;
    // mState->transformedImage = nullptr;
    mCacheInvalidated = true;
}

void SelectionBitmapEditor::setSelection(const QRect& rect)
{
    setSelection(QPolygon(rect));
}

void SelectionBitmapEditor::setSelection(const QPolygon& polygon)
{
    mState->selectionPolygon = polygon;
    mState->originalRect = polygon.boundingRect().adjusted(0, 0,-1,-1);

    createImageCache();
}

QRect SelectionBitmapEditor::mySelectionRect() const
{
    if (!mIsValid) { return QRect(); }
    return mState->originalRect;
}

QPolygon SelectionBitmapEditor::mySelectionPolygon() const
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

    return mCommonEditor.myTranslation();
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
    mCommonEditor.setDragOrigin(point);
}

QPointF SelectionBitmapEditor::currentAnchorPoint() const
{
    if (!mIsValid) { return QPointF(); }
    return mCommonEditor.currentAnchorPoint();
}

void SelectionBitmapEditor::setTransformAnchor(const QPointF& anchorPoint)
{
    if (!mIsValid) { return; }
    mCommonEditor.setTransformAnchor(anchorPoint);
}

void SelectionBitmapEditor::translate(const QPointF& point)
{
    if (!mIsValid) { return; }
    mCommonEditor.translate(point);
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
    updateTransformedSelection();
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
    if (!mState->originalRect.isValid()) {
        return;
    }

    if (mCacheInvalidated) {
        invalidateBitmapCache();
        updateTransformedSelection();
    }
    mCacheInvalidated = false;
}

void SelectionBitmapEditor::resetTransformation()
{
    if (!mIsValid) { return; }
    mCommonEditor.resetState();
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    if (!mIsValid) { return; }

    if (mBitmapImage) {
        mBitmapImage->mSelectionState = SelectionBitmapState();
        mCommonEditor.invalidate();
    }
    invalidate();
}

bool SelectionBitmapEditor::somethingSelected() const
{
    if (!mIsValid) { return false; }

    return mState->selectionPolygon.count() > 0;
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
    const QPolygon& alignedSelection = mState->selectionPolygon;

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
        mBitmapImage->clear(mState->selectionPolygon);
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
    mCommonEditor.adjustCurrentSelection(selectionState.selectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
    updateTransformedSelection();
}

// QRect SelectionBitmapEditor::computeTransformedBounds(const QRect sourceBounds, const QTransform& transform)
// {

//     QPolygon sourcePolygon = QPolygon(sourceBounds);
//     QRectF boundingRect = transform.map(sourcePolygon).boundingRect();
//     // The bounding box of a qpolygon is always 1 pixel larger
//     // eg. 1x1 polygon becomes 2x2 boundingRect
//     boundingRect = boundingRect.adjusted(0, 0,-1,-1);

//     QRectF floatingRect = transform.map(QPolygonF(imagePolygon)).boundingRect();
//     QRect alignedRect = QRect(
//                             qFloor(boundingRect.x()),
//                             qFloor(boundingRect.y()),
//                             qCeil(boundingRect.width()),
//                             qCeil(boundingRect.height());

//     return
// }

void SelectionBitmapEditor::updateTransformedSelection()
{
    const QRect& imageBounds = mBitmapImage->bounds();
    const QPolygon& imagePolygon = QPolygon(imageBounds);
    QTransform transform = mState->commonState.selectionTransform;

    QRectF boundingRect = transform.map(imagePolygon).boundingRect();
    // // The bounding box of a qpolygon is always 1 pixel larger
    // // eg. 1x1 polygon becomes 2x2 boundingRect
    boundingRect = boundingRect.adjusted(0, 0,-1,-1);

    QRect transformedImageBounds = QRect(
                                       qFloor(boundingRect.x()),
                                       qFloor(boundingRect.y()),
                                       qCeil(boundingRect.width()),
                                       qCeil(boundingRect.height())
                                   );

    QRectF bRectF = transform.map(QPolygonF(imagePolygon)).boundingRect();

    QImage transformedImage = QImage(QSize(transformedImageBounds.width(), transformedImageBounds.height()), QImage::Format_ARGB32_Premultiplied);
    transformedImage.fill(Qt::transparent);

    QPainter imagePainter(&transformedImage);

    QPointF preciseCenter = QPointF(bRectF.width() * 0.5, bRectF.height() * 0.5);

    // Move origin to the center of the initial selection rect
    QTransform centeredTransform;
    centeredTransform.translate(preciseCenter.x(), preciseCenter.y());

    // Apply the selection transform
    centeredTransform *= transform;

    // Translate Back to the oldCenter of the
    centeredTransform.translate(-preciseCenter.x(), -preciseCenter.y());

    if (mSmoothTransform) {
        imagePainter.setRenderHint(QPainter::SmoothPixmapTransform);
        imagePainter.setRenderHint(QPainter::Antialiasing);
    }
    imagePainter.setTransform(centeredTransform);

    const QPointF& copiedImageCenter = QPointF(imageBounds.width() * 0.5, imageBounds.height() * 0.5);

    QPointF pixelCorrectionOffset = bRectF.topLeft() - transformedImageBounds.topLeft();
    QPointF centerInSourceCoords = centeredTransform.inverted().map(preciseCenter + pixelCorrectionOffset);

    QPointF imageDrawPoint = centerInSourceCoords - copiedImageCenter;

    imagePainter.drawImage(imageDrawPoint, *mBitmapImage->image());

    imagePainter.end();

    mState->transformedImage = transformedImage;
    mState->transformedRect = transformedImageBounds;
}
