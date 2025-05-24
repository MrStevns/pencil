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
#include "stroker.h"

#include <QDebug>

Stroker::Stroker()
{
}

void Stroker::begin(const QPointF &start, const StrokeDynamics& dynamics)
{
    mStrokeSegment << start;
    mDynamics = dynamics;
}

void Stroker::append(const QList<QPointF>& points)
{
    mStrokeSegment = points;
}

const QList<QPointF> Stroker::strokedSegment()
{
    qreal leftOverDistance = mLeftOverDabDistance;
    qreal totalDistance = 0.f;
    int segmentSize = mStrokeSegment.size();
    const StrokeDynamics& dynamics = mDynamics;

    QLineF line;
    for (int i = 1; i < segmentSize; i += 1) {
        const QPointF& lastPoint = mStrokeSegment[i-1];
        const QPointF& currentPoint = mStrokeSegment[i];

        // calculate the euclidean distance
        // to find the distance that we need to cover with dabs
        line.setP1(lastPoint);
        line.setP2(currentPoint);
        qreal distance = line.length();

        // Calculate the unit direction vector
        qreal dirX = (currentPoint.x() - lastPoint.x()) / distance;
        qreal dirY = (currentPoint.y() - lastPoint.y()) / distance;

        qreal offsetX = 0.0f;
        qreal offsetY = 0.0f;

        // since we want to dab at a specific interval,
        // add the potentially missing leftover distance to the current distance
        totalDistance = leftOverDistance + distance;

        // Draw dabs until totalDistance is less than dabSpacing
        while (totalDistance >= dynamics.dabSpacing)
        {
            // make sure to add potentially missed distance
            // to our offset
            qreal dabDelta = dynamics.dabSpacing;
            if (leftOverDistance > 0) {
                dabDelta -= leftOverDistance;
                leftOverDistance -= dynamics.dabSpacing;
            }
            offsetX += dirX * dabDelta;
            offsetY += dirY * dabDelta;

            mStrokeSegment << QPointF(lastPoint.x()+offsetX, lastPoint.y()+offsetY);

            // remove the distance we've covered already
            totalDistance -= dynamics.dabSpacing;
        }

        leftOverDistance = totalDistance;
    }

    // set the remaining dabs for next stroke
    mLeftOverDabDistance = totalDistance;
    return mStrokeSegment;

}

void Stroker::end()
{
    mStrokePoints.clear();
    mStrokePressures.clear();
    mStrokeSegment.clear();
}
