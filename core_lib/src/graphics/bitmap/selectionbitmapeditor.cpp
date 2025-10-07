#include "selectionbitmapeditor.h"

#include "bitmapimage.h"

#include <QRectF>
#include <QDebug>
#include <QImage>

SelectionBitmapEditor::SelectionBitmapEditor()
{
    // mBitmapImage = bitmapImage;
}

// SelectionBitmapEditor::SelectionBitmapEditor(SelectionBitmapEditor& editor, BitmapImage* bitmapImage) : SelectionEditor(editor)
// {
//     // mState = editor.mState;
//     mBitmapImage = bitmapImage;

//     // if (editor.mTransformCopyImage) {
//     //     mTransformCopyImage.reset(new BitmapImage(*editor.mTransformCopyImage.get()));
//     // }
// }

SelectionBitmapEditor::~SelectionBitmapEditor()
{
    qDebug() << "deinit SelectionBitmapEditor";
}

SelectionBitmapState SelectionBitmapEditor::state() const
{
    if (mBitmapImage == nullptr) { return SelectionBitmapState(); }

    return mBitmapImage->mSelectionState;
}

void SelectionBitmapEditor::setSelection(BitmapImage* image, const QRect& rect)
{
    setSelection(image, QPolygon(rect));
}

void SelectionBitmapEditor::setSelection(BitmapImage* image, const QPolygon& polygon)
{
    mBitmapImage = image;
    commonEditor = SelectionEditor();
    image->mSelectionState.selectionPolygon = polygon;
    image->mSelectionState.originalRect = polygon.boundingRect().adjusted(0, 0,-1,-1);

    // SelectionEditor::setSelection(polygon);

    createImageCache();
}

QRect SelectionBitmapEditor::mySelectionRect() const
{
    return mBitmapImage->mSelectionState.originalRect;
}

qreal SelectionBitmapEditor::myRotation() const
{
    return mBitmapImage->mSelectionState.commonState.rotatedAngle;
}


qreal SelectionBitmapEditor::myScaleX() const
{
    return mBitmapImage->mSelectionState.commonState.scaleX;
}

qreal SelectionBitmapEditor::myScaleY() const
{
    return mBitmapImage->mSelectionState.commonState.scaleY;
}

QPointF SelectionBitmapEditor::myTranslation() const
{
    return mBitmapImage->mSelectionState.commonState.translation;
}

void SelectionBitmapEditor::setTranslation(const QPointF& point)
{
    commonEditor.setTranslation(mBitmapImage->mSelectionState.commonState, point);
}

void SelectionBitmapEditor::setRotation(qreal rotationAngle)
{
    commonEditor.setRotation(mBitmapImage->mSelectionState.commonState, rotationAngle);
}

void SelectionBitmapEditor::setScale(qreal scaleX, qreal scaleY)
{
    commonEditor.setScale(mBitmapImage->mSelectionState.commonState, scaleX, scaleY);
}

void SelectionBitmapEditor::setMoveMode(MoveMode mode)
{
    commonEditor.setMoveMode(mode);
}

void SelectionBitmapEditor::setTransformAnchor(const QPointF& anchorPoint)
{
    commonEditor.setTransformAnchor(mBitmapImage->mSelectionState.commonState, anchorPoint);
}

void SelectionBitmapEditor::translate(const QPointF& point)
{
    commonEditor.translate(mBitmapImage->mSelectionState.commonState, point);
}

void SelectionBitmapEditor::rotate(qreal rotationAngle, qreal lockedAngle)
{
    commonEditor.rotate(mBitmapImage->mSelectionState.commonState, rotationAngle, lockedAngle);
}

void SelectionBitmapEditor::scale(qreal scaleX, qreal scaleY)
{
    commonEditor.scale(mBitmapImage->mSelectionState.commonState, scaleX, scaleY);
}

QPointF SelectionBitmapEditor::mapToSelection(const QPointF& point) const
{
    return commonEditor.mapToSelection(mBitmapImage->mSelectionState.commonState, point);
}

QPointF SelectionBitmapEditor::mapFromLocalSpace(const QPointF& point) const
{
    return commonEditor.mapFromLocalSpace(mBitmapImage->mSelectionState.commonState, point);
}

QPolygonF SelectionBitmapEditor::mapToSelection(const QPolygonF& polygon) const
{
    return commonEditor.mapToSelection(mBitmapImage->mSelectionState.commonState, polygon);
}

QPolygonF SelectionBitmapEditor::mapFromLocalSpace(const QPolygonF& polygon) const
{
    return commonEditor.mapFromLocalSpace(mBitmapImage->mSelectionState.commonState, polygon);
}

void SelectionBitmapEditor::calculateSelectionTransformation()
{
    commonEditor.calculateSelectionTransformation(mBitmapImage->mSelectionState.commonState);
}

qreal SelectionBitmapEditor::angleFromPoint(const QPointF &point, const QPointF &anchorPoint) const
{
    return commonEditor.angleFromPoint(mBitmapImage->mSelectionState.commonState, point, anchorPoint);
}

QPointF SelectionBitmapEditor::alignedPositionToAxis(QPointF currentPoint) const
{
    return commonEditor.alignedPositionToAxis(currentPoint);
}

void SelectionBitmapEditor::createImageCache()
{
    if (mBitmapImage == nullptr) { return; }

    auto state = mBitmapImage->mSelectionState;
    // Make sure the selection is valid before creating a cache
    if (!state.originalRect.isValid()) {
        return;
    }

    if (mCacheInvalidated || !mTransformCopyImage || mTransformCopyImage.get()->bounds().size() != state.originalRect.size()) {
        mTransformCopyImage.reset(new BitmapImage(mBitmapImage->copy(state.originalRect, state.selectionPolygon)));
    }
    mCacheInvalidated = false;
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    // resetSelectionTransformProperties();

    if (mBitmapImage) {
        mBitmapImage->mSelectionState = SelectionBitmapState();
    }
    mTransformCopyImage.reset();
    mCacheInvalidated = true;
    // SelectionEditor::resetSelectionProperties();
}

bool SelectionBitmapEditor::somethingSelected() const
{
    if (mBitmapImage == nullptr) { return false; }

    return mBitmapImage->mSelectionState.selectionPolygon.count() > 0;
}

void SelectionBitmapEditor::flipSelection(bool flipVertical)
{
    commonEditor.flipSelection(mBitmapImage->mSelectionState.commonState, flipVertical);
}

void SelectionBitmapEditor::commitChanges()
{

    if (!somethingSelected()) { return; }
    // BitmapImage* currentBitmapImage = static_cast<BitmapImage*>(keyframe);

    auto state = mBitmapImage->mSelectionState;
    // TODO: replace with qpolygon
    const QPolygon& alignedSelection = state.selectionPolygon;

    if (!mTransformCopyImage) {
        return;
    }
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
        mBitmapImage->clear(mBitmapImage->mSelectionState.selectionPolygon);
    }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    if (!somethingSelected()) { return QPointF(); }

    auto state = mBitmapImage->mSelectionState;
    return commonEditor.getSelectionAnchorPoint(state.selectionPolygon);
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    if (!somethingSelected()) { return true; }

    auto state = mBitmapImage->mSelectionState;
    return (!state.commonState.selectionTransform.map(state.selectionPolygon).containsPoint(point.toPoint(), Qt::WindingFill)) && commonEditor.getMoveMode() == MoveMode::NONE;
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    auto selectionState = mBitmapImage->mSelectionState;
    commonEditor.adjustCurrentSelection(selectionState.commonState, selectionState.selectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
