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
#include "selectionpainter.h"

#include "object.h"
#include "basetool.h"
#include "bitmapimage.h"
#include "vectorimage.h"

#include <QDebug>
#include <QPainter>

SelectionPainter::SelectionPainter()
{
}

void SelectionPainter::paint(QPainter& painter,
                             const Object* object,
                             int layerIndex,
                             BaseTool* tool)
{
    Layer* layer = object->getLayer(layerIndex);

    if (layer == nullptr) { return; }

    const QRectF& selectionRect = mOptions.selectionRect;
    const QTransform& viewTransform = mOptions.viewTransform;
    const QTransform& selectionTransform = mOptions.selectionTransform;
    bool selectionActive = mOptions.isSelectionActive;

    if (selectionRect.isEmpty() || !selectionActive) { return; }

    QTransform transform = selectionTransform * viewTransform;
    QPolygonF projectedSelectionPolygon = transform.map(selectionRect);

    if (layer->type() == Layer::BITMAP)
    {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Qt::DashLine));

        // Draw current selection
        painter.drawPolygon(projectedSelectionPolygon.toPolygon());

    }
    if (layer->type() == Layer::VECTOR)
    {
        painter.setBrush(QColor(0, 0, 0, 20));
        painter.setPen(Qt::gray);
        painter.drawPolygon(projectedSelectionPolygon);
    }

    if (tool->type() == SELECT || tool->type() == MOVE)
    {
        painter.setPen(Qt::SolidLine);
        painter.setBrush(QBrush(Qt::gray));
        int radius = HANDLE_WIDTH / 2;

        const QRectF topLeftCorner = QRectF(projectedSelectionPolygon[0].x() - radius,
                                            projectedSelectionPolygon[0].y() - radius,
                                            HANDLE_WIDTH, HANDLE_WIDTH);
        painter.drawRect(topLeftCorner);

        const QRectF topRightCorner = QRectF(projectedSelectionPolygon[1].x() - radius,
                                             projectedSelectionPolygon[1].y() - radius,
                                             HANDLE_WIDTH, HANDLE_WIDTH);
        painter.drawRect(topRightCorner);

        const QRectF bottomRightCorner = QRectF(projectedSelectionPolygon[2].x() - radius,
                                                projectedSelectionPolygon[2].y() - radius,
                                                HANDLE_WIDTH, HANDLE_WIDTH);
        painter.drawRect(bottomRightCorner);

        const QRectF bottomLeftCorner = QRectF(projectedSelectionPolygon[3].x() - radius,
                                               projectedSelectionPolygon[3].y() - radius,
                                               HANDLE_WIDTH, HANDLE_WIDTH);
        painter.drawRect(bottomLeftCorner);
    }

    if (tool->properties.showSelectionInfo) {
        paintSelectionInfo(painter, selectionTransform, selectionRect.toAlignedRect(), projectedSelectionPolygon);
    }
}

void SelectionPainter::paintSelectionInfo(QPainter& painter, const QTransform& selectionTransform, const QRect& selectionRect, const QPolygonF& projectedPolygonF)
{
    QRect projectedSelectionRect = selectionTransform.mapRect(selectionRect);
    QPolygon projectedPolygon = projectedPolygonF.toPolygon();

    QPoint projectedCenter = projectedSelectionRect.center();
    QPoint originalCenter = selectionRect.center();
    int diffX = static_cast<int>(projectedCenter.x() - originalCenter.x());
    int diffY = static_cast<int>(projectedCenter.y() - originalCenter.y());
    painter.drawText(projectedPolygon[0] - QPoint(HANDLE_WIDTH, HANDLE_WIDTH),
                    QString("Size: %1x%2. Diff: %3, %4.").arg(QString::number(projectedSelectionRect.width()),
                                                      QString::number(projectedSelectionRect.height()),
                                                      QString::number(diffX),
                                                      QString::number(diffY)));
}
