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
#ifndef UIASSISTS_H
#define UIASSISTS_H

#include <QTransform>
#include <QPolygonF>

class UIAssist
{
public:
    UIAssist() {};

    static QRectF mapFromLocalRect(const QTransform& transform, const QRect& rect);
    static QRectF mapToWorldRect(const QTransform& transform, const QTransform& worldT, const QRect rect);

    static QPolygonF mapFromLocalPolygon(const QTransform& transform, const QRect& rect);
    static QPolygonF mapToWorldPolygon(const QTransform& transform, const QTransform& worldT, const QRect& rect);
};

class RotationUIAssist
{
public:
    RotationUIAssist() { };

    static QPointF mapFromLocalPoint(const QPoint& origin, const QTransform& localT, const qreal objectScale, float worldScale);
    static QPointF mapToWorldPoint(const QPoint& origin, const QTransform& localT, const qreal objectScale, const QTransform& worldT, float worldScale);

private:

    // Spacer for rotation handle offset
    Q_CONSTEXPR static qreal mOffsetFromFrame = 0.05;
};

#endif // UIASSISTS_H
