#ifndef SELECTIONSTATE_H
#define SELECTIONSTATE_H

#include <QPointF>
#include <QTransform>
#include <QPolygonF>
#include <QImage>

struct SelectionState {
    qreal scaleX = 1;
    qreal scaleY = 1;
    QPointF translation;

    qreal rotatedAngle = 0.0;

    QPointF anchorPoint;

    QTransform selectionTransform;
    QTransform prevSelectionTransform;

    SelectionState() = default;
};

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

    /// Clipping
    QPolygon clipPolygon;
    QTransform clipTransform;

    SelectionState commonState;

    // Padding to account for pixels being out of bound when rotating
    int boundsPadding = 2;
};

#endif // SELECTIONSTATE_H
