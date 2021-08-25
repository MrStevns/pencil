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
#include "layercamera.h"
#include "vectorimage.h"
#include "util.h"
#include <imagecompositor.h>

CanvasPainter::CanvasPainter()
{
}

CanvasPainter::~CanvasPainter()
{
}

void CanvasPainter::setCanvas(QPixmap* canvas)
{
    Q_ASSERT(canvas);
    mCanvas = canvas;
    mCanvasRect = mCanvas->rect();
}

void CanvasPainter::setViewTransform(const QTransform view)
{
    if (mViewTransform != view) {
        mViewTransform = view;
    }
}

void CanvasPainter::setTransformedSelection(QRect selectionBeforeTransform, QRect currentSeletion, QTransform transform, bool modified)
{
    // Make sure that the selection is not empty
    if (selectionBeforeTransform.width() > 0 && selectionBeforeTransform.height() > 0)
    {
        mStartSelection = selectionBeforeTransform;
        mCurrentSelection = currentSeletion;
        mSelectionTransform = transform;
        mRenderTransform = true;
        mSelectionModified = modified;
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
    mModifiedTransformSet = false;
}

void CanvasPainter::paintCached()
{
    QPixmap tempPixmap(mCanvas->size());
    tempPixmap.fill(Qt::transparent);
    mCanvas->fill(Qt::transparent);
    QPainter tempPainter;
    QPainter painter;
    initializePainter(tempPainter, tempPixmap);
    initializePainter(painter, *mCanvas);

    if (!mPreLayersCache)
    {
        renderPreLayers(painter);
        mPreLayersCache.reset(new QPixmap(*mCanvas));
    }
    else
    {
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(0, 0, *(mPreLayersCache.get()));
        painter.setWorldMatrixEnabled(true);
    }

    renderCurLayer(painter);

    if (!mPostLayersCache)
    {
        renderPostLayers(tempPainter);
        mPostLayersCache.reset(new QPixmap(tempPixmap));
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(0, 0, tempPixmap);
        painter.setWorldMatrixEnabled(true);
    }
    else
    {
        painter.setWorldMatrixEnabled(false);
        painter.drawPixmap(0, 0, *(mPostLayersCache.get()));
        painter.setWorldMatrixEnabled(true);
    }
}

void CanvasPainter::resetLayerCache()
{
    mPreLayersCache.reset();
    mPostLayersCache.reset();
}

void CanvasPainter::initializePainter(QPainter& painter, QPixmap& pixmap)
{
    painter.begin(&pixmap);
    painter.setWorldMatrixEnabled(true);
    painter.setWorldTransform(mViewTransform);
}

void CanvasPainter::renderPreLayers(QPixmap *pixmap)
{
    QPainter painter;
    initializePainter(painter, *pixmap);
    renderPreLayers(painter);
}

void CanvasPainter::renderPreLayers(QPainter& painter)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrameAtLayer(painter, 0, mCurrentLayerIndex-1);
    }

    paintOnionSkin(painter);
    painter.setOpacity(1.0);
}

void CanvasPainter::renderCurLayer(QPixmap *pixmap)
{
    QPainter painter;
    initializePainter(painter, *pixmap);
    renderCurLayer(painter);
}

void CanvasPainter::renderCurLayer(QPainter& painter)
{
    int currentLayerIndex = mCurrentLayerIndex;
    Layer* layer = mObject->getLayer(currentLayerIndex);

    painter.setOpacity(1.0);

    bool isCameraLayer = layer->type() == Layer::CAMERA;
    if (layer->visible() == false)
        return;

    if (mOptions.eLayerVisibility == LayerVisibility::RELATED && !isCameraLayer) {
        painter.setOpacity(calculateRelativeOpacityForLayer(currentLayerIndex));
    }

    switch (layer->type())
    {
    case Layer::BITMAP: { paintCurrentBitmapFrame(painter, layer); break; }
    case Layer::VECTOR: { paintVectorFrame(painter, layer, mFrameNumber, false, true, true); break; } // TODO: do same refactoring for vector..
    default: break;
    }
}

void CanvasPainter::renderPostLayers(QPixmap *pixmap)
{
    QPainter painter;
    initializePainter(painter, *pixmap);
    renderPostLayers(painter);
}

