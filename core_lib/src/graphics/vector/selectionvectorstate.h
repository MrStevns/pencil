#ifndef SELECTIONVECTORSTATE_H
#define SELECTIONVECTORSTATE_H

#include "selectiontransformstate.h"
#include "vertexref.h"

#include <QList>

struct SelectionVectorState
{
    QPolygonF selectionPolygon;
    QList<int> selectedCurves;
    QList<VertexRef> selectedVertices;
    SelectionTransformState transformState;
};

#endif // SELECTIONVECTORSTATE_H
