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
        mPostLayersPixmap = QPixmap(mCanvas->size());
        mPreLayersPixmap = QPixmap(mCanvas->size());
        mPreLayersPixmap.fill(Qt::transparent);
        mCanvas->fill(Qt::transparent);
        mPostLayersPixmap.fill(Qt::transparent);

        mCanvas = canvas;
        mCanvasRect = mCanvas->rect();
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
        renderPreLayers(preLayerPainter);
        preLayerPainter.end();
        mPreLayersPixmapCacheValid = true;
    }

    mainPainter.setWorldMatrixEnabled(false);
    mainPainter.drawPixmap(blitRect, mPreLayersPixmap, blitRect);
    mainPainter.setWorldMatrixEnabled(true);

    renderCurLayer(mainPainter, blitRect);

    if (!mPostLayersPixmapCacheValid)
    {
        renderPostLayers(postLayerPainter);
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

void CanvasPainter::renderPreLayers(QPainter& painter)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrameAtLayer(painter, 0, mCurrentLayerIndex - 1);
    }

    paintOnionSkin(painter);
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

void CanvasPainter::renderPostLayers(QPainter& painter)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrameAtLayer(painter, mCurrentLayerIndex+1, mObject->getLayerCount()-1);
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
    renderPreLayers(preLayerPainter);
    preLayerPainter.end();

    mainPainter.setWorldMatrixEnabled(false);
    mainPainter.drawPixmap(blitRect, mPreLayersPixmap, blitRect);
    mainPainter.setWorldMatrixEnabled(true);

    renderCurLayer(mainPainter, blitRect);

    initializePainter(postLayerPainter, mPostLayersPixmap, blitRect);
    renderPostLayers(postLayerPainter);
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

void CanvasPainter::paintOnionSkin(QPainter& painter)
{
    Layer* layer = mObject->getLayer(mCurrentLayerIndex);

    mOnionSkinSubPainter.paint(painter, layer, mOnionSkinPainterOptions, mFrameNumber, [&] (OnionSkinPaintState state, int onionFrameNumber) {
        if (state == OnionSkinPaintState::PREV) {
            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizePrevFrames); break; }
            case Layer::VECTOR: { paintVectorOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizePrevFrames); break; }
            default: break;
            }
        }
        if (state == OnionSkinPaintState::NEXT) {
            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizeNextFrames); break; }
            case Layer::VECTOR: { paintVectorOnionSkinFrame(painter, layer, onionFrameNumber, mOnionSkinPainterOptions.colorizeNextFrames); break; }
            default: break;
            }
        }
    });
}

void CanvasPainter::paintBitmapOnionSkinFrame(QPainter& painter, Layer* layer, int nFrame, bool colorize)
{
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);

    BitmapImage* bitmapImage = bitmapLayer->getLastBitmapImageAtFrame(nFrame, 0);

    if (bitmapImage == nullptr) { return; }

    painter.save();
    painter.setWorldMatrixEnabled(false);
    painter.setOpacity(bitmapImage->getOpacity() - (1.0-painter.opacity()));

    ImageCompositor compositor(mCanvasRect, bitmapImage->topLeft(), mViewTransform);

    compositor.addImage(*bitmapImage->image());
    QColor colorBrush = Qt::transparent; //no color for the current frame
    if (colorize) 
    {   
        if (nFrame < mFrameNumber)
        {
            colorBrush = Qt::red;
        }
        else if (nFrame > mFrameNumber)
        {
            colorBrush = Qt::blue;
        }
    }

    compositor.addEffect(CompositeEffect::Colorize, colorBrush);

    painter.drawImage(QPoint(), compositor.output());
    painter.restore();
}