void CanvasPainter::renderPostLayers(QPainter& painter)
{
    if (mOptions.eLayerVisibility != LayerVisibility::CURRENTONLY || mObject->getLayer(mCurrentLayerIndex)->type() == Layer::CAMERA)
    {
        paintCurrentFrameAtLayer(painter, mCurrentLayerIndex+1, mObject->getLayerCount()-1);
    }

    paintCameraBorder(painter);

    // post effects
    if (mOptions.bAxis)
    {
        paintAxis(painter);
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

void CanvasPainter::paint()
{
    QPainter painter;
    initializePainter(painter, *mCanvas);

    renderPreLayers(painter);
    renderCurLayer(painter);
    renderPostLayers(painter);
}

void CanvasPainter::paintBackground()
{
    mCanvas->fill(Qt::transparent);
}

void CanvasPainter::paintOnionSkin(QPainter& painter)
{
    if (!mOptions.onionWhilePlayback && mOptions.isPlaying) { return; }

    Layer* layer = mObject->getLayer(mCurrentLayerIndex);

    if (layer->visible() == false)
        return;

    if (layer->keyFrameCount() == 0)
        return;

    qreal minOpacity = static_cast<qreal>(mOptions.fOnionSkinMinOpacity / 100);
    qreal maxOpacity = static_cast<qreal>(mOptions.fOnionSkinMaxOpacity / 100);

    if (mOptions.bPrevOnionSkin && mFrameNumber > 1)
    {
        // Paint onion skin before current frame.
        qreal prevOpacityIncrement = (maxOpacity - minOpacity) / mOptions.nPrevOnionSkinCount;
        qreal opacity = maxOpacity;

        int onionFrameNumber = mFrameNumber;
        if (mOptions.bIsOnionAbsolute)
        {
            onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber + 1, true);
        }
        onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber, mOptions.bIsOnionAbsolute);

        int onionPosition = 0;

        while (onionPosition < mOptions.nPrevOnionSkinCount && onionFrameNumber > 0)
        {
            painter.setOpacity(opacity);

            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapOnionSkinFrame(painter, layer, onionFrameNumber, mOptions.bColorizePrevOnion); break; }
            case Layer::VECTOR: { paintVectorFrame(painter, layer, onionFrameNumber, mOptions.bColorizePrevOnion, false, false); break; }
            default: break;
            }
            opacity = opacity - prevOpacityIncrement;

            onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber, mOptions.bIsOnionAbsolute);
            onionPosition++;
        }
    }

    if (mOptions.bNextOnionSkin)
    {
        // Paint onion skin after current frame.
        qreal nextOpacityIncrement = (maxOpacity - minOpacity) / mOptions.nNextOnionSkinCount;
        qreal opacity = maxOpacity;

        int onionFrameNumber = layer->getNextFrameNumber(mFrameNumber, mOptions.bIsOnionAbsolute);
        int onionPosition = 0;

        while (onionPosition < mOptions.nNextOnionSkinCount && onionFrameNumber > 0)
        {
            painter.setOpacity(opacity);

            switch (layer->type())
            {
            case Layer::BITMAP: { paintBitmapOnionSkinFrame(painter, layer, onionFrameNumber, mOptions.bColorizeNextOnion); break; }
            case Layer::VECTOR: { paintVectorFrame(painter, layer, onionFrameNumber, mOptions.bColorizeNextOnion, false, false); break; }
            default: break;
            }
            opacity = opacity - nextOpacityIncrement;

            onionFrameNumber = layer->getNextFrameNumber(onionFrameNumber, mOptions.bIsOnionAbsolute);
            onionPosition++;
        }
    }
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
    QColor colorizeColor = Qt::black;
    if (colorize) {
        if (nFrame < mFrameNumber)
        {
            colorizeColor = Qt::red;
        }
        else if (nFrame > mFrameNumber)
        {
            colorizeColor = Qt::blue;
        }
    }

    compositor.addEffect(CompositeEffect::Colorize, colorizeColor);

    painter.drawImage(QPoint(), compositor.output());
    painter.restore();
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

void CanvasPainter::paintCurrentBitmapFrame(QPainter& painter, Layer* layer)
{
    LayerBitmap* bitmapLayer = static_cast<LayerBitmap*>(layer);
    BitmapImage* bitmapImage = bitmapLayer->getLastBitmapImageAtFrame(mFrameNumber);

    if (bitmapImage == nullptr) { return; }
    bool isPainting = mOptions.isPainting;

    painter.setWorldMatrixEnabled(false);
    // Only paint with tiles for the frame we are painting on
    if (isPainting) {
        paintBitmapTiles(painter, bitmapImage);
    } else {
        painter.save();
        ImageCompositor compositor(mCanvasRect, bitmapImage->topLeft(), mViewTransform);
        compositor.addImage(*bitmapImage->image());

        if (mRenderTransform) {
            compositor.addEffect(CompositeEffect::Transformation, mSelectionTransform, mStartSelection);
        }
        auto output = compositor.output();

        painter.drawImage(QPoint(), compositor.output());

        painter.restore();
    }
}

