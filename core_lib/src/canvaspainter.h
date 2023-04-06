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

#ifndef CANVASPAINTER_H
#define CANVASPAINTER_H

#include <memory>
#include <QCoreApplication>
#include <QObject>
#include <QTransform>
#include <QPainter>
#include "log.h"
#include "pencildef.h"

#include "imagepainter.h"

#include "layer.h"

#include "onionskinpainteroptions.h"
#include "onionskinsubpainter.h"

class Object;
class BitmapImage;
class ViewManager;
class MPTile;

struct CanvasPainterOptions
{
    bool  bAntiAlias = false;
    bool  bThinLines = false;
    bool  bOutlines = false;
    LayerVisibility eLayerVisibility = LayerVisibility::RELATED;
    float fLayerVisibilityThreshold = 0.f;
    float scaling = 1.0f;
    
    // TODO: verify whether we still need this
    bool isPlaying = false;
    bool onionWhilePlayback = false;
    bool isPainting = false;
    //

    QList<MPTile*> tilesToBeRendered;
    bool useCanvasBuffer;

    QPainter::CompositionMode cmBufferBlendMode = QPainter::CompositionMode_SourceOver;
    OnionSkinPainterOptions mOnionSkinOptions;
};

class CanvasPainter
{
    Q_DECLARE_TR_FUNCTIONS(CanvasPainter)
public:
    explicit CanvasPainter();
    virtual ~CanvasPainter();

    void setCanvas(QPixmap* canvas);
    void setViewTransform(const QTransform view, const QTransform viewInverse);

    void setOnionSkinOptions(const OnionSkinPainterOptions& onionSkinOptions) { mOnionSkinPainterOptions = onionSkinOptions;}
    void setOptions(const CanvasPainterOptions& p) { mOptions = p; }
    void setTransformedSelection(QRect selection, QTransform transform);
    void ignoreTransformedSelection();

    void setPaintSettings(const Object* object, int currentLayer, int frame, QRect rect, BitmapImage* buffer);
    void paint(const QRect& blitRect);
    void paintCached(const QRect& blitRect);
    void resetLayerCache();

private:

    /**
     * CanvasPainter::initializePainter
     * Enriches the painter with a context and sets it's initial matrix.
     * @param painter The in/out painter
     * @param pixmap The paint device ie. a pixmap
     */
    void initializePainter(QPainter& painter, QPaintDevice& device, const QRect& blitRect);

    void renderPreLayers(QPainter& painter);
    void renderCurLayer(QPainter& painter, const QRect& blitRect);
    void renderPostLayers(QPainter& painter);

    void paintBackground();
    void paintOnionSkin(QPainter& painter);

    void paintCurrentFrameAtLayer(QPainter& painter, int startLayer, int endLayer);

    /** paintCurrentBitmapFrame
     * This should only be used for the current frame at the current layer
     * @param painter
     * @param layer
     */
    void paintCurrentBitmapFrame(QPainter& painter, Layer* layer, const QRect& blitRect);

    /** paintBitmapTiles
     * Paints tiled images on top of the given bitmap image and canvas painter if needed.
     * @param painter
     * @param image
     */
    void paintBitmapTiles(QPainter& painter, BitmapImage* image, const QRect& blitRect);

    /** paintBitmapFrame
     * Paints bitmap frames to any given layer, do not use on current frame at current layer.
     * @param layer
     * @param nFrame
     * @param colorize
     * @param isCurrentLayer
     * @param isCurrentFrame
     */
    void paintBitmapFrame(QPainter&, Layer* layer, int nFrame, bool isCurrentFrame, bool isCurrentLayer);
    void paintVectorFrame(QPainter&, Layer* layer, int nFrame, bool colorize, bool useLastKeyFrame, bool isCurrentFrame);

    void paintTransformedSelection(QPainter& painter) const;

    /** Check if the given rect lies inside the canvas, assumes that the input rect is mapped correctly **/
    inline bool isRectInsideCanvas(const QRect& rect) const;

private:
    void paintBitmapOnionSkinFrame(QPainter& painter, Layer* layer, int nFrame, bool colorize);

    CanvasPainterOptions mOptions;

    const Object* mObject = nullptr;
    QPixmap* mCanvas = nullptr;

    // Caches specifically for when drawing on the canvas
    QPixmap mPostLayersPixmap;
    QPixmap mPreLayersPixmap;
    bool mPreLayersPixmapCacheValid = false;
    bool mPostLayersPixmapCacheValid = false;

    QTransform mViewTransform;
    QTransform mViewInvTransform;

    QRect mCameraRect;
    QRect mCanvasRect;

    int mCurrentLayerIndex = 0;
    int mFrameNumber = 0;
    BitmapImage* mBuffer = nullptr;

    // Handle selection transformation
    bool mRenderTransform = false;
    QRect mSelection;
    QTransform mSelectionTransform;

    OnionSkinSubPainter mOnionSkinSubPainter;
    OnionSkinPainterOptions mOnionSkinPainterOptions;

    QPaintDevice* mPaintDevice = nullptr;

    const static int OVERLAY_SAFE_CENTER_CROSS_SIZE = 25;
};

#endif // CANVASRENDERER_H
