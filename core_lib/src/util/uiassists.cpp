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
#include "uiassists.h"

/* The UI assistant class is meant to be used for mapping between the local and world coordinate spaces
 * mapFrom(...) functions should always contain an inverted transform.
 * mapToWorld(...) functions should always contains both the inverted object transform as well as the world transform
 *
 * Note: It is assumed that the rect, polygon, point etc... hasn't been transformed when inputted.
*/

QRectF Transform::mapFromLocalRect(const QTransform& transform, const QRect& rect)
{
    return QRectF(transform.inverted().mapRect(QRectF(rect)));
}

QRectF Transform::mapToWorldRect(const QTransform& transform, const QTransform& worldT, const QRect rect)
{
    return worldT.mapRect(mapFromLocalRect(transform, rect));
}

QPolygonF Transform::mapFromLocalPolygon(const QTransform& transform, const QRect& rect)
{
    return QPolygonF(transform.inverted().map(QPolygonF(QRectF(rect))));
}

QPolygonF Transform::mapToWorldPolygon(const QTransform& transform, const QTransform& worldT, const QRect& rect)
{
    return worldT.map(mapFromLocalPolygon(transform, rect));
}

QPointF RotationUIAssist::mapFromLocalPoint(const QPoint& origin, const QTransform& localT, const qreal objectScale, float worldScale)
{
    // Calculate the perceived distance from the frame to the handle
    // so that it looks like the handle is always x pixels above the origin
    qreal topDis = origin.y() + ((objectScale * origin.y()) * mOffsetFromFrame) * worldScale;
    return QPointF(localT.inverted().map(QPointF(0, topDis)));
}

QPointF RotationUIAssist::mapToWorldPoint(const QPoint& origin, const QTransform& localT, const qreal objectScale, const QTransform& worldT, float worldScale)
{
    return worldT.map(mapFromLocalPoint(origin, localT, objectScale, worldScale));
}
