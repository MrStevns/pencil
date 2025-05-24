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

#include <QList>
#include <QPointF>

class Stroker
{
public:
    Stroker();

    /// Begin a stroke at the given point and with the specified stroke dynamics
    void begin(const QPointF& start, const StrokeDynamics& dynamics);

    /// Append a list of points to be used to create the stroked segment
    void append(const QList<QPointF>& next);
    const QList<QPointF> strokedSegment();

    const StrokeDynamics& dynamics() const { return mDynamics; }

    /// Cleanup the stroke
    void end();
private:

    // The segment we need to draw the current stroke
    QList<QPointF> mStrokeSegment;

    // The data points we need to recreate the stroke as a BezierCurve
    QList<QPointF> mStrokePoints;
    QList<qreal> mStrokePressures;

    StrokeDynamics mDynamics;
    qreal mLeftOverDabDistance = 0.0;
};

#endif // STROKER_H