void CanvasPainter::paintBitmapTiles(QPainter& painter, BitmapImage* image)
{
    auto tilesToRender = mOptions.tilesToBeRendered;

    painter.save();

    ImageCompositor compositor(mCanvasRect, image->topLeft(), mViewTransform);

    compositor.addImage(*image->image());
    auto output = compositor.output();

    QPainter imagePaintDevice(&output);

    imagePaintDevice.translate(-painter.viewport().width()/2, -painter.viewport().height()/2);
    imagePaintDevice.setTransform(mViewTransform);
    for (MPTile* item : tilesToRender) {
        const QPixmap& pix = item->pixmap();

        QRect tileRect = QRect(item->pos(), item->pixmap().size());

        if (mOptions.useCanvasBuffer) {
            imagePaintDevice.setCompositionMode(QPainter::CompositionMode_SourceOver);
        } else {
            imagePaintDevice.setCompositionMode(QPainter::CompositionMode_Source);
        }
        imagePaintDevice.drawPixmap(tileRect, pix, pix.rect());
    }

    painter.drawImage(QPoint(), output);
    painter.restore();
}

bool CanvasPainter::isRectInsideCanvas(const QRect& rect) const
{
    return mCanvasRect.adjusted(-rect.width(),
                                    -rect.width(),
                                    rect.width(),
                                    rect.width()).contains(rect);
}

void CanvasPainter::prescale(BitmapImage* bitmapImage)
{
    QImage origImage = bitmapImage->image()->copy();

    // copy content of our unmodified qimage
    // to our (not yet) scaled bitmap
    mScaledBitmap = origImage.copy();

    if (mOptions.scaling >= 1.0f)
    {
        // TODO: Qt doesn't handle huge upscaled qimages well...
        // possible solution, myPaintLib canvas renderer splits its canvas up in chunks.
    }
    else
    {
        // map to correct matrix
        QRect mappedOrigImage = mViewTransform.mapRect(bitmapImage->bounds());
        mScaledBitmap = mScaledBitmap.scaled(mappedOrigImage.size(),
                                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
}

void CanvasPainter::paintVectorFrame(QPainter& painter,
                                     Layer* layer,
                                     int nFrame,
                                     bool colorize,
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

    QImage* strokeImage = new QImage(mCanvas->size(), QImage::Format_ARGB32_Premultiplied);
    strokeImage->fill(Qt::transparent);
    vectorImage->outputImage(strokeImage, mViewTransform, mOptions.bOutlines, mOptions.bThinLines, mOptions.bAntiAlias);

    // Go through a Bitmap image to paint the onion skin colour
    BitmapImage rasterizedVectorImage;
    rasterizedVectorImage.setImage(strokeImage);

    if (colorize)
    {
        QBrush colorBrush = QBrush(Qt::transparent); //no color for the current frame

        if (nFrame < mFrameNumber)
        {
            colorBrush = QBrush(Qt::red);
        }
        else if (nFrame > mFrameNumber)
        {
            colorBrush = QBrush(Qt::blue);
        }
        rasterizedVectorImage.drawRect(strokeImage->rect(),
                                 Qt::NoPen, colorBrush,
                                 QPainter::CompositionMode_SourceIn, false);
    }

    // Don't transform the image here as we used the viewTransform in the image output
    painter.setWorldMatrixEnabled(false);

    painter.setOpacity(vectorImage->getOpacity() - (1.0-painter.opacity()));
    if (isCurrentFrame)
    {
        // Paste buffer onto image to see stroke in realtime
        rasterizedVectorImage.paste(mBuffer, mOptions.cmBufferBlendMode);
    }

    // Paint buffer pasted on top of vector image:
    // fixes polyline not being rendered properly
    rasterizedVectorImage.paintImage(painter);
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

        if (mOptions.eLayerVisibility == LayerVisibility::RELATED && !isCameraLayer) {
            painter.setOpacity(calculateRelativeOpacityForLayer(i));
        }

        bool isCurrentLayer = i == mCurrentLayerIndex;

        switch (layer->type())
        {
        case Layer::BITMAP: { paintBitmapFrame(painter, layer, mFrameNumber, true, isCurrentLayer); break; }
        case Layer::VECTOR: { paintVectorFrame(painter, layer, mFrameNumber, false, true, isCurrentLayer); break; }
        default: break;
        }
    }
}

qreal CanvasPainter::calculateRelativeOpacityForLayer(int layerIndex) const
{
    int layerOffset = mCurrentLayerIndex - layerIndex;
    int absoluteOffset = qAbs(layerOffset);
    qreal newOpacity = 1.0;
    if (absoluteOffset != 0) {
        newOpacity = qPow(static_cast<qreal>(mOptions.fLayerVisibilityThreshold), absoluteOffset);
    }
    return newOpacity;
}

void CanvasPainter::paintAxis(QPainter& painter)
{
    painter.setPen(Qt::green);
    painter.drawLine(QLineF(0, -500, 0, 500));

    painter.setPen(Qt::red);
    painter.drawLine(QLineF(-500, 0, 500, 0));
}

int round100(double f, int gridSize)
{
    return static_cast<int>(f) / gridSize * gridSize;
}

void CanvasPainter::paintGrid(QPainter& painter)
{
    int gridSizeW = mOptions.nGridSizeW;
    int gridSizeH = mOptions.nGridSizeH;

    QRectF rect = painter.viewport();
    QRectF boundingRect = mViewTransform.inverted().mapRect(rect);

    int left = round100(boundingRect.left(), gridSizeW) - gridSizeW;
    int right = round100(boundingRect.right(), gridSizeW) + gridSizeW;
    int top = round100(boundingRect.top(), gridSizeH) - gridSizeH;
    int bottom = round100(boundingRect.bottom(), gridSizeH) + gridSizeH;

    QPen pen(Qt::lightGray);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);
    QPainter::RenderHints previous_renderhints = painter.renderHints();
    painter.setRenderHint(QPainter::Antialiasing, false);
    // draw vertical gridlines
    for (int x = left; x < right; x += gridSizeW)
    {
        painter.drawLine(x, top, x, bottom);
    }

    // draw horizontal gridlines
    for (int y = top; y < bottom; y += gridSizeH)
    {
        painter.drawLine(left, y, right, y);
    }
    painter.setRenderHints(previous_renderhints);
}

