/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang
Copyright (C) 2025-2099 Oliver S. Larsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#ifndef RADIALOFFSETTOOL_H
#define RADIALOFFSETTOOL_H

#include <QObject>
#include <QPainterPath>
#include <QKeyEvent>

class PointerEvent;

class RadialOffsetTool : public QObject
{
    Q_OBJECT
public:
    RadialOffsetTool(QObject* parent = nullptr);
    ~RadialOffsetTool();

    /// Setup which modifier the tool should identity with
    void setup(Qt::KeyboardModifiers modifiers);

    void setOffset(qreal offset) { mOffset = offset; }
    void pointerEvent(PointerEvent* event);

    bool isAdjusting() const { return mIsAdjusting; }

    const QPointF& offsetPoint() const { return mAdjustPoint; }

signals:
    void offsetChanged(qreal newSize);

private:

    bool startAdjusting(PointerEvent* event);
    void stopAdjusting(PointerEvent* event);
    void adjust(PointerEvent* event);

    Qt::KeyboardModifiers mKbModifiers;

    bool mIsAdjusting = false;
    qreal mOffset = 0.;
    QPointF mAdjustPoint = QPointF();
};

#endif // RADIALOFFSETTOOL_H
