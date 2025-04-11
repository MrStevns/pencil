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

#ifndef CAMERAPAINTER_H
#define CAMERAPAINTER_H

#include <memory>
#include <QColor>
#include <QTransform>
#include <QPen>

#include "onionskinpainteroptions.h"
#include "onionskinsubpainter.h"
#include "pencildef.h"

class LayerCamera;
class Object;
class QPalette;
class QPixmap;
class QRect;
class KeyFrame;

struct CameraPainterOptions
{
    const Object* object = nullptr;
    int frameIndex = 0;
    int currentLayerIndex = 0;
    int passepartoutOpacity = 25;
    LayerVisibility layerVisibility;
    float relativeLayerOpacityThreshold = 0;

    QTransform viewTransform;
    qreal viewScale = 0;

    bool isPlaying = false;
    bool cameraCacheValid = false;

    OnionSkinPainterOptions onionSkinOptions;
};

class CameraPainter
{
public:
    explicit CameraPainter(QPixmap& canvas);

    void paint(const QRect& blitRect);
    void paintCached(const QRect& blitRect);

    // void setOnionSkinPainterOptions(const OnionSkinPainterOptions& options) { mOnionSkinOptions = options; }
    void preparePainter(const CameraPainterOptions& cameraPainterOptions);
    void reset();

    void resetCache();

private:
    void initializePainter(QPainter& painter, QPixmap& pixmap, const QRect& blitRect, bool blitEnabled);
    void paintVisuals(QPainter& painter, const QRect& blitRect);
    void paintBorder(QPainter& painter, const QTransform& camTransform, const QRect& camRect);
    void paintOnionSkinning(QPainter& painter, const LayerCamera* cameraLayer);

    OnionSkinSubPainter mOnionSkinPainter;
    QPixmap& mCanvas;
    QPixmap mCameraPixmap;

    const QPointF mZeroPoint;
    CameraPainterOptions mOptions;
};

#endif // CAMERAPAINTER_H
