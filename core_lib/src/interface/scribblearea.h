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
#include "overlaypainter.h"
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
    friend class StrokeTool;

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

    /** Update current frame.
     *  calls update() behind the scene and update cache if necessary */
    void updateCurrentFrame();
    /** Update frame.
     * calls update() behind the scene and update cache if necessary */
    void updateFrame(int frame);

    /** Frame scrubbed, invalidate relevant cache */
    void onScrubbed(int frameNumber);

    /** Multiple frames modified, invalidate cache for affected frames */
    void onFramesModified();

    /** Playstate changed, invalidate relevant cache */
    void onPlayStateChanged();

    /** View updated, invalidate relevant cache */
    void onViewChanged();

    /** Frame modified, invalidate cache for frame if any */
    void onFrameModified(int frameNumber);

    /** Current frame modified, invalidate current frame cache if any.
     * Convenient function that does the same as onFrameModified */
    void onCurrentFrameModified();

    /** Layer changed, invalidate relevant cache */
    void onLayerChanged();

    /** Selection was changed, keep cache */
    void onSelectionChanged();

    /** Onion skin type changed, all frames will be affected.
     * All cache will be invalidated */
    void onOnionSkinTypeChanged();

    /** Object updated, invalidate all cache */
    void onObjectLoaded();

    /** After applying a stroke,
     * note: optimization to avoid clearing mypaint when we draw on the canvas */
    void onDidDraw(int frameNumber);

    /** Set frame on layer to modified and invalidate current frame cache */
    void setModified(int layerNumber, int frameNumber);
    void setModified(const Layer* layer, int frameNumber);

    void flipSelection(bool flipVertical);
    void renderOverlays();
    void prepOverlays();

    BaseTool* currentTool() const;
    BaseTool* getTool(ToolType eToolMode);
    void setCurrentTool(ToolType eToolMode);

    void floodFillError(int errorType);

    bool isMouseInUse() const { return mMouseInUse; }
    bool isTabletInUse() const { return mTabletInUse; }
    bool isPointerInUse() const { return mMouseInUse || mTabletInUse; }

    /**
     * @brief prepareForDrawing
     * Used to get the current frame content into mypaint
     */
    void prepareForDrawing();
    void drawCanvas(int frame, QRect rect);

    // mypaint
    void loadMPBrush(const QByteArray &content);
    void brushSettingChanged(BrushSettingType settingType, float value);
    float getBrushSettingBaseValue(BrushSettingType settingType);

    const BrushSettingInfo getBrushSettingInfo(BrushSettingType setting);
    const BrushInputInfo getBrushInputInfo(BrushInputType input);

    void setBrushInputMapping(QVector<QPointF> points, BrushSettingType settingType, BrushInputType inputType);
    const BrushInputMapping getBrushInputMapping(BrushSettingType settingType, BrushInputType inputType);

    void keyEvent(QKeyEvent* event);
    void keyEventForSelection(QKeyEvent* event);

signals:
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
    void strokeTo(QPointF point, float pressure, float xtilt, float ytilt, double dt);
    void endStroke();

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

    /** Invalidate the layer pixmap cache.
     * Call this in most situations where the layer rendering order is affected.
     * Peviously known as setAllDirty.
    */
    void invalidateLayerPixmapCache();

    /** Invalidate cache for the given frame */
    void invalidateCacheForFrame(int frameNumber);

    /** Invalidate all cache.
     * call this if you're certain that the change you've made affects all frames */
    void invalidateAllCache();

    /** invalidate cache for dirty keyframes. */
    void invalidateCacheForDirtyFrames();

    /** invalidate onion skin cache around frame */
    void invalidateOnionSkinsCacheAround(int frame);

    /** reloadMyPaint
     * Use this method whenver the mypaint surface should be cleared
     * eg. when changing layer
     */
    void reloadMyPaint();

    /** forceUpdateMyPaintStates
     * Use this method if you intend to update brush states without causing a stroke on the canvas
     */
    void forceUpdateMyPaintStates();

    void prepCanvas(int frame, QRect rect);
    void settingUpdated(SETTING setting);
    void paintSelectionVisuals(QPainter &painter);

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
    OverlayPainter mOverlayPainter;
    SelectionPainter mSelectionPainter;

    // Pixmap Cache keys
    QMap<unsigned int, QPixmapCache::Key> mPixmapCacheKeys;

    // debug
    QLoggingCategory mLog{ "ScribbleArea" };
};

#endif
