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


#ifndef SCRIBBLEAREA_H
#define SCRIBBLEAREA_H

#include <cstdint>
#include <ctime>
#include <deque>
#include <memory>

#include <QColor>
#include <QTransform>
#include <QPoint>
#include <QWidget>
#include <QPixmapCache>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QElapsedTimer>

#include "movemode.h"
#include "log.h"
#include "pencildef.h"
#include "bitmapimage.h"
#include "colorref.h"
#include "vectorselection.h"
#include "canvaspainter.h"
#include "preferencemanager.h"
#include "strokemanager.h"
#include "selectionpainter.h"

#include "brushsetting.h"


class Layer;
class Editor;
class BaseTool;
class PointerEvent;
class BitmapImage;
class VectorImage;

class MPHandler;
class MPSurface;
class MPTile;
class QGraphicsPixmapItem;

class ScribbleArea : public QWidget
{
    Q_OBJECT

    friend class MoveTool;
    friend class EditTool;
    friend class SmudgeTool;
    friend class BucketTool;

public:
    ScribbleArea(QWidget* parent);
    ~ScribbleArea() override;

    bool init();
    void setEditor(Editor* e) { mEditor = e; }
    StrokeManager* getStrokeManager() const { return mStrokeManager.get(); }
    Editor* editor() const { return mEditor; }



    void deleteSelection();
    void applySelectionChanges();
    void displaySelectionProperties();

    void paintTransformedSelection();
    void applyTransformedSelection();
    void cancelTransformedSelection();

    bool isLayerPaintable() const;

    QVector<QPoint> calcSelectionCenterPoints();

    void setEffect(SETTING e, bool isOn);

    LayerVisibility getLayerVisibility() const { return mLayerVisibility; }
    qreal getCurveSmoothing() const { return mCurveSmoothingLevel; }
    bool usePressure() const { return mUsePressure; }
    bool makeInvisible() const { return mMakeInvisible; }

    QRectF getCameraRect();
    QPointF getCentralPoint();

    void updateCurrentFrame();

    /** Check if the cache should be invalidated for all frames since the last paint operation
     */
    void updateAllFramesIfNeeded();
    void updateFrame(int frame);

    void updateAllFrames();
    void updateAllVectorLayersAtCurrentFrame();
    void updateAllVectorLayersAt(int frameNumber);

    void setModified(int layerNumber, int frameNumber);
    bool shouldUpdateAll() const { return mNeedUpdateAll; }
    void setAllDirty();

    void flipSelection(bool flipVertical);

    BaseTool* currentTool() const;
    BaseTool* getTool(ToolType eToolMode);
    void currentToolSet(ToolType eToolMode);

    void floodFillError(int errorType);

    bool isMouseInUse() const { return mMouseInUse; }
    bool isTabletInUse() const { return mTabletInUse; }
    bool isPointerInUse() const { return mMouseInUse || mTabletInUse; }

    void showCurrentFrame();

    /**
     * @brief prepareForDrawing
     * Used to get the current frame content into mypaint
     */
    void prepareForDrawing();

    // mypaint
    void loadMPBrush(const QByteArray &content);
    void brushSettingChanged(BrushSettingType settingType, float value);
    float getBrushSettingBaseValue(BrushSettingType settingType);

    const BrushSettingInfo getBrushSettingInfo(BrushSettingType setting);
    const BrushInputInfo getBrushInputInfo(BrushInputType input);

    void setBrushInputMapping(QVector<QPointF> points, BrushSettingType settingType, BrushInputType inputType);
    const BrushInputMapping getBrushInputMapping(BrushSettingType settingType, BrushInputType inputType);

    /** Check if the content of the canvas depends on the active layer.
      *
      * Currently layers are only affected by Onion skins are displayed only for the active layer, and the opacity of all layers
      * is affected when relative layer visiblity is active.
      *
      * @return True if the active layer could potentially influence the content of the canvas. False otherwise.
      */
    bool isAffectedByActiveLayer() const;

    void keyEvent(QKeyEvent* event);
    void keyEventForSelection(QKeyEvent* event);

signals:
    void modification(int);
    void multiLayerOnionSkinChanged(bool);
    void refreshPreview();

public slots:
    void clearCanvas();

    void setCurveSmoothing(int);
    void toggleThinLines();
    void toggleOutlines();
    void increaseLayerVisibilityIndex();
    void decreaseLayerVisibilityIndex();
    void setLayerVisibility(LayerVisibility visibility);

    void updateToolCursor();
    void paletteColorChanged(QColor);

    void showLayerNotVisibleWarning();

    void updateDirtyTiles();
    void refreshSurface();

