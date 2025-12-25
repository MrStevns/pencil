/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang
Copyright (C) 2025-2099 Oliver Stevns Larsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef STROKER_H
#define STROKER_H

#include "strokedynamics.h"

#include <QVector>
#include <QPointF>

class Stroker
{
public:
    Stroker();

    void begin(QVector<QPointF> strokePoints);

    bool next(const StrokeDynamics& dynamics, QPointF& outPoint);

private:

    // The segment we need to draw the current stroke
    QVector<QPointF> mStrokeSegment;

    qreal mLeftOverDabDistance = 0.0;
    qreal mSegmentOffset = 0.0;
    int mIndex = 0;
};

#endif // STROKER_H
