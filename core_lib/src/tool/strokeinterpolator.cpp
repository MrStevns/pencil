/***************************************************************************
 * This code is heavily influenced by the instrument proxy from QAquarelle *
 * QAquarelle -   Copyright (C) 2009 by Anton R. <commanderkyle@gmail.com> *
 *                                                                         *
 *   QAquarelle is free software; you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "strokeinterpolator.h"

#include <QLineF>
#include <QPainterPath>
#include "object.h"
#include "pointerevent.h"

StrokeInterpolator::StrokeInterpolator()
{
    mTabletInUse = false;
    reset();
}

void StrokeInterpolator::reset()
{
    mStrokeStarted = false;
    pressureQueue.clear();
    strokeQueue.clear();
}

void StrokeInterpolator::pointerPressEvent(PointerEvent* event)
{
    reset();
    if (!(event->button() == Qt::NoButton)) // if the user is pressing the left/right button
    {
        mCurrentPressPixel = event->viewportPos();
    }

    mLastPixel = mCurrentPixel = event->viewportPos();

    mStrokeStarted = true;
    mTabletInUse = mTabletInUse || event->isTabletEvent();
}

void StrokeInterpolator::pointerMoveEvent(PointerEvent* event)
{
    poll(event->viewportPos(), event->pressure());
}

void StrokeInterpolator::pointerReleaseEvent(PointerEvent* event)
{
    // flush out stroke
    if (mStrokeStarted)
    {
        pointerMoveEvent(event);
    }

    mStrokeStarted = false;
    mTabletInUse = mTabletInUse && !event->isTabletEvent();
}

void StrokeInterpolator::poll(QPointF pos, qreal pressure)
{
    // simple interpolation
    // QPointF smoothPos = QPointF((pos.x() + mCurrentPixel.x()) / 2.0, (pos.y() + mCurrentPixel.y()) / 2.0);
    mLastPixel = mCurrentPixel;
    mCurrentPixel = pos;
    mLastInterpolated = mCurrentPixel;

    // shift queue
    while (strokeQueue.size() >= STROKE_QUEUE_LENGTH)
    {
        strokeQueue.pop_front();
        pressureQueue.pop_front();
    }

    // Note(MrStevns): Was smoothPos, but we don't want a smooth pos, unless explicit said
    // maybe create a GUI control to set the smoothed position?
    strokeQueue.push_back(pos);
    pressureQueue.push_back(pressure);

    if (!mStrokeStarted)
    {
        return;
    }
}


QPointF StrokeInterpolator::interpolateStart(QPointF firstPoint)
{
    // Clear queue
    strokeQueue.clear();
    pressureQueue.clear();

    mLastPixel = firstPoint;
    return firstPoint;
}

QList<QPointF> StrokeInterpolator::interpolateStroke()
{
    return catmulInpolOp(strokeQueue);
}

QPointF StrokeInterpolator::catmullRomInterpolate(const QPointF& p0, const QPointF& p1,
                                                  const QPointF& p2, const QPointF& p3,
                                                  float t) const
{
    float t2 = t * t;
    float t3 = t2 * t;

    // Catmull-Rom spline equation (centripetal form, tension = 0.5)
    QPointF term1 = 2.0f * p1;
    QPointF term2 = (p2 - p0) * t;
    QPointF term3 = (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2;
    QPointF term4 = (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3;

    return 0.5f * (term1 + term2 + term3 + term4);
}


QList<QPointF> StrokeInterpolator::catmulInpolOp(const QList<QPointF>& points) const
{
    QList<QPointF> result;

    // You need at least 4 points for Catmull-Rom
    if (points.size() < 4)
        return result;

    for (int i = 0; i < points.size() - 3; ++i)
    {
        QPointF p0 = points[i];
        QPointF p1 = points[i + 1];
        QPointF p2 = points[i + 2];
        QPointF p3 = points[i + 3];

        // Subdivide each segment into 8 points
        const int subdivisions = 8;
        for (int j = 0; j <= subdivisions; ++j)
        {
            float t = float(j) / float(subdivisions);
            QPointF pt = catmullRomInterpolate(p0, p1, p2, p3, t);
            result.append(pt);
        }
    }

    return result;
}

void StrokeInterpolator::interpolateEnd()
{
    reset();
}
