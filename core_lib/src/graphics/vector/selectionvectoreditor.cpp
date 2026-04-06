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
    mState->selectionPolygon = rect;//calculateBoundingBox(rect);
    mTransformEditor.setTransformAnchor(mTransformEditor.resolveAnchorPoint(QPoint(), mState->selectionPolygon, 0));
}

QRectF SelectionVectorEditor::calculateBoundingBox(const QRectF& selectionRect)
{
    QRectF newSelection = selectionRect;
    auto curves = mVectorImage->curves();
    for (int i = 0; i < curves.size(); i++)
    {
        if (curves.at(i).isPartlySelected())
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
