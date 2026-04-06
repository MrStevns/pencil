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
    void resetSelectionProperties();

    void applyTransformation();

    bool isValid() const { return mIsValid; }

    bool somethingSelected() const;
    QRectF selectionRect() const;

    DragHandle resolveHandleMode(const QPointF& point, qreal selectionTolerance) const;

    bool isOutsideSelectionArea(const QPointF& point, qreal tolerance) const;

private:
    // void setAreaSelected(int areaNumber, bool YesOrNo);
    // void setSelected(int curveNumber, bool YesOrNo);
    // void setSelected(int curveNumber, int vertexNumber, bool YesOrNo);
    // void select(const QRectF& rect);

    QRectF calculateBoundingBox(const QRectF& selectionRect);

private:
    bool mIsValid = false;
    SelectionTransformEditor mTransformEditor;

    VectorImage* mVectorImage = nullptr;
    SelectionVectorState* mState = nullptr;
};

#endif // SELECTIONVECTOREDITOR_H