void CanvasPainter::paintVectorOnionSkinFrame(QPainter& painter, Layer* layer, int nFrame, bool colorize)
{
    LayerVector* vectorLayer = static_cast<LayerVector*>(layer);

    CANVASPAINTER_LOG("Paint Onion skin vector, Frame = %d", nFrame);
    VectorImage* vectorImage = vectorLayer->getLastVectorImageAtFrame(nFrame, 0);
    if (vectorImage == nullptr)
    {
        return;
    }

    ImageCompositor compositor(mCanvasRect, mBuffer->topLeft(), QTransform());

    if (mRenderTransform) {
        vectorImage->setSelectionTransformation(mSelectionTransform);
    }

    // Paint vector image to offscreen buffer, strokeImage.
    vectorImage->outputImage(&compositor.output(), mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    compositor.addImage(*mBuffer->image(), mOptions.cmBufferBlendMode);
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

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    // Remember to adjust opacity based on addition opacity value from image
    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    painter.drawImage(QPoint(), compositor.output());
}


void CanvasPainter::paintBitmapFrame(QPainter& painter, Layer* layer, int nFrame, bool isCurrentFrame, bool isCurrentLayer)
{
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);

    BitmapImage* bitmapImage = bitmapLayer->getLastBitmapImageAtFrame(nFrame, 0);

    if (bitmapImage == nullptr) { return; }
    painter.setOpacity(bitmapImage->getOpacity() - (1.0-painter.opacity()));

    bool isPainting = mOptions.isPainting;

    if (!isPainting || !isCurrentFrame || !isCurrentLayer) {
        painter.save();
        painter.setWorldMatrixEnabled(false);

        ImageCompositor compositor(mCanvasRect, bitmapImage->topLeft(), mViewTransform);

        compositor.addImage(*bitmapImage->image());

        painter.drawImage(QPoint(), compositor.output());
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
        ImageCompositor compositor(mCanvasRect, bitmapImage->topLeft(), mViewTransform);
        compositor.addImage(*bitmapImage->image());

        if (mRenderTransform) {
            compositor.addEffect(CompositeEffect::Transformation, mSelectionTransform, mSelection);
        }

        painter.drawImage(blitRect, compositor.output(), blitRect);

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

    ImageCompositor compositor(mCanvasRect, mBuffer->topLeft(), QTransform());

    if (mRenderTransform) {
        vectorImage->setSelectionTransformation(mSelectionTransform);
    }

    // Paint vector image to offscreen buffer, strokeImage.
    vectorImage->outputImage(&compositor.output(), mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    compositor.addImage(*mBuffer->image(), mOptions.cmBufferBlendMode);

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    // Remember to adjust opacity based on addition opacity value from image
    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    painter.drawImage(blitRect, compositor.output(), blitRect);
}

void CanvasPainter::paintBitmapTiles(QPainter& painter, BitmapImage* image, const QRect& blitRect)
{
    const auto& tilesToRender = mOptions.tilesToBeRendered;

    painter.save();

    ImageCompositor compositor(mCanvasRect, image->topLeft(), mViewTransform);

    compositor.addImage(*image->image());
    auto output = compositor.output();

    QPainter imagePaintDevice(&output);

    imagePaintDevice.setTransform(mViewTransform);

    // FIXME: polyline tool rendering broken after optimizing painting backend
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

bool CanvasPainter::isRectInsideCanvas(const QRect& rect) const
{
    return mCanvasRect.adjusted(-rect.width(),
                                    -rect.width(),
                                    rect.width(),
                                    rect.width()).contains(rect);
}

void CanvasPainter::paintVectorFrame(QPainter& painter,
                                     Layer* layer,
                                     int nFrame,
                                     bool useLastKeyFrame,
                                     bool isCurrentFrame)
{
#ifdef _DEBUG
    LayerVector* vectorLayer = dynamic_cast<LayerVector*>(layer);
    Q_ASSERT(vectorLayer);
#else
    LayerVector* vectorLayer = static_cast<LayerVector*>(layer);
#endif

    CANVASPAINTER_LOG("Paint Onion skin vector, Frame = %d", nFrame);
    VectorImage* vectorImage = nullptr;
    if (useLastKeyFrame)
    {
        vectorImage = vectorLayer->getLastVectorImageAtFrame(nFrame, 0);
    }
    else
    {
        vectorImage = vectorLayer->getVectorImageAtFrame(nFrame);
    }
    if (vectorImage == nullptr)
    {
        return;
    }

    ImageCompositor compositor(mCanvasRect, mBuffer->topLeft(), QTransform());

    // Paint vector image to offscreen buffer, strokeImage.
    vectorImage->outputImage(&compositor.output(), mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    compositor.addImage(*mBuffer->image());

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    painter.drawImage(QPoint(), compositor.output());
}

void CanvasPainter::paintTransformedSelection(QPainter& painter) const
{
    // Make sure there is something selected
    if (mSelection.width() == 0 || mSelection.height() == 0)
        return;

    Layer* layer = mObject->getLayer(mCurrentLayerIndex);

    if (layer->type() == Layer::BITMAP)
    {
        // Get the transformed image
        BitmapImage* bitmapImage = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(mFrameNumber, 0);
        if (bitmapImage == nullptr) { return; };
        BitmapImage transformedImage = bitmapImage->transformed(mSelection, mSelectionTransform, mOptions.bAntiAlias);

        // Paint the transformation output
        painter.setWorldMatrixEnabled(true);
        transformedImage.paintImage(painter);
    }
}

/** Paints layers within the specified range for the current frame.
 *
 *  @param painter The painter to paint to
 *  @param startLayer The first layer to paint (inclusive)
 *  @param endLayer The last layer to paint (inclusive)
 */
void CanvasPainter::paintCurrentFrameAtLayer(QPainter& painter, int startLayer, int endLayer)
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
        case Layer::BITMAP: { paintBitmapFrame(painter, layer, mFrameNumber, true, isCurrentLayer); break; }
        case Layer::VECTOR: { paintVectorFrame(painter, layer, mFrameNumber, true, isCurrentLayer); break; }
        default: break;
        }
    }
}
