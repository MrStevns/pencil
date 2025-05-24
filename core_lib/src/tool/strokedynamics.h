#ifndef STROKEDYNAMICS_H
#define STROKEDYNAMICS_H

#include <QPainter>

struct StrokeDynamics {
    bool canSingleDab;
    qreal width;
    qreal pressure;
    qreal feather;
    qreal opacity;
    qreal dabSpacing;
    bool antiAliasingEnabled;
    QColor color;

    QPainter::CompositionMode blending;
};

#endif // STROKEDYNAMICS_H
