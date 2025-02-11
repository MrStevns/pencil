#include "selectionvectoreditor.h"


SelectionVectorEditor::SelectionVectorEditor() : SelectionEditor()
{
}

SelectionVectorEditor::~SelectionVectorEditor()
{
}

void SelectionVectorEditor::setSelection(const QRectF& rect)
{
    resetSelectionTransformProperties();
    mSelectionPolygon = rect;
    SelectionEditor::setSelection(rect);
}

void SelectionVectorEditor::resetSelectionProperties()
{
    resetSelectionTransformProperties();
    mSelectionPolygon = QPolygonF();
    SelectionEditor::resetSelectionProperties();
}

QPointF SelectionVectorEditor::getSelectionAnchorPoint() const
{
    return SelectionEditor::getSelectionAnchorPoint(mSelectionPolygon);
}

// const QRectF SelectionVectorEditor::mySelectionRect() const
// {
//     // TODO: This should fetch the rectangle from the vector keyframe.
//     return mSelectionPolygon.boundingRect();
// }

bool SelectionVectorEditor::isOutsideSelectionArea(const QPointF& point) const
{
    return (!mSelectionTransform.map(mSelectionPolygon).containsPoint(point, Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
}

void SelectionVectorEditor::adjustCurrentSelection(const QPointF &currentPoint, const QPointF &offset, qreal rotationOffset, int rotationIncrement)
{
    SelectionEditor::adjustCurrentSelection(mSelectionPolygon, currentPoint, offset, rotationOffset, rotationIncrement);
}
