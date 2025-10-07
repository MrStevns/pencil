#ifndef SELECTIONSTATE_H
#define SELECTIONSTATE_H

#include <QPointF>
#include <QTransform>
#include <QPolygonF>

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
};

#endif // SELECTIONSTATE_H
