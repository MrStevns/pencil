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

void Stroker::begin(QVector<QPointF> strokePoints)
{
    this->mStrokeSegment = strokePoints;
    this->mIndex = 0;
}

bool Stroker::next(const StrokeDynamics& dynamics, QPointF& outPoint)
{
    if (mStrokeSegment.size() <= 0) { return false; }

    for (; mIndex < mStrokeSegment.size() - 1; mIndex += 1) {

        const QPointF& a = mStrokeSegment[mIndex];
        const QPointF& b = mStrokeSegment[mIndex + 1];

        const qreal segmentLength = QLineF(a,b).length();

        if (segmentLength <= 0.0) {
            mSegmentOffset = 0.0;
            continue;
        }

        const QPointF dir = (b - a) / segmentLength;
        const qreal remaining = segmentLength - mSegmentOffset;
        qreal totalDistance = mLeftOverDabDistance + remaining;

        if (totalDistance >= dynamics.dabSpacing) {
            const qreal dabDelta = dynamics.dabSpacing - mLeftOverDabDistance;

            mSegmentOffset += dabDelta;
            outPoint = a + dir * mSegmentOffset;

            mLeftOverDabDistance = 0.0;
            return true;
        }

        // no dab yet, accumulate and move on
        mLeftOverDabDistance = totalDistance;
        mSegmentOffset = 0.0;
    }

    return false;

}
