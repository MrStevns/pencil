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

void MoveTool::pressEventPerspectiveTool(PointerEvent* event, PerspectiveOverlayTool& tool)
{
    LayerCamera* layerCam = mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex());
    Q_ASSERT(layerCam);

    tool.perspectiveMode = mEditor->overlays()->getMoveModeForPoint(event->canvasPos(), layerCam->getViewAtFrame(mEditor->currentFrame()));
    mEditor->overlays()->setPerspectiveMode(tool.perspectiveMode);
    QPoint mapped = layerCam->getViewAtFrame(mEditor->currentFrame()).map(event->canvasPos()).toPoint();
    mEditor->overlays()->updatePerspective(mapped);
}
