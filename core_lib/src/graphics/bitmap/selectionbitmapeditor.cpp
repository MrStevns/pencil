#include "selectionbitmapeditor.h"

#include "bitmapimage.h"
#include "bitmapeditor.h"

#include <QRectF>
#include <QDebug>
#include <QImage>

SelectionBitmapEditor::SelectionBitmapEditor(BitmapEditor* bitmapEditor) : SelectionEditor()
{
    mBitmapEditor = bitmapEditor;
}

SelectionBitmapEditor::SelectionBitmapEditor(SelectionBitmapEditor& editor, BitmapEditor* bitmapEditor) : SelectionEditor(editor)
{
    mSelectionPolygon = editor.mSelectionPolygon;
    mOriginalRect = editor.mOriginalRect;
    mBitmapEditor = bitmapEditor;

    if (editor.mTransformCopyEditor) {
        mTransformCopyEditor.reset(new BitmapEditor(*editor.mTransformCopyEditor.get()));
    }
}

SelectionBitmapEditor::~SelectionBitmapEditor()
{
    qDebug() << "deinit SelectionBitmapEditor";
}

void SelectionBitmapEditor::setSelection(const QRectF& rect)
{
    setSelection(QPolygonF(rect));
}

void SelectionBitmapEditor::setSelection(const QPolygonF& polygon)
{
    resetSelectionTransformProperties();
    mSelectionPolygon = polygon.toPolygon();

    SelectionEditor::setSelection(polygon);

    mOriginalRect = mSelectionPolygon.boundingRect().adjusted(0, 0,-1,-1);

    createImageCache();
}

void SelectionBitmapEditor::createImageCache()
{
    // Make sure the selection is valid before creating a cache
    if (!mOriginalRect.isValid()) {
        return;
    }

    if (mCacheInvalidated || !mTransformCopyEditor || mTransformCopyEditor.get()->bounds().size() != mOriginalRect.size()) {
        mTransformCopyEditor.reset(new BitmapEditor(mBitmapEditor->copyArea(mOriginalRect, mSelectionPolygon)));
    }
    mCacheInvalidated = false;
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    resetSelectionTransformProperties();
    mSelectionPolygon = QPolygon();
    mOriginalRect = QRect();
    mTransformCopyEditor.reset();
    mCacheInvalidated = true;
    SelectionEditor::resetSelectionProperties();
}

void SelectionBitmapEditor::commitChanges()
{

    if (!somethingSelected()) { return; }
    // BitmapImage* currentBitmapImage = static_cast<BitmapImage*>(keyframe);

    // TODO: replace with qpolygon
    const QPolygon& alignedSelection = mSelectionPolygon;

    if (!mTransformCopyEditor) {
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
        BitmapEditor transformedImage = mTransformCopyEditor.get()->transformed(mSelectionTransform, true);

        mBitmapEditor->clear(alignedSelection);
        mBitmapEditor->paste(transformedImage, QPainter::CompositionMode_SourceOver);
    // }
    // // When the selection has been applied, a new rect is applied based on the bounding box.
    // // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
    setSelection(mapToSelection(QPolygonF(alignedSelection)).boundingRect());
}

BitmapEditor SelectionBitmapEditor::transformedEditor()
{
    if (!mTransformCopyEditor) {
        return BitmapEditor();
    }
    if (mCacheInvalidated) {
        createImageCache();
    }
    return mTransformCopyEditor.get()->transformed(mSelectionTransform, true);
}

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
        mBitmapEditor->clear(mSelectionPolygon);
    }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    return SelectionEditor::getSelectionAnchorPoint(mSelectionPolygon);
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    return (!mSelectionTransform.map(mSelectionPolygon).containsPoint(point.toPoint(), Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    SelectionEditor::adjustCurrentSelection(mSelectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
