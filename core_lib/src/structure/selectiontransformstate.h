#ifndef SELECTIONTRANSFORMSTATE_H
#define SELECTIONTRANSFORMSTATE_H

#include <QPointF>
#include <QTransform>
#include <QPolygonF>

struct SelectionTransformState {
    qreal scaleX = 1;
    qreal scaleY = 1;
    QPointF translation;
    qreal rotatedAngle = 0.0;
    QPointF anchorPoint;

    QTransform selectionTransform;
};

#endif // SELECTIONTRANSFORMSTATE_H
