#ifndef SELECTIONBITMAPSTATE_H
#define SELECTIONBITMAPSTATE_H

#include "selectiontransformstate.h"

#include <QPointF>
#include <QTransform>
#include <QPolygonF>
#include <QImage>

struct SelectionBitmapState {
    // The image bounds that belong to the initial selection
    QRect baseImageBounds;
    // The base image cache is constructed from the bitmap image
    QImage baseImageCache;

    /// The geometry of the selection, used to draw the visual selection outline
    QPolygon selectionGeometry;

    /// the state before being transformed
    QRect selectionImageBounds;
    ///

    /// The state after being transformed
    QImage transformedImage;
    QRect transformedImageBounds;
    ///

    /// Renderer cache
    QImage cachedPatch;
    QRect cachedPatchBounds;

    // The editor that owns the current transformation
    SelectionTransformState transformState;

    // Padding to account for pixels being out of bound when rotating
    int boundsPadding = 2;

    bool smoothTransform = true;
};

#endif // SELECTIONBITMAPSTATE_H
