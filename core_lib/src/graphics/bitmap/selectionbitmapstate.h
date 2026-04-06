#ifndef SELECTIONBITMAPSTATE_H
#define SELECTIONBITMAPSTATE_H

#include "selectiontransformstate.h"

#include <QPointF>
#include <QTransform>
#include <QPolygonF>
#include <QImage>

struct SelectionBitmapState {
    // The rect that belongs to the initial selection
    QRect originalRect;

    /// the state before being transformed
    QRect selectionRect;
    QImage selectionImage;
    QPolygon selectionPolygon;
    ///

    /// The state after being transformed
    QImage transformedImage;
    QRect transformedRect;
    ///

    SelectionTransformState transformState;

    // Padding to account for pixels being out of bound when rotating
    int boundsPadding = 2;

    bool smoothTransform = true;
};

#endif // SELECTIONBITMAPSTATE_H
