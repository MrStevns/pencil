#include "selectionbitmapeditor.h"

#include "bitmapimage.h"

#include <QRectF>
#include <QDebug>
#include <QImage>

SelectionBitmapEditor::SelectionBitmapEditor() : SelectionEditor()
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
    resetSelectionTransformProperties();
    image->mSelectionState.selectionPolygon = polygon;
    image->mSelectionState.originalRect = polygon.boundingRect().adjusted(0, 0,-1,-1);

    // SelectionEditor::setSelection(polygon);

    createImageCache();
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
        mBitmapImage->clearSelectedArea();
    }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    if (!somethingSelected()) { return QPointF(); }

    auto state = mBitmapImage->mSelectionState;
    return SelectionEditor::getSelectionAnchorPoint(state.selectionPolygon);
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    if (!somethingSelected()) { return true; }

    auto state = mBitmapImage->mSelectionState;
    return (!state.commonState.selectionTransform.map(state.selectionPolygon).containsPoint(point.toPoint(), Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    SelectionEditor::adjustCurrentSelection(mState.selectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
