#ifndef SELECTIONVECTOREDITOR_H
#define SELECTIONVECTOREDITOR_H

#include "selectiontransformeditor.h"
#include "selectionvectorstate.h"

class VectorImage;

class SelectionVectorEditor
{
public:
    explicit SelectionVectorEditor();
    explicit SelectionVectorEditor(VectorImage* vectorImage);

    void setSelection(const QRectF& rect);
    void invalidate();

    void applyTransformation();

    bool isValid() const { return mIsValid; }

private:
    QRectF calculateBoundingBox(const QRectF& selectionRect);

private:
    bool mIsValid = false;
    SelectionTransformEditor mTransformEditor;

    VectorImage* mVectorImage = nullptr;
    SelectionVectorState* mState = nullptr;
};

#endif // SELECTIONVECTOREDITOR_H
