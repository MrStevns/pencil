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
}

SelectionBitmapEditor::~SelectionBitmapEditor()
{
    qDebug() << "deinit SelectionBitmapEditor";
}

void SelectionBitmapEditor::setSelection(const QRectF& rect)
{
    resetSelectionTransformProperties();
    mSelectionPolygon = rect.toAlignedRect();
    mOriginalRect = rect.toAlignedRect();
    SelectionEditor::setSelection(rect.toAlignedRect());
}

void SelectionBitmapEditor::resetSelectionProperties()
{
    resetSelectionTransformProperties();
    mSelectionPolygon = QPolygon();
    mOriginalRect = QRect();
    SelectionEditor::resetSelectionProperties();
}

void SelectionBitmapEditor::commitChanges()
{

    if (!somethingSelected()) { return; }
    // BitmapImage* currentBitmapImage = static_cast<BitmapImage*>(keyframe);

    const QRect& alignedSelectionRect = mOriginalRect;
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
        BitmapEditor transformedImage = mBitmapEditor->transformed(alignedSelectionRect, mSelectionTransform, true);
        mBitmapEditor->clear(alignedSelectionRect);
        mBitmapEditor->paste(transformedImage, QPainter::CompositionMode_SourceOver);
    // }
    // // When the selection has been applied, a new rect is applied based on the bounding box.
    // // This ensures that if the selection has been rotated, it will still fit the bounds of the image.
    setSelection(mapToSelection(QPolygonF(QRectF(mOriginalRect))).boundingRect());
}
void SelectionBitmapEditor::discardChanges()
{
    // if (!keyframe) { return; }

    // BitmapImage* bitmapImage = static_cast<BitmapImage*>(keyframe);

    // if (bitmapImage->temporaryImage()) {
    //     bitmapImage->clearTemporaryImage();
    // }

    // resetSelectionProperties();
}

// void SelectionBitmapEditor::setFloatingImage(const QImage &floatingImage, const QRect &bounds)
// {
//     // mFloatingImage = SelectionBitmapImage();
//     // mFloatingImage.selection = bounds;
//     // mFloatingImage.image = floatingImage;
// }

void SelectionBitmapEditor::deleteSelection()
{
    // if (somethingSelected())
    // {
    //     int currentFrame = editor()->currentFrame();
    //     editor()->backup(tr("Delete Selection", "Undo Step: clear the selection area."));
    //     if (mWorkingLayer->type() == Layer::VECTOR)
    //     {
    //         clearCurves();
    //         VectorImage* vectorImage = static_cast<VectorImage*>(mWorkingLayer->getLastKeyFrameAtPosition(currentFrame));
    //         Q_CHECK_PTR(vectorImage);
    //         vectorImage->deleteSelection();
    //     }
    //     else if (mWorkingLayer->type() == Layer::BITMAP)
    //     {
    //         BitmapImage* bitmapImage = static_cast<BitmapImage*>(mWorkingLayer->getLastKeyFrameAtPosition(currentFrame));
    //         Q_CHECK_PTR(bitmapImage);
    //         bitmapImage->clear(mOriginalRect);
    //     }
    //     editor()->setModified(editor()->currentLayerIndex(), currentFrame);
    // }
}

QPointF SelectionBitmapEditor::getSelectionAnchorPoint() const
{
    return SelectionEditor::getSelectionAnchorPoint(mSelectionPolygon).toPoint();
}

bool SelectionBitmapEditor::isOutsideSelectionArea(const QPointF& point) const
{
    return (!mSelectionTransform.map(mSelectionPolygon).containsPoint(point.toPoint(), Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
}

void SelectionBitmapEditor::setMoveModeForAnchorInRange(const QPointF& point)
{
    return SelectionEditor::setMoveModeForAnchorInRange(mSelectionPolygon, point);
}

void SelectionBitmapEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    SelectionEditor::adjustCurrentSelection(mSelectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
