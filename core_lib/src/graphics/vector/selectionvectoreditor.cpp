#include "selectionvectoreditor.h"

#include "vectorimage.h"

SelectionVectorEditor::SelectionVectorEditor()
{
}

SelectionVectorEditor::SelectionVectorEditor(VectorImage* vectorImage)
{
    if (!vectorImage) {
        invalidate(); return;
    }

    mVectorImage = vectorImage;
    mState = &mVectorImage->mSelectionState;
    mTransformEditor = SelectionTransformEditor(&mVectorImage->mSelectionState.transformState);
    mIsValid = true;
}

void SelectionVectorEditor::invalidate()
{
    if (mVectorImage) {
        mVectorImage->mSelectionState = SelectionVectorState();
        mVectorImage = nullptr;
    }
    mTransformEditor.invalidate();
    mState = nullptr;
    mIsValid = false;
}

void SelectionVectorEditor::setSelection(const QRectF& rect)
{
    mState->selectionRect = calculateBoundingBox(rect);
    mTransformEditor.setTransformAnchor(mTransformEditor.resolveAnchorPoint(QPoint(), QPolygonF(rect), 0));
}

QRectF SelectionVectorEditor::calculateBoundingBox(const QRectF& selectionRect)
{
    QRectF newSelection;
    auto curves = mVectorImage->curves();
    for (int i = 0; i < curves.size(); i++)
    {
        if (curves.at(i).intersects(selectionRect))
            newSelection |= curves[i].getBoundingRect();
    }

    return newSelection;
}

void SelectionVectorEditor::applyTransformation()
{
    auto curves = mVectorImage->curves();
    for (int i = 0; i < curves.size(); i++)
    {
        if (curves.at(i).isPartlySelected())
        {
            mVectorImage->curve(i).transform(mState->transformState.selectionTransform);
        }
    }
    // mState->selectionPolygon = calculateSelectionRect();
    // mSelectionTransformation.reset();
    mTransformEditor.resetState();
    mVectorImage->modification();
}

bool SelectionVectorEditor::somethingSelected() const
{
    if (!mIsValid) { return false; }

    // TODO: What does a selection mean here?
    // - A curve can be selected
    // - A vertex can be selected
    // - An area can be selected
    // Shall we always draw a bounding box in this case... or should be only be when a particular tool is selected

    return mState->selectionRect.isValid();
}

QRectF SelectionVectorEditor::selectionRect() const
{
    if (!mIsValid) { return QRectF(); }

    return mState->selectionRect;
}

DragHandle SelectionVectorEditor::resolveHandleMode(const QPointF& point, qreal selectionTolerance) const
{
    if (!mIsValid) { return DragHandle::NONE; }
    return mTransformEditor.resolveHandleMode(point, QPolygonF(mState->selectionRect), selectionTolerance);
}

bool SelectionVectorEditor::isOutsideSelectionArea(const QPointF &point, qreal tolerance) const
{
    if (!mIsValid) { return true; }

    return mTransformEditor.isOutsideSelection(point, mState->selectionRect, tolerance);
}

void SelectionVectorEditor::resetSelectionProperties()
{
    if (!mIsValid) { return; }

    invalidate();
}