void CanvasPainter::renderGrid(QPainter& painter)
{
    if (mOptions.bGrid)
    {
        painter.save();
        painter.setWorldTransform(mViewTransform);
        paintGrid(painter);
        painter.restore();
    }
}

void CanvasPainter::paintCameraBorder(QPainter& painter)
{
    LayerCamera* cameraLayer = nullptr;
    bool isCameraMode = false;

    // Find the first visiable camera layers
    for (int i = 0; i < mObject->getLayerCount(); ++i)
    {
        Layer* layer = mObject->getLayer(i);
        if (layer->type() == Layer::CAMERA && layer->visible())
        {
            cameraLayer = static_cast<LayerCamera*>(layer);
            isCameraMode = (i == mCurrentLayerIndex);
            break;
        }
    }

    if (cameraLayer == nullptr) { return; }

    QRectF viewRect = painter.viewport();
    QRect boundingRect;
    mCameraRect = cameraLayer->getViewRect();

    QRegion rg2(mCameraRect);
    if (isCameraMode)
    {
        painter.setWorldMatrixEnabled(false);
        QTransform center = QTransform::fromTranslate(viewRect.width() / 2.0, viewRect.height() / 2.0);
        boundingRect = viewRect.toAlignedRect();
        mCameraRect = center.mapRect(mCameraRect);
        rg2 = center.map(rg2);
    }
    else
    {
        painter.setWorldMatrixEnabled(true);
        QTransform viewInverse = mViewTransform.inverted();
        boundingRect = viewInverse.mapRect(viewRect).toAlignedRect();

        QTransform camTransform = cameraLayer->getViewAtFrame(mFrameNumber);
        mCameraRect = camTransform.inverted().mapRect(mCameraRect);
        rg2 = camTransform.inverted().map(rg2);
    }

    painter.setOpacity(1.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 80));

    QRegion rg1(boundingRect);
    QRegion rg3 = rg1.subtracted(rg2);

    painter.setClipRegion(rg3);
    painter.drawRect(boundingRect);

    /*
    painter.setClipping(false);

    QPen pen( Qt::black,
                2,
                Qt::SolidLine,
                Qt::FlatCap,
                Qt::MiterJoin );
    painter.setPen( pen );
    painter.setBrush( Qt::NoBrush );
    painter.drawRect( mCameraRect.adjusted( -1, -1, 1, 1) );
    */
}

QRect CanvasPainter::getCameraRect()
{
    return mCameraRect;
}
