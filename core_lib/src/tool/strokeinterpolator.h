/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef STROKEINTERPOLATOR_H
#define STROKEINTERPOLATOR_H

#include <QQueue>
#include <QPointF>
#include <QList>
#include <QTimer>

struct StrokeSegment {
    QQueue<QPointF> positions;
    QQueue<qreal> pressures;
};

class PointerEvent;

class StrokeInterpolator : public QObject
{
public:
    StrokeInterpolator();

    void pointerPressEvent(PointerEvent* event);
    void pointerMoveEvent(PointerEvent* event);
    void pointerReleaseEvent(PointerEvent* event);

    bool isActive() const { return mStrokeStarted; }

    StrokeSegment interpolateStroke();
    StrokeSegment interpolateStart();
    void interpolateEnd();
    void poll(QPointF pos, qreal pressure);

    QPointF getCurrentPixel() const { return mCurrentPixel; }
    QPointF getLastPixel() const { return mLastPixel; }
    QPointF getLastMeanPixel() const { return mLastInterpolated; }
    QPointF getCurrentPressPixel() const { return mCurrentPressPixel; }

private:
    QQueue<QPointF> catmulInpolOp(const QQueue<QPointF>& points) const;
    QPointF catmullRomInterpolate(const QPointF& p0, const QPointF& p1,
                                                      const QPointF& p2, const QPointF& p3, float t) const;
    static const int STROKE_QUEUE_LENGTH = 4; // 4 points for cubic bezier

    void reset();

    QQueue<QPointF> strokeQueue;
    QQueue<qreal> pressureQueue;

    QPointF mCurrentPressPixel = { 0, 0 };
    QPointF mCurrentPixel = { 0, 0 };
    QPointF mLastPixel = { 0, 0 };
    QPointF mLastInterpolated = { 0, 0 };

    qreal mPressure = 1.0;

    bool    mStrokeStarted = false;
    bool    mTabletInUse = false;
};

#endif // STROKEINTERPOLATOR_H
