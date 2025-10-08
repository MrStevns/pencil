#include "selectionbitmapeditor.h"

#include "bitmapimage.h"

#include <QRectF>
#include <QDebug>
#include <QImage>

SelectionBitmapEditor::SelectionBitmapEditor()
{
    mIsValid = false;
}

SelectionBitmapEditor::SelectionBitmapEditor(BitmapImage *bitmapImage)
{
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
    mBitmapImage = nullptr;
    mState = nullptr;
    mCommonEditor.invalidate();
    mIsValid = false;
    // mTransformCopyImage.reset();
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
    return mState->originalRect;
}

QPolygon SelectionBitmapEditor::mySelectionPolygon() const
{
    return mState->selectionPolygon;
}

qreal SelectionBitmapEditor::myRotation() const
{
    return mCommonEditor.myRotation();
}

qreal SelectionBitmapEditor::myScaleX() const
{
    return mCommonEditor.myScaleX();
}

qreal SelectionBitmapEditor::myScaleY() const
{
    return mCommonEditor.myScaleY();
}

QPointF SelectionBitmapEditor::myTranslation() const
{
    return mCommonEditor.myTranslation();
}

QTransform SelectionBitmapEditor::myTransform() const
{
    return mCommonEditor.myTransform();
}

void SelectionBitmapEditor::setTranslation(const QPointF& point)
{
    mCommonEditor.setTranslation(point);
}

void SelectionBitmapEditor::setRotation(qreal rotationAngle)
{
    mCommonEditor.setRotation(rotationAngle);
}

void SelectionBitmapEditor::setScale(qreal scaleX, qreal scaleY)
{
    mCommonEditor.setScale(scaleX, scaleY);
}

void SelectionBitmapEditor::setTransform(const QTransform& transform)
{
    mCommonEditor.setTransform(transform);
}

MoveMode SelectionBitmapEditor::moveMode() const
{
    return mCommonEditor.getMoveMode();
}

void SelectionBitmapEditor::setMoveMode(MoveMode mode)
{
    mCommonEditor.setMoveMode(mode);
}

MoveMode SelectionBitmapEditor::resolveMoveModeForAnchorInRange(const QPointF& point, qreal selectionTolerance) const
{
    return mCommonEditor.resolveMoveModeForAnchorInRange(point, mState->selectionPolygon, selectionTolerance);
}

void SelectionBitmapEditor::setDragOrigin(const QPointF& point)
{
    mCommonEditor.setDragOrigin(point);
}

QPointF SelectionBitmapEditor::currentAnchorPoint() const
{
    return mCommonEditor.currentAnchorPoint();
}

void SelectionBitmapEditor::setTransformAnchor(const QPointF& anchorPoint)
{
    mCommonEditor.setTransformAnchor(anchorPoint);
}

void SelectionBitmapEditor::translate(const QPointF& point)
{
    mCommonEditor.translate(point);
}

void SelectionBitmapEditor::rotate(qreal rotationAngle, qreal lockedAngle)
{
    mCommonEditor.rotate(rotationAngle, lockedAngle);
}

void SelectionBitmapEditor::scale(qreal scaleX, qreal scaleY)
{
    mCommonEditor.scale(scaleX, scaleY);
}

QPointF SelectionBitmapEditor::mapToSelection(const QPointF& point) const
{
    return mCommonEditor.mapToSelection(point);
}

QPointF SelectionBitmapEditor::mapFromLocalSpace(const QPointF& point) const
{
    return mCommonEditor.mapFromLocalSpace(point);
}

QPolygonF SelectionBitmapEditor::mapToSelection(const QPolygonF& polygon) const
{
    return mCommonEditor.mapToSelection(polygon);
}

QPolygonF SelectionBitmapEditor::mapFromLocalSpace(const QPolygonF& polygon) const
{
    return mCommonEditor.mapFromLocalSpace(polygon);
}

void SelectionBitmapEditor::calculateSelectionTransformation()
{
    mCommonEditor.calculateSelectionTransformation();
}

qreal SelectionBitmapEditor::angleFromPoint(const QPointF &point, const QPointF &anchorPoint) const
{
    return mCommonEditor.angleFromPoint(point, anchorPoint);
}

void SelectionBitmapEditor::lockMovementToAxis(const bool state)
{
    mCommonEditor.lockMovementToAxis(state);
}

QPointF SelectionBitmapEditor::alignedPositionToAxis(QPointF currentPoint) const
{
    return mCommonEditor.alignedPositionToAxis(currentPoint);
}

void SelectionBitmapEditor::maintainAspectRatio(const bool state)
{
    mCommonEditor.maintainAspectRatio(state);
}

void SelectionBitmapEditor::createImageCache()
{
    if (mBitmapImage == nullptr) { return; }

    // Make sure the selection is valid before creating a cache
    if (!mState->originalRect.isValid()) {
        return;
    }

    // if (mCacheInvalidated || !mTransformCopyImage || mTransformCopyImage.get()->bounds().size() != mState->originalRect.size()) {
    //     mTransformCopyImage.reset(new BitmapImage(mBitmapImage->copy(mState->originalRect, mState->selectionPolygon)));
    // }
    mCacheInvalidated = false;
}

void SelectionBitmapEditor::resetTransformation()
{
    mCommonEditor.resetState();
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    if (mBitmapImage) {
        mBitmapImage->mSelectionState = SelectionBitmapState();
        mCommonEditor.invalidate();
    }
    invalidate();
}

bool SelectionBitmapEditor::somethingSelected() const
{
    if (mBitmapImage == nullptr) { return false; }

    return mState->selectionPolygon.count() > 0;
}

void SelectionBitmapEditor::flipSelection(bool flipVertical)
{
    mCommonEditor.flipSelection(flipVertical);
}

void SelectionBitmapEditor::commitChanges()
{

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
    if (somethingSelected())
    {
        mBitmapImage->clear(mState->selectionPolygon);
    }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    if (!somethingSelected()) { return QPointF(); }

    return mCommonEditor.getSelectionAnchorPoint(mState->selectionPolygon);
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    if (!somethingSelected()) { return true; }

    return mCommonEditor.isOutsideSelection(point, mState->selectionPolygon);
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    auto selectionState = mBitmapImage->mSelectionState;
    mCommonEditor.adjustCurrentSelection(selectionState.selectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