    void updateTile(MPSurface* surface, MPTile* tile);
    void clearTile(MPSurface *surface, MPTile *tile);


protected:
    bool event(QEvent *event) override;
    void tabletEvent(QTabletEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;

public:
    void startStroke();
    void strokeTo(QPointF point, float pressure, float xtilt, float ytilt);
    void endStroke();
    QColor pickColorFromSurface(QPointF point, int radius);

    void drawPolyline(QPainterPath path, QPen pen, bool useAA);
    void drawLine(QPointF P1, QPointF P2, QPen pen, QPainter::CompositionMode cm);
    void drawPath(QPainterPath path, QPen pen, QBrush brush, QPainter::CompositionMode cm);

    void paintBitmapBuffer(QPainter::CompositionMode composition = QPainter::CompositionMode_Source);
    void paintCanvasCursor(QPainter& painter);
    void paintCanvasCursor();
    void clearBitmapBuffer();
    void refreshBitmap(const QRectF& rect, int rad);
    void refreshVector(const QRectF& rect, int rad);
    void setGaussianGradient(QGradient &gradient, QColor colour, qreal opacity, qreal offset);

    void pointerPressEvent(PointerEvent*);
    void pointerMoveEvent(PointerEvent*);
    void pointerReleaseEvent(PointerEvent*);

    void updateCanvasCursor();

    void clearTilesBuffer();

    void layerChanged();

    /// Call this when starting to use a paint tool. Checks whether we are drawing
    /// on an empty frame, and if so, takes action according to use preference.
    void handleDrawingOnEmptyFrame();

    void setIsPainting(bool isPainting) { mIsPainting = isPainting; }

    BitmapImage* mBufferImg = nullptr; // used to pre-draw vector modifications

    QPixmap mCursorImg;
    QPixmap mTransCursImg;

    MPHandler* mMyPaint = nullptr;
    QHash<QString, MPTile*> mBufferTiles;
private:

    /** reloadMyPaint
     * Use this method whenver the mypaint surface should be cleared
     * eg. when changing layer
     */
    void reloadMyPaint();

    /** forceUpdateMyPaintStates
     * Use this method if you intend to update brush states without causing a stroke on the canvas
     */
    void forceUpdateMyPaintStates();

    /** updatePixmapCache
     * Updates the qpixmap cache that is used for getting the canvas images from cache.
     */
    void updatePixmapCache(const int frame);
    
    /** remove cache for dirty keyframes */
    void removeCacheForDirtyFrames();

    /** remove onion skin cache around frame */
    void removeOnionSkinsCacheAround(int frame);

    void prepCanvas(int frame, QRect rect);
    void settingUpdated(SETTING setting);
    void paintSelectionVisuals(QPainter &painter);

    void drawCanvas(int frame);

    /**
     * @brief ScribbleArea::calculateDeltaTime
     * calculates the number of seconds that has passed between the previous and current frame
     * should be called from paintEvent
     */
    qreal calculateDeltaTime();

    QString getCachedFrameKey(int frame);

    BitmapImage* currentBitmapImage(Layer* layer) const;
    VectorImage* currentVectorImage(Layer* layer) const;

    MoveMode mMoveMode = MoveMode::NONE;

    BitmapImage mBitmapSelection; // used to temporary store a transformed portion of a bitmap image

    std::unique_ptr<StrokeManager> mStrokeManager;

    Editor* mEditor = nullptr;

    /// Used to load frame into mypaint. Should only be true when the surface is not valid anymore
    bool mNeedLoadImageToMyPaint = false;

    bool mIsSimplified = false;
    bool mShowThinLines = false;
    bool mQuickSizing = true;
    LayerVisibility mLayerVisibility = LayerVisibility::ALL;
    bool mUsePressure   = true;
    bool mMakeInvisible = false;
    bool mToolCursors = true;
    qreal mCurveSmoothingLevel = 0.0;
    bool mMultiLayerOnionSkin = false; // future use. If required, just add a checkbox to updated it.
    QColor mOnionColor;

    bool mNeedUpdateAll = false;

    /**
     * @brief ScribbleArea::updateMyPaintCanvas
     * Loads an image into libmypaint
       should preferably only be used when loading new content that is otherwise not added automatically.
     */
    void updateMyPaintCanvas(BitmapImage* bitmapImage = nullptr);

private:
    bool mKeyboardInUse = false;
    bool mMouseInUse = false;
    bool mMouseRightButtonInUse = false;
    bool mTabletInUse = false;

    // Double click handling for tablet input
    void handleDoubleClick();
    bool mIsFirstClick = true;
    int mDoubleClickMillis = 0;
    // Microsoft suggests that a double click action should be no more than 500 ms
    const int DOUBLE_CLICK_THRESHOLD = 500;
    QTimer* mDoubleClickTimer;

    QPoint mCursorCenterPos;

    QPointF mTransformedCursorPos;

    bool mIsPainting = false;

    PreferenceManager* mPrefs = nullptr;

    QPixmap mCanvas;
    CanvasPainter mCanvasPainter;
    SelectionPainter mSelectionPainter;

    // Pixmap Cache keys
    QMap<unsigned int, QPixmapCache::Key> mPixmapCacheKeys;

    // debug
    QRectF mDebugRect;
    std::deque<clock_t> mDebugTimeQue;

    bool isInPreviewMode = false;
    bool mNeedQuickUpdate = false;

    QElapsedTimer deltaTimer;
    int lastFrameTime;
    int currentFrameTime;
    double deltaTime = 0;

    QRect debugBlitRect;
};

#endif
