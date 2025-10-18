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
    QPolygon selectionPolygon;
    QRect originalRect;

    SelectionState commonState;

    // The uncomitted image contains data which hasn't been committed anywhere.
    // We use it to handle drawing on the floating selection.
    QImage uncomittedImage;

    // The transformed image contains the image data from the bitmap image,
    // after it's been transformed
    QImage transformedImage;
    QRect transformedRect;

    // The finished image, which contains both the transformed iamge and the uncomitted image.
    QImage compositedImage;
    QRect compositedRect;
};

#endif // SELECTIONSTATE_H
