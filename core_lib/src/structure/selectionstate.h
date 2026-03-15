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
};

struct SelectionBitmapState {
    // The rect that belongs to the initial selection
    QRect originalRect;

    /// the state before being transformed
    QRect selectionRect;
    QImage selectionImage;
    QPolygonF selectionPolygon;
    ///

    SelectionState commonState;

    /// The state after being transformed
    QImage transformedImage;
    QRect transformedRect;
    ///
};

#endif // SELECTIONSTATE_H
