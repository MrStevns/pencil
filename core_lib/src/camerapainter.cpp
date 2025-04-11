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

#include "camerapainter.h"

#include <QPainter>
#include <QPixmap>
#include "object.h"
#include "layercamera.h"
#include "camera.h"
#include "keyframe.h"

#include "transform.h"

#include "painterutils.h"

CameraPainter::CameraPainter(QPixmap& canvas) : mCanvas(canvas)
{
    reset();
}

void CameraPainter::reset()
{
    mCameraPixmap = QPixmap(mCanvas.size());
    mCameraPixmap.setDevicePixelRatio(mCanvas.devicePixelRatioF());
    mCameraPixmap.fill(Qt::transparent);
}

void CameraPainter::resetCache()
{
    mOptions.cameraCacheValid = false;
}

void CameraPainter::preparePainter(const CameraPainterOptions& cameraPainterOptions)
{
    mOptions = cameraPainterOptions;
}

void CameraPainter::paint(const QRect& blitRect)
{
    QPainter painter;
    initializePainter(painter, mCanvas, blitRect, false);
    paintVisuals(painter, blitRect);

    mOptions.cameraCacheValid = true;
}

void CameraPainter::paintCached(const QRect& blitRect)
{
    QPainter painter;
    // As always, initialize the painter with the canvas image, as this is what we'll paint on
    // In this case though because the canvas has already been painted, we're not interested in
    // having the blitter clear the image again, as that would remove our previous painted data, ie. strokes...
    initializePainter(painter, mCanvas, blitRect, false);
    if (!mOptions.cameraCacheValid) {
        paintVisuals(painter, blitRect);
        painter.end();
        mOptions.cameraCacheValid = true;
    } else {
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(mZeroPoint, mCameraPixmap);
        painter.setWorldMatrixEnabled(true);
        painter.end();
    }
}

void CameraPainter::initializePainter(QPainter& painter, QPixmap& pixmap, const QRect& blitRect, bool blitEnabled)
{
    painter.begin(&pixmap);

    if (blitEnabled) {
        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(blitRect, Qt::transparent);
        // Surface has been cleared and is ready to be painted on
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    painter.setClipRect(blitRect);
    painter.setWorldMatrixEnabled(true);
    painter.setWorldTransform(mOptions.viewTransform);
}

void CameraPainter::paintVisuals(QPainter& painter, const QRect& blitRect)
{
    LayerCamera* cameraLayerBelow = static_cast<LayerCamera*>(mOptions.object->getLayerBelow(mOptions.currentLayerIndex, Layer::CAMERA));

    if (cameraLayerBelow == nullptr) { return; }

    const Layer* currentLayer = mOptions.object->getLayer(mOptions.currentLayerIndex);

    if (mOptions.layerVisibility == LayerVisibility::CURRENTONLY && currentLayer->type() != Layer::CAMERA) { return; }

    QPainter visualsPainter;
    initializePainter(visualsPainter, mCameraPixmap, blitRect, true);

    if (!mOptions.isPlaying || mOptions.onionSkinOptions.enabledWhilePlaying) {

        int startLayerI = 0;
        int endLayerI = mOptions.object->getLayerCount() - 1;
        for (int i = startLayerI; i <= endLayerI; i++) {
            Layer* layer = mOptions.object->getLayer(i);
            if (layer->type() != Layer::CAMERA) { continue; }

            LayerCamera* cameraLayer = static_cast<LayerCamera*>(layer);

            bool isCurrentLayer = cameraLayer == cameraLayerBelow;

            visualsPainter.save();
            visualsPainter.setOpacity(1);
            if (mOptions.layerVisibility == LayerVisibility::RELATED && !isCurrentLayer) {
                visualsPainter.setOpacity(calculateRelativeOpacityForLayer(mOptions.currentLayerIndex, i, mOptions.relativeLayerOpacityThreshold));
            }

            paintOnionSkinning(visualsPainter, cameraLayer);

            visualsPainter.restore();
        }
    }

    if (!cameraLayerBelow->visible()) { return; }

    QTransform camTransform = cameraLayerBelow->getViewAtFrame(mOptions.frameIndex);
    QRect cameraRect = cameraLayerBelow->getViewRect();
    paintBorder(visualsPainter, camTransform, cameraRect);

    painter.setWorldMatrixEnabled(false);
    painter.drawPixmap(mZeroPoint, mCameraPixmap);
}

void CameraPainter::paintBorder(QPainter& painter, const QTransform& camTransform, const QRect& camRect)
{
    painter.save();
    QRect viewRect = painter.viewport();

    painter.setOpacity(mOptions.passepartoutOpacity * 0.01);
    painter.setWorldMatrixEnabled(true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 255));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QTransform viewInverse = mOptions.viewTransform.inverted();
    QRect boundingRect = viewInverse.mapRect(viewRect);

    QPolygon camPoly = camTransform.inverted().map(QPolygon(camRect));
    QPolygon boundingRectPoly = boundingRect;
    QPolygon visibleCanvasPoly = boundingRectPoly.subtracted(camPoly);

    painter.drawPolygon(visibleCanvasPoly);

    painter.restore();
}

void CameraPainter::paintOnionSkinning(QPainter& painter, const LayerCamera* cameraLayer)
{
    QPen onionSkinPen;

    painter.save();
    painter.setBrush(Qt::NoBrush);
    painter.setWorldMatrixEnabled(false);

    onionSkinPen.setStyle(Qt::PenStyle::DashLine);
    QTransform viewTransform = mOptions.viewTransform;
    mOnionSkinPainter.paint(painter, cameraLayer, mOptions.onionSkinOptions, mOptions.frameIndex, [&] (OnionSkinPaintState state, int onionSkinNumber) {

        const QTransform& cameraTransform = cameraLayer->getViewAtFrame(onionSkinNumber);
        const QPolygonF& cameraPolygon = Transform::mapToWorldPolygon(cameraTransform, viewTransform, cameraLayer->getViewRect());
        if (state == OnionSkinPaintState::PREV) {

            if (mOptions.onionSkinOptions.colorizePrevFrames) {
                onionSkinPen.setColor(Qt::red);
            }

            painter.setPen(onionSkinPen);
            painter.drawPolygon(cameraPolygon);
        } else if (state == OnionSkinPaintState::NEXT) {
            if (mOptions.onionSkinOptions.colorizeNextFrames) {
                onionSkinPen.setColor(Qt::blue);
            }

            painter.setPen(onionSkinPen);
            painter.drawPolygon(cameraPolygon);
        } else if (state == OnionSkinPaintState::CURRENT) {
            painter.save();
            painter.setPen(Qt::black);
            painter.drawPolygon(cameraPolygon);
            painter.restore();
        }
    });
    painter.restore();
}
