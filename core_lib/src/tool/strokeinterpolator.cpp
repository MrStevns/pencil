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
    mTabletPressure = 0;

    reset();
    connect(&timer, &QTimer::timeout, this, &StrokeInterpolator::interpolatePollAndPaint);
}

void StrokeInterpolator::reset()
{
    mStrokeStarted = false;
    pressureQueue.clear();
    strokeQueue.clear();
    pressure = 0.0f;
    mHasTangent = false;
    timer.stop();
}

void StrokeInterpolator::setPressure(float pressure)
{
    mTabletPressure = pressure;
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
    setPressure(event->pressure());

    mTabletInUse = mTabletInUse || event->isTabletEvent();
}

void StrokeInterpolator::pointerMoveEvent(PointerEvent* event)
{
    // only applied to drawing tools.
    if (mStabilizerLevel != -1)
    {
        smoothMousePos(event->viewportPos());
    }
    else
    {
        // No smoothing
        mLastPixel = mCurrentPixel;
        mCurrentPixel = event->viewportPos();
        mLastInterpolated = mCurrentPixel;
    }
    if(event->isTabletEvent())
    {
        setPressure(event->pressure());
    }
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

void StrokeInterpolator::setStabilizerLevel(int level)
{
    mStabilizerLevel = level;
}

void StrokeInterpolator::smoothMousePos(QPointF pos)
{
    // Smooth mouse position before drawing
    QPointF smoothPos;

    if (mStabilizerLevel == StabilizationLevel::NONE)
    {
        mLastPixel = mCurrentPixel;
        mCurrentPixel = pos;
        mLastInterpolated = mCurrentPixel;
    }
    else if (mStabilizerLevel == StabilizationLevel::SIMPLE)
    {
        // simple interpolation
        smoothPos = QPointF((pos.x() + mCurrentPixel.x()) / 2.0, (pos.y() + mCurrentPixel.y()) / 2.0);
        mLastPixel = mCurrentPixel;
        mCurrentPixel = smoothPos;
        mLastInterpolated = mCurrentPixel;

        // shift queue
        while (strokeQueue.size() >= STROKE_QUEUE_LENGTH)
        {
            strokeQueue.pop_front();
        }

        // Note(MrStevns): Was smoothPos, but we don't want a smooth pos, unless explicit said
        // maybe create a GUI control to set the smoothed position?
        strokeQueue.push_back(pos);
        // qDebug() << strokeQueue;
    }
    else if (mStabilizerLevel == StabilizationLevel::STRONG)
    {
        smoothPos = QPointF((pos.x() + mLastInterpolated.x()) / 2.0, (pos.y() + mLastInterpolated.y()) / 2.0);

        mLastInterpolated = mCurrentPixel;
        mCurrentPixel = smoothPos;
        mLastPixel = mLastInterpolated;
    }

    if (!mStrokeStarted)
    {
        return;
    }

    if (!mTabletInUse)   // a mouse is used instead of a tablet
    {
        setPressure(1.0);
    }
}


QPointF StrokeInterpolator::interpolateStart(QPointF firstPoint)
{
    if (mStabilizerLevel == StabilizationLevel::SIMPLE)
    {
        // Clear queue
        strokeQueue.clear();
        pressureQueue.clear();

        mLastPixel = firstPoint;
    }
    else if (mStabilizerLevel == StabilizationLevel::STRONG)
    {
        // Clear queue
        strokeQueue.clear();
        pressureQueue.clear();

        const int sampleSize = 5;
        Q_ASSERT(sampleSize > 0);

        // fill strokeQueue with firstPoint x times
        for (int i = sampleSize; i > 0; i--)
        {
            strokeQueue.enqueue(firstPoint);
        }

        // last interpolated stroke should always be firstPoint
        mLastInterpolated = firstPoint;

        // draw and poll each millisecond
        timer.setInterval(sampleSize);
        timer.start();
    }
    else if (mStabilizerLevel == StabilizationLevel::NONE)
    {
        // Clear queue
        strokeQueue.clear();
        pressureQueue.clear();

        mLastPixel = firstPoint;
    }
    return firstPoint;
}

void StrokeInterpolator::interpolatePoll()
{
    // remove oldest stroke
    strokeQueue.dequeue();

    // add new stroke with the last interpolated pixel position
    strokeQueue.enqueue(mLastInterpolated);
}

void StrokeInterpolator::interpolatePollAndPaint()
{
    //qDebug() <<"inpol:" << mStabilizerLevel << "strokes"<< strokeQueue;
    if (!strokeQueue.isEmpty())
    {
        interpolatePoll();
        interpolateStroke();
    }
}

QList<QPointF> StrokeInterpolator::interpolateStroke()
{
    // is nan initially
    QList<QPointF> result;

    if (mStabilizerLevel == StabilizationLevel::SIMPLE)
    {
        result = catmulInpolOp(strokeQueue);
    }
    else if (mStabilizerLevel == StabilizationLevel::STRONG)
    {
        qreal x = 0;
        qreal y = 0;
        qreal pressure = 0;
        result = meanInpolOp(result, x, y, pressure);

    }
    else if (mStabilizerLevel == StabilizationLevel::NONE)
    {
        result = noInpolOp(result);
    }
    return result;
}

QList<QPointF> StrokeInterpolator::noInpolOp(QList<QPointF> points)
{
    setPressure(getPressure());

    points << mLastPixel << mLastPixel << mCurrentPixel << mCurrentPixel;

    // Set lastPixel to CurrentPixel
    // new interpolated pixel
    mLastPixel = mCurrentPixel;

    return points;
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


QList<QPointF> StrokeInterpolator::catmulInpolOp(const QList<QPointF>& points)
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
            float t = float(j) / subdivisions;
            QPointF pt = catmullRomInterpolate(p0, p1, p2, p3, t);
            result.append(pt);
        }
    }

    return result;
}

// Mean sampling interpolation operation
QList<QPointF> StrokeInterpolator::meanInpolOp(QList<QPointF> points, qreal x, qreal y, qreal pressure)
{
    for (int i = 0; i < strokeQueue.size(); i++)
    {
        x += strokeQueue[i].x();
        y += strokeQueue[i].y();
        pressure += getPressure();
    }

    // get arithmetic mean of x, y and pressure
    x /= strokeQueue.size();
    y /= strokeQueue.size();
    pressure /= strokeQueue.size();

    // Use our interpolated points
    QPointF mNewInterpolated(x, y);

    points << mLastPixel << mLastInterpolated << mNewInterpolated << mCurrentPixel;

    // Set lastPixel non interpolated pixel to our
    // new interpolated pixel
    mLastPixel = mNewInterpolated;

    return points;
}

void StrokeInterpolator::interpolateEnd()
{
    // Stop timer
    timer.stop();
    if (mStabilizerLevel == StabilizationLevel::STRONG)
    {
        if (!strokeQueue.isEmpty())
        {
            // How many samples should we get point from?
            // TODO: Qt slider.
            int sampleSize = 5;

            Q_ASSERT(sampleSize > 0);
            for (int i = sampleSize; i > 0; i--)
            {
                interpolatePoll();
                interpolateStroke();
            }
        }
    }
}
