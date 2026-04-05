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

#include "movetool.h"
#include "pointerevent.h"
#include "layercamera.h"

#include "editor.h"
#include "layermanager.h"
#include "overlaymanager.h"

QCursor MoveTool::perspectiveToolCreateCursor(PerspectiveMode mode) const
{
    QPixmap cursorPixmap = QPixmap(24, 24);

    cursorPixmap.fill(QColor(255, 255, 255, 0));
    QPainter cursorPainter(&cursorPixmap);
    cursorPainter.setRenderHint(QPainter::Antialiasing);

    switch(mode)
    {
    case PerspectiveMode::LEFT:
    case PerspectiveMode::RIGHT:
    case PerspectiveMode::MIDDLE:
    case PerspectiveMode::SINGLE:
    {
        cursorPainter.drawImage(QPoint(6,6),QImage("://icons/general/cursor-move.svg"));
        break;
    }
    default:
        return Qt::ArrowCursor;
    }
    cursorPainter.end();

    return QCursor(cursorPixmap);
}

void MoveTool::perspectiveToolPressEvent(PointerEvent* event, PerspectiveOverlayTool& tool)
{
    LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
    Q_ASSERT(layerCam);

    tool.perspectiveMode = mEditor->overlays()->getMoveModeForPoint(event->canvasPos(), layerCam->getViewAtFrame(mEditor->currentFrame()));
    mEditor->overlays()->setPerspectiveMode(tool.perspectiveMode);
    QPoint mapped = layerCam->getViewAtFrame(mEditor->currentFrame()).map(event->canvasPos()).toPoint();
    mEditor->overlays()->updatePerspective(mapped);
}
