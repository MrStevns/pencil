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
    // mOriginalRect = editor.mOriginalRect;
    mBitmapEditor = bitmapEditor;
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
    // mOriginalRect = rect.toAlignedRect();
    SelectionEditor::setSelection(polygon);
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    resetSelectionTransformProperties();
    mSelectionPolygon = QPolygon();
    // mOriginalRect = QRect();
    SelectionEditor::resetSelectionProperties();
}

void SelectionBitmapEditor::commitChanges()
{

    if (!somethingSelected()) { return; }
    // BitmapImage* currentBitmapImage = static_cast<BitmapImage*>(keyframe);

    // TODO: replace with qpolygon
    const QPolygon& alignedSelection = mSelectionPolygon;
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
        BitmapEditor transformedImage = mBitmapEditor->transformed(alignedSelection, mSelectionTransform, true);
        mBitmapEditor->clear(alignedSelection);
        mBitmapEditor->paste(transformedImage, QPainter::CompositionMode_SourceOver);
    // }
    // // When the selection has been applied, a new rect is applied based on the bounding box.
    // // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
    setSelection(mapToSelection(QPolygonF(alignedSelection)).boundingRect());
}

BitmapEditor SelectionBitmapEditor::transformedEditor() const
{
    return mBitmapEditor->transformed(mSelectionPolygon, mSelectionTransform, true);
}

QImage SelectionBitmapEditor::transformedEditorDebug() const
{
    return mBitmapEditor->transformed2(mSelectionPolygon, mSelectionTransform, true);
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
    return SelectionEditor::getSelectionAnchorPoint(mSelectionPolygon).toPoint();
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    return (!mSelectionTransform.map(mSelectionPolygon).containsPoint(point.toPoint(), Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    SelectionEditor::adjustCurrentSelection(mSelectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
