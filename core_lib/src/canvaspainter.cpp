/*

Pencil2D - Traditional Animation Software
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "canvaspainter.h"

#include <QtMath>
#include <QSettings>

#include "object.h"
#include "mptile.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "bitmapimage.h"
#include "vectorimage.h"

#include "painterutils.h"
#include "imagecompositor.h"

CanvasPainter::CanvasPainter()
{
}

CanvasPainter::~CanvasPainter()
{
}

void CanvasPainter::setCanvas(QPixmap* canvas)
{
    Q_ASSERT(canvas);
    if (mCanvas == nullptr || mCanvasRect != mCanvas->rect()) {

        mCanvas = canvas;
        mCanvasRect = mCanvas->rect();
        mPostLayersPixmap = QPixmap(mCanvasRect.size());
        mPreLayersPixmap = QPixmap(mCanvasRect.size());
        mPreLayersPixmap.fill(Qt::transparent);
        mCanvas->fill(Qt::transparent);
        mPostLayersPixmap.fill(Qt::transparent);
        mOnionSkinCompositor = ImageCompositor(mCanvasRect);
        mCanvasCompositor = ImageCompositor(mCanvasRect);
    }
}

void CanvasPainter::setViewTransform(const QTransform view, const QTransform viewInverse)
{
    if (mViewTransform != view || mViewInvTransform != viewInverse) {
        mViewTransform = view;
        mViewInvTransform = viewInverse;
    }
}

void CanvasPainter::setTransformedSelection(QRect selection, QTransform transform)
{
    // Make sure that the selection is not empty
    if (selection.width() > 0 && selection.height() > 0)
    {
        mSelection = selection;
        mSelectionTransform = transform;
        mRenderTransform = true;
    }
    else
    {
        // Otherwise we shouldn't be in transformation mode
        ignoreTransformedSelection();
    }
}

void CanvasPainter::ignoreTransformedSelection()
{
    mRenderTransform = false;
}

void CanvasPainter::paintCached(const QRect& blitRect)
{
    QPainter preLayerPainter;
    QPainter mainPainter;
    QPainter postLayerPainter;

    initializePainter(mainPainter, *mCanvas, blitRect);

    if (!mPreLayersPixmapCacheValid)
    {
        initializePainter(preLayerPainter, mPreLayersPixmap, blitRect);
        renderPreLayers(preLayerPainter, blitRect);
        preLayerPainter.end();
        mPreLayersPixmapCacheValid = true;
    }

    mainPainter.setWorldMatrixEnabled(false);
    mainPainter.drawPixmap(blitRect, mPreLayersPixmap, blitRect);
    mainPainter.setWorldMatrixEnabled(true);

    renderCurLayer(mainPainter, blitRect);

    if (!mPostLayersPixmapCacheValid)
    {
        renderPostLayers(postLayerPainter, blitRect);
        mPostLayersPixmapCacheValid = true;
    }

    mainPainter.setWorldMatrixEnabled(false);
    mainPainter.drawPixmap(blitRect, mPostLayersPixmap, blitRect);
    mainPainter.setWorldMatrixEnabled(true);
}

void CanvasPainter::resetLayerCache()
{
    mPreLayersPixmapCacheValid = false;
    mPreLayersPixmapCacheValid = false;
}

void CanvasPainter::initializePainter(QPainter& painter, QPaintDevice& device, const QRect& blitRect)
{
    painter.begin(&device);

    // Clear the area that's about to be painted again, to avoid painting on top of existing pixels
    // causing artifacts.
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(blitRect, Qt::transparent);

    // Surface has been cleared and is ready to be painted on
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setWorldMatrixEnabled(true);
    painter.setWorldTransform(mViewTransform);
}

void CanvasPainter::renderPreLayers(QPainter& painter, const QRect& blitRect)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrameAtLayer(painter, 0, mCurrentLayerIndex - 1, blitRect);
    }

    paintOnionSkin(painter, blitRect);
}

void CanvasPainter::renderCurLayer(QPainter& painter, const QRect& blitRect)
{
    int currentLayerIndex = mCurrentLayerIndex;
    Layer* layer = mObject->getLayer(currentLayerIndex);

    painter.setOpacity(1.0);

    bool isCameraLayer = layer->type() == Layer::CAMERA;
    if (layer->visible() == false)
        return;

    if (mOptions.eLayerVisibility == LayerVisibility::RELATED && !isCameraLayer) {
        painter.setOpacity(calculateRelativeOpacityForLayer(currentLayerIndex, currentLayerIndex, mOptions.fLayerVisibilityThreshold));
    }

    switch (layer->type())
    {
    case Layer::BITMAP: { paintCurrentBitmapFrame(painter, layer, blitRect); break; }
    case Layer::VECTOR: { paintCurrentVectorFrame(painter, layer, blitRect); break; }
    default: break;
    }
}

void CanvasPainter::renderPostLayers(QPainter& painter, const QRect& blitRect)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrameAtLayer(painter, mCurrentLayerIndex+1, mObject->getLayerCount()-1, blitRect);
    }
}

void CanvasPainter::setPaintSettings(const Object* object, int currentLayer, int frame, QRect rect, BitmapImage *buffer)
{
    Q_UNUSED(rect)
    Q_ASSERT(object);
    mObject = object;

    mCurrentLayerIndex = currentLayer;
    mFrameNumber = frame;
    mBuffer = buffer;
}

void CanvasPainter::paint(const QRect& blitRect)
{
    QPainter preLayerPainter;
    QPainter mainPainter;
    QPainter postLayerPainter;

    initializePainter(mainPainter, *mCanvas, blitRect);

    initializePainter(preLayerPainter, mPreLayersPixmap, blitRect);
    renderPreLayers(preLayerPainter, blitRect);
    preLayerPainter.end();

    mainPainter.setWorldMatrixEnabled(false);
    mainPainter.drawPixmap(blitRect, mPreLayersPixmap, blitRect);
    mainPainter.setWorldMatrixEnabled(true);

    renderCurLayer(mainPainter, blitRect);

    initializePainter(postLayerPainter, mPostLayersPixmap, blitRect);
    renderPostLayers(postLayerPainter, blitRect);
    postLayerPainter.end();

    mainPainter.setWorldMatrixEnabled(false);
    mainPainter.drawPixmap(blitRect, mPostLayersPixmap, blitRect);
    mainPainter.setWorldMatrixEnabled(true);

    mPreLayersPixmapCacheValid = true;
    mPostLayersPixmapCacheValid = true;
}

void CanvasPainter::paintBackground()
{
    mCanvas->fill(Qt::transparent);
}

void CanvasPainter::paintOnionSkin(QPainter& painter, const QRect& blitRect)
{
    Layer* layer = mObject->getLayer(mCurrentLayerIndex);

    mOnionSkinSubPainter.paint(painter, layer, mOnionSkinPainterOptions, mFrameNumber, [&] (OnionSkinPaintState state, int onionFrameNumber) {
        if (state == OnionSkinPaintState::PREV) {
            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizePrevFrames, blitRect); break; }
            case Layer::VECTOR: { paintVectorOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizePrevFrames, blitRect); break; }
            default: break;
            }
        }
        if (state == OnionSkinPaintState::NEXT) {
            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizeNextFrames, blitRect); break; }
            case Layer::VECTOR: { paintVectorOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizeNextFrames, blitRect); break; }
            default: break;
            }
        }
    });
}

void CanvasPainter::paintOnionSkinFrame(QPainter& painter, ImageCompositor& compositor, int nFrame, bool colorize, qreal frameOpacity, const QRect& blitRect)
{
    if (colorize)
    {
        QColor colorBrush = Qt::transparent; //no color for the current frame

        if (nFrame < mFrameNumber)
        {
            colorBrush = Qt::red;
        }
        else if (nFrame > mFrameNumber)
        {
            colorBrush = Qt::blue;
        }
        compositor.addEffect(CompositeEffect::Colorize, colorBrush);
    }

    painter.save();
    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    // Remember to adjust opacity based on addition opacity value from image
    painter.setOpacity(frameOpacity - (1.0-painter.opacity()));
    painter.drawImage(blitRect, compositor.output(), blitRect);
    painter.restore();
}

void CanvasPainter::paintBitmapOnionSkinFrame(QPainter& painter, Layer* layer, int nFrame, bool colorize, const QRect& blitRect)
{
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);

    BitmapImage* bitmapImage = bitmapLayer->getLastBitmapImageAtFrame(nFrame, 0);

    if (bitmapImage == nullptr) { return; }
    mOnionSkinCompositor.initialize(blitRect, bitmapImage->topLeft(), mViewTransform);

    mOnionSkinCompositor.addImage(*bitmapImage->image());
    paintOnionSkinFrame(painter, mOnionSkinCompositor, nFrame, colorize, bitmapImage->getOpacity(), blitRect);
}


void CanvasPainter::paintVectorOnionSkinFrame(QPainter& painter, Layer* layer, int nFrame, bool colorize, const QRect& blitRect)
{
    LayerVector* vectorLayer = static_cast<LayerVector*>(layer);

    CANVASPAINTER_LOG("Paint Onion skin vector, Frame = %d", nFrame);
    VectorImage* vectorImage = vectorLayer->getLastVectorImageAtFrame(nFrame, 0);
    if (vectorImage == nullptr)
    {
        return;
    }

    mOnionSkinCompositor.initialize(blitRect, mBuffer->topLeft(), QTransform());
    vectorImage->outputImage(&mOnionSkinCompositor.output(), mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);
    mOnionSkinCompositor.addImage(*mBuffer->image(), mOptions.cmBufferBlendMode);
    paintOnionSkinFrame(painter, mOnionSkinCompositor, nFrame, colorize, vectorImage->getOpacity(), blitRect);
}

void CanvasPainter::paintBitmapFrame(QPainter& painter, Layer* layer, int nFrame, bool isCurrentFrame, bool isCurrentLayer, const QRect& blitRect)
{
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);

    BitmapImage* bitmapImage = bitmapLayer->getLastBitmapImageAtFrame(nFrame, 0);

    if (bitmapImage == nullptr) { return; }
    painter.setOpacity(bitmapImage->getOpacity() - (1.0-painter.opacity()));

    bool isPainting = mOptions.isPainting;

    if (!isPainting || !isCurrentFrame || !isCurrentLayer) {
        painter.save();
        painter.setWorldMatrixEnabled(false);

        mCanvasCompositor.initialize(blitRect, bitmapImage->topLeft(), mViewTransform);

        mCanvasCompositor.addImage(*bitmapImage->image());

        painter.drawImage(blitRect, mCanvasCompositor.output(), blitRect);
        painter.restore();
    }
}

void CanvasPainter::paintCurrentBitmapFrame(QPainter& painter, Layer* layer, const QRect& blitRect)
{
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);
    BitmapImage* bitmapImage = bitmapLayer->getLastBitmapImageAtFrame(mFrameNumber);

    if (bitmapImage == nullptr) { return; }
    bool isPainting = mOptions.isPainting;

    painter.setWorldMatrixEnabled(false);
    // Only paint with tiles for the frame we are painting on

    painter.setOpacity(bitmapImage->getOpacity() - (1.0-painter.opacity()));
    if (isPainting && !mOptions.tilesToBeRendered.isEmpty()) {
        paintBitmapTiles(painter, bitmapImage, blitRect);
    } else {
        painter.save();

        mCanvasCompositor.initialize(blitRect, bitmapImage->topLeft(), mViewTransform);
        mCanvasCompositor.addImage(*bitmapImage->image());

        if (mRenderTransform) {
            mCanvasCompositor.addEffect(CompositeEffect::Transformation, mSelectionTransform, mSelection);
        }

        painter.drawImage(blitRect, mCanvasCompositor.output(), blitRect);

        painter.restore();
    }
}

void CanvasPainter::paintCurrentVectorFrame(QPainter& painter, Layer* layer, const QRect& blitRect)
{
    LayerVector* vectorLayer = static_cast<LayerVector*>(layer);

    CANVASPAINTER_LOG("Paint Onion skin vector, Frame = %d", nFrame);
    VectorImage* vectorImage = vectorLayer->getLastVectorImageAtFrame(mFrameNumber, 0);
    if (vectorImage == nullptr)
    {
        return;
    }

    mCanvasCompositor.initialize(blitRect, mBuffer->topLeft(), QTransform());

    if (mRenderTransform) {
        vectorImage->setSelectionTransformation(mSelectionTransform);
    }

    // Paint vector image to offscreen buffer, strokeImage.
    vectorImage->outputImage(&mCanvasCompositor.output(), mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    mCanvasCompositor.addImage(*mBuffer->image(), mOptions.cmBufferBlendMode);

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    // Remember to adjust opacity based on addition opacity value from image
    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    painter.drawImage(blitRect, mCanvasCompositor.output(), blitRect);
}

void CanvasPainter::paintBitmapTiles(QPainter& painter, BitmapImage* image, const QRect& blitRect)
{
    const auto& tilesToRender = mOptions.tilesToBeRendered;

    painter.save();

    mCanvasCompositor.initialize(blitRect, image->topLeft(), mViewTransform);
    mCanvasCompositor.addImage(*image->image());
    auto output = mCanvasCompositor.output();

    QPainter imagePaintDevice(&output);

    imagePaintDevice.setTransform(mViewTransform);

    if (mOptions.useCanvasBuffer) {
        imagePaintDevice.setCompositionMode(QPainter::CompositionMode_SourceOver);
    } else {
        imagePaintDevice.setCompositionMode(QPainter::CompositionMode_Source);
    }
    for (const MPTile* item : tilesToRender) {
        const QImage& img = item->image();

        const QRect& tileRect = QRect(item->pos(), img.size());
        imagePaintDevice.drawImage(tileRect, img, img.rect());
    }

    painter.drawImage(blitRect, output, blitRect);
    painter.restore();
}

void CanvasPainter::paintVectorFrame(QPainter& painter,
                                     Layer* layer,
                                     int nFrame,
                                     bool isCurrentFrame,
                                     bool isCurrentLayer,
                                     const QRect& blitRect)
{
    Q_UNUSED(isCurrentFrame)
    Q_UNUSED(isCurrentLayer)
    LayerVector* vectorLayer = static_cast<LayerVector*>(layer);

    CANVASPAINTER_LOG("Paint Onion skin vector, Frame = %d", nFrame);
    VectorImage* vectorImage = vectorLayer->getLastVectorImageAtFrame(nFrame, 0);
    if (vectorImage == nullptr)
    {
        return;
    }

    mCanvasCompositor.initialize(blitRect, mBuffer->topLeft(), QTransform());

    // Paint vector image to offscreen buffer, strokeImage.
    vectorImage->outputImage(&mCanvasCompositor.output(), mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    mCanvasCompositor.addImage(*mBuffer->image());

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    painter.drawImage(blitRect, mCanvasCompositor.output(), blitRect);
}

/** Paints layers within the specified range for the current frame.
 *
 *  @param painter The painter to paint to
 *  @param startLayer The first layer to paint (inclusive)
 *  @param endLayer The last layer to paint (inclusive)
 */
void CanvasPainter::paintCurrentFrameAtLayer(QPainter& painter, int startLayer, int endLayer, const QRect& blitRect)
{
    painter.setOpacity(1.0);

    bool isCameraLayer = mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA;

    for (int i = startLayer; i <= endLayer; ++i)
    {
        Layer* layer = mObject->getLayer(i);

        if (layer->visible() == false)
            continue;

        if (mOptions.eLayerVisibility == LayerVisibility::RELATED && !isCameraLayer)
        {
            painter.setOpacity(calculateRelativeOpacityForLayer(mCurrentLayerIndex, i, mOptions.fLayerVisibilityThreshold));
        }

        bool isCurrentLayer = i == mCurrentLayerIndex;

        switch (layer->type())
        {
        case Layer::BITMAP: { paintBitmapFrame(painter, layer, mFrameNumber, true, isCurrentLayer, blitRect); break; }
        case Layer::VECTOR: { paintVectorFrame(painter, layer, mFrameNumber, true, isCurrentLayer, blitRect); break; }
        default: break;
        }
    }
}
