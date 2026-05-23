#ifndef SELECTIONVECTORSTATE_H
#define SELECTIONVECTORSTATE_H

#include "selectiontransformstate.h"
#include "vertexref.h"

#include <QList>

struct SelectionVectorState
{
    QPolygonF selectionPolygon;
    QList<int> selectedCurves;
    QList<VertexRef> mClosestVertices;
    SelectionTransformState transformState;
};

#endif // SELECTIONVECTORSTATE_H
