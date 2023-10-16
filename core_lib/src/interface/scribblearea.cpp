﻿/*

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

#include "scribblearea.h"

#include <cmath>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPixmapCache>
#include <QVBoxLayout>

#include "pointerevent.h"
#include "beziercurve.h"
#include "object.h"
#include "editor.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "layercamera.h"
#include "bitmapimage.h"
#include "vectorimage.h"
#include "blitrect.h"
#include "tile.h"

#include "onionskinpainteroptions.h"

#include "colormanager.h"
#include "toolmanager.h"
#include "strokemanager.h"
#include "layermanager.h"
#include "playbackmanager.h"
#include "viewmanager.h"
#include "selectionmanager.h"
#include "overlaymanager.h"

#include "mphandler.h"
#include "mpbrush.h"
#include "mpsurface.h"
#include "mptile.h"

ScribbleArea::ScribbleArea(QWidget* parent) : QWidget(parent), mCanvasPainter(mCanvas), mCameraPainter(mCanvas)
{
    setObjectName("ScribbleArea");

    // Qt::WA_StaticContents ensure that the widget contents are rooted to the top-left corner
    // and don't change when the widget is resized.
    setAttribute(Qt::WA_StaticContents);

    mStrokeManager.reset(new StrokeManager);
}

ScribbleArea::~ScribbleArea()
{
}

bool ScribbleArea::init()
{
    mPrefs = mEditor->preference();
    mDoubleClickTimer = new QTimer(this);
    mMouseFilterTimer = new QTimer(this);
    mMyPaint = new MPHandler();

    connect(mMyPaint, &MPHandler::tileAdded, this, &ScribbleArea::loadTile);
    connect(mMyPaint, &MPHandler::tileUpdated, this, &ScribbleArea::updateTile);
    connect(mMyPaint, &MPHandler::tileCleared, this, &ScribbleArea::clearTile);

    connect(&mTiledBuffer, &TiledBuffer::tileUpdated, this, &ScribbleArea::onTileUpdated);
    connect(&mTiledBuffer, &TiledBuffer::tileCreated, this, &ScribbleArea::onTileCreated);

    connect(mPrefs, &PreferenceManager::optionChanged, this, &ScribbleArea::settingUpdated);
    connect(mEditor->tools(), &ToolManager::toolPropertyChanged, this, &ScribbleArea::onToolPropertyUpdated);
    connect(mEditor->tools(), &ToolManager::toolChanged, this, &ScribbleArea::onToolChanged);

    connect(mDoubleClickTimer, &QTimer::timeout, this, &ScribbleArea::handleDoubleClick);
    connect(mMouseFilterTimer, &QTimer::timeout, this, &ScribbleArea::tabletReleaseEventFired);

    connect(mEditor->select(), &SelectionManager::selectionChanged, this, &ScribbleArea::onSelectionChanged);
    connect(mEditor->select(), &SelectionManager::needDeleteSelection, this, &ScribbleArea::deleteSelection);

    mDoubleClickTimer->setInterval(50);
    mMouseFilterTimer->setInterval(50);

    const int curveSmoothingLevel = mPrefs->getInt(SETTING::CURVE_SMOOTHING);
    mCurveSmoothingLevel = curveSmoothingLevel / 20.0; // default value is 1.0

    mQuickSizing = mPrefs->isOn(SETTING::QUICK_SIZING);
    mMakeInvisible = false;

    mMultiLayerOnionSkin = mPrefs->isOn(SETTING::MULTILAYER_ONION);

    mLayerVisibility = static_cast<LayerVisibility>(mPrefs->getInt(SETTING::LAYER_VISIBILITY));

    mDeltaFactor = mEditor->preference()->isOn(SETTING::INVERT_SCROLL_ZOOM_DIRECTION) ? -1 : 1;

    updateCanvasCursor();

    setMouseTracking(true); // reacts to mouse move events, even if the button is not pressed
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    setTabletTracking(true); // tablet tracking first added in 5.9
#endif

    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

    QPixmapCache::setCacheLimit(100 * 1024); // unit is kb, so it's 100MB cache
    mPixmapCacheKeys.clear();

    return true;
}

void ScribbleArea::settingUpdated(SETTING setting)
{
    switch (setting)
    {
    case SETTING::CURVE_SMOOTHING:
        setCurveSmoothing(mPrefs->getInt(SETTING::CURVE_SMOOTHING));
        break;
    case SETTING::TOOL_CURSOR:
        updateToolCursor();
        break;
    case SETTING::ONION_PREV_FRAMES_NUM:
    case SETTING::ONION_NEXT_FRAMES_NUM:
    case SETTING::ONION_MIN_OPACITY:
    case SETTING::ONION_MAX_OPACITY:
        invalidateAllCache();
        break;
    case SETTING::ANTIALIAS:
    case SETTING::GRID:
    case SETTING::GRID_SIZE_W:
    case SETTING::GRID_SIZE_H:
    case SETTING::OVERLAY_CENTER:
    case SETTING::OVERLAY_THIRDS:
    case SETTING::OVERLAY_GOLDEN:
    case SETTING::OVERLAY_SAFE:
    case SETTING::OVERLAY_PERSPECTIVE1:
    case SETTING::OVERLAY_PERSPECTIVE2:
    case SETTING::OVERLAY_PERSPECTIVE3:
    case SETTING::ACTION_SAFE_ON:
    case SETTING::ACTION_SAFE:
    case SETTING::TITLE_SAFE_ON:
    case SETTING::TITLE_SAFE:
    case SETTING::OVERLAY_SAFE_HELPER_TEXT_ON:
    case SETTING::PREV_ONION:
    case SETTING::NEXT_ONION:
    case SETTING::ONION_BLUE:
    case SETTING::ONION_RED:
    case SETTING::INVISIBLE_LINES:
    case SETTING::OUTLINES:
    case SETTING::ONION_TYPE:
    case SETTING::ONION_WHILE_PLAYBACK:
        invalidateAllCache();
        break;
    case SETTING::QUICK_SIZING:
        mQuickSizing = mPrefs->isOn(SETTING::QUICK_SIZING);
        break;
    case SETTING::MULTILAYER_ONION:
        mMultiLayerOnionSkin = mPrefs->isOn(SETTING::MULTILAYER_ONION);
        invalidateAllCache();
        break;
    case SETTING::LAYER_VISIBILITY_THRESHOLD:
    case SETTING::LAYER_VISIBILITY:
        setLayerVisibility(static_cast<LayerVisibility>(mPrefs->getInt(SETTING::LAYER_VISIBILITY)));
        break;
    case SETTING::INVERT_SCROLL_ZOOM_DIRECTION:
        mDeltaFactor = mEditor->preference()->isOn(SETTING::INVERT_SCROLL_ZOOM_DIRECTION) ? -1 : 1;
        break;
    default:
        break;
    }

}

void ScribbleArea::updateToolCursor()
{
    setCursor(currentTool()->cursor());
    updateCanvasCursor();
}

void ScribbleArea::setCurveSmoothing(int newSmoothingLevel)
{
    mCurveSmoothingLevel = newSmoothingLevel / 20.0;
    invalidatePainterCaches();
}

void ScribbleArea::setEffect(SETTING e, bool isOn)
{
    mPrefs->set(e, isOn);
    invalidatePainterCaches();
}

/************************************************************************************/
// update methods

void ScribbleArea::updateFrame()
{
    update();
}

void ScribbleArea::invalidateCacheForDirtyFrames()
{
    Layer* currentLayer = mEditor->layers()->currentLayer();
    for (int pos : currentLayer->dirtyFrames()) {

        invalidateCacheForFrame(pos);
        invalidateOnionSkinsCacheAround(pos);
    }
    currentLayer->clearDirtyFrames();
}

void ScribbleArea::invalidateOnionSkinsCacheAround(int frameNumber)
{
    if (frameNumber < 0) { return; }

    bool isOnionAbsolute = mPrefs->getString(SETTING::ONION_TYPE) == "absolute";
    Layer *layer = mEditor->layers()->currentLayer(0);

    // The current layer can be null if updateFrame is triggered when creating a new project
    if (!layer) return;

    if (mPrefs->isOn(SETTING::PREV_ONION))
    {
        int onionFrameNumber = frameNumber;
        if (isOnionAbsolute)
        {
            onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber + 1, true);
        }

        for(int i = 1; i <= mPrefs->getInt(SETTING::ONION_PREV_FRAMES_NUM); i++)
        {
            onionFrameNumber = layer->getPreviousFrameNumber(onionFrameNumber, isOnionAbsolute);
            if (onionFrameNumber < 0) break;

            invalidateCacheForFrame(onionFrameNumber);
        }
    }

    if (mPrefs->isOn(SETTING::NEXT_ONION))
    {
        int onionFrameNumber = frameNumber;

        for(int i = 1; i <= mPrefs->getInt(SETTING::ONION_NEXT_FRAMES_NUM); i++)
        {
            onionFrameNumber = layer->getNextFrameNumber(onionFrameNumber, isOnionAbsolute);
            if (onionFrameNumber < 0) break;

            invalidateCacheForFrame(onionFrameNumber);
        }
    }
}

void ScribbleArea::invalidateAllCache()
{
    QPixmapCache::clear();
    mPixmapCacheKeys.clear();
    invalidatePainterCaches();
    mEditor->layers()->currentLayer()->clearDirtyFrames();

    updateFrame();
}

void ScribbleArea::invalidateCacheForFrame(int frameNumber)
{
    auto cacheKeyIter = mPixmapCacheKeys.find(static_cast<unsigned int>(frameNumber));
    if (cacheKeyIter != mPixmapCacheKeys.end())
    {
        QPixmapCache::remove(cacheKeyIter.value());
        unsigned int key = cacheKeyIter.key();
        mPixmapCacheKeys.remove(key);
    }
}

void ScribbleArea::invalidatePainterCaches()
{
    mCameraPainter.resetCache();
    mCanvasPainter.resetLayerCache();
    updateFrame();
}

void ScribbleArea::onToolPropertyUpdated(ToolType, ToolPropertyType type)
{
    switch (type)
    {
    case ToolPropertyType::CAMERAPATH:
        onFrameModified(mEditor->currentFrame());
        break;
    default:
        break;
    }
}

void ScribbleArea::onToolChanged(ToolType)
{
    int frame = mEditor->currentFrame();
    prepOverlays(frame);
    prepCameraPainter(frame);
    invalidateCacheForFrame(frame);
    updateFrame();
}


void ScribbleArea::onPlayStateChanged()
{
    int currentFrame = mEditor->currentFrame();
    if (mPrefs->isOn(SETTING::PREV_ONION) ||
        mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidatePainterCaches();
    }

    prepOverlays(currentFrame);
    prepCameraPainter(currentFrame);
    invalidateCacheForFrame(currentFrame);
    updateFrame();
}

void ScribbleArea::onWillScrub(int frameNumber)
{
    Q_UNUSED(frameNumber)

    // If we're in the middle of a painting session, stop it and save what was painted

    if (mTiledBuffer.isValid() || !mTilesBlitRect.isEmpty()) {
        endStroke();
    }
}

void ScribbleArea::onDidScrub(int frameNumber)
{
    Q_UNUSED(frameNumber)
    invalidatePainterCaches();
    updateFrame();
}

void ScribbleArea::onFramesModified()
{
    invalidateCacheForDirtyFrames();
    if (mPrefs->isOn(SETTING::PREV_ONION) || mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidatePainterCaches();
    }
    updateFrame();
}

void ScribbleArea::onFrameModified(int frameNumber)
{
    if (mPrefs->isOn(SETTING::PREV_ONION) || mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidateOnionSkinsCacheAround(frameNumber);
        invalidatePainterCaches();
    }
    invalidateCacheForFrame(frameNumber);
    updateFrame();
}

void ScribbleArea::onViewChanged()
{
    invalidateAllCache();
}

void ScribbleArea::onLayerChanged()
{
    invalidateAllCache();
}

void ScribbleArea::onSelectionChanged()
{
    int currentFrame = mEditor->currentFrame();
    invalidateCacheForFrame(currentFrame);
    updateFrame();
}

void ScribbleArea::onOnionSkinTypeChanged()
{
    invalidateAllCache();
}

void ScribbleArea::onObjectLoaded()
{
    invalidateAllCache();
}

bool ScribbleArea::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate)
    {
        editor()->tools()->clearTemporaryTool();
    }
    return QWidget::event(event);
}

/************************************************************************/
/* key event handlers                                                   */
/************************************************************************/

void ScribbleArea::keyPressEvent(QKeyEvent *event)
{
    // Don't handle this event on auto repeat
    if (event->isAutoRepeat()) { return; }

    if (isPointerInUse()) { return; } // prevents shortcuts calls while drawing

    if (currentTool()->keyPressEvent(event))
    {
        return; // has been handled by tool
    }

    // --- fixed control key shortcuts ---
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) &&
        editor()->tools()->setTemporaryTool(ERASER, {}, event->modifiers()))
    {
        return;
    }

    // ---- fixed normal keys ----

    auto selectMan = mEditor->select();
    bool isSomethingSelected = selectMan->somethingSelected();
    if (isSomethingSelected)
    {
        keyEventForSelection(event);
    }
    else
    {
        keyEvent(event);
    }
}

void ScribbleArea::keyEventForSelection(QKeyEvent* event)
{
    auto selectMan = mEditor->select();
    switch (event->key())
    {
    case Qt::Key_Right:
        selectMan->translate(QPointF(1, 0));
        selectMan->calculateSelectionTransformation();
        mEditor->frameModified(mEditor->currentFrame());
        return;
    case Qt::Key_Left:
        selectMan->translate(QPointF(-1, 0));
        selectMan->calculateSelectionTransformation();
        mEditor->frameModified(mEditor->currentFrame());
        return;
    case Qt::Key_Up:
        selectMan->translate(QPointF(0, -1));
        selectMan->calculateSelectionTransformation();
        mEditor->frameModified(mEditor->currentFrame());
        return;
    case Qt::Key_Down:
        selectMan->translate(QPointF(0, 1));
        selectMan->calculateSelectionTransformation();
        mEditor->frameModified(mEditor->currentFrame());
        return;
    case Qt::Key_Return:
        applyTransformedSelection();
        mEditor->deselectAll();
        return;
    case Qt::Key_Escape:
        cancelTransformedSelection();
        mEditor->deselectAll();
        return;
    case Qt::Key_Backspace:
        deleteSelection();
        mEditor->deselectAll();
        return;
    case Qt::Key_Space:
        if (editor()->tools()->setTemporaryTool(HAND, Qt::Key_Space, Qt::NoModifier))
        {
            return;
        }
        break;
    default:
        break;
    }
    event->ignore();
}

void ScribbleArea::keyEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Right:
        mEditor->scrubForward();
        break;
    case Qt::Key_Left:
        mEditor->scrubBackward();
        break;
    case Qt::Key_Up:
        mEditor->layers()->gotoNextLayer();
        break;
    case Qt::Key_Down:
        mEditor->layers()->gotoPreviouslayer();
        break;
    case Qt::Key_Space:
        if(editor()->tools()->setTemporaryTool(HAND, Qt::Key_Space, Qt::NoModifier))
        {
            return;
        }
        break;
    default:
        break;
    }
    event->ignore();
}

void ScribbleArea::keyReleaseEvent(QKeyEvent *event)
{
    // Don't handle this event on auto repeat
    //
    if (event->isAutoRepeat()) {
        return;
    }

    if (event->key() == 0)
    {
        editor()->tools()->tryClearTemporaryTool(Qt::Key_unknown);
    }
    else
    {
        editor()->tools()->tryClearTemporaryTool(static_cast<Qt::Key>(event->key()));
    }

    if (isPointerInUse()) { return; }

    if (currentTool()->keyReleaseEvent(event))
    {
        // has been handled by tool
        return;
    }
}



/************************************************************************************/
// mouse and tablet event handlers
void ScribbleArea::wheelEvent(QWheelEvent* event)
{
    // Don't change view if the tool is in use
    if (isPointerInUse()) return;

    static const bool isX11 = QGuiApplication::platformName() == "xcb";
    const QPoint pixels = event->pixelDelta();
    const QPoint angle = event->angleDelta();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QPointF offset = mEditor->view()->mapScreenToCanvas(event->position());
#else
    const QPointF offset = mEditor->view()->mapScreenToCanvas(event->posF());
#endif

    const qreal currentScale = mEditor->view()->scaling();
    // From the pixelDelta documentation: On X11 this value is driver-specific and unreliable, use angleDelta() instead
    if (!isX11 && !pixels.isNull())
    {
        // XXX: This pixel-based zooming algorithm currently has some shortcomings compared to the angle-based one:
        //      Zooming in is faster than zooming out and scrolling twice with delta x yields different zoom than
        //      scrolling once with delta 2x. Someone with the ability to test this code might want to "upgrade" it.
        const int delta = pixels.y();
        const qreal newScale = currentScale * (1 + ((delta * mDeltaFactor) * 0.01));
        mEditor->view()->scaleAtOffset(newScale, offset);
    }
    else if (!angle.isNull())
    {
        const int delta = angle.y();
        // 12 rotation steps at "standard" wheel resolution (120/step) result in 100x zoom
        const qreal newScale = currentScale * std::pow(100, (delta * mDeltaFactor) / (12.0 * 120));
        mEditor->view()->scaleAtOffset(newScale, offset);
    }
    updateCanvasCursor();
    event->accept();
}

void ScribbleArea::tabletEvent(QTabletEvent *e)
{
    PointerEvent event(e);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (event.pointerType() == QPointingDevice::PointerType::Eraser)
#else
    if (event.pointerType() == QTabletEvent::Eraser)
#endif
    {
        editor()->tools()->tabletSwitchToEraser();
    }
    else
    {
        editor()->tools()->tabletRestorePrevTool();
    }

    if (event.eventType() == QTabletEvent::TabletPress)
    {
        event.accept();
        mStrokeManager->pointerPressEvent(&event);
        mStrokeManager->setTabletInUse(true);
        if (mIsFirstClick)
        {
            mIsFirstClick = false;
            mDoubleClickTimer->start();
            pointerPressEvent(&event);
        }
        else
        {
            qreal distance = QLineF(currentTool()->getCurrentPressPoint(), currentTool()->getLastPressPoint()).length();

            if (mDoubleClickMillis <= DOUBLE_CLICK_THRESHOLD && distance < 5.0) {
                currentTool()->pointerDoubleClickEvent(&event);
            }
            else
            {
                // in case we handled the event as double click but really should have handled it as single click.
                pointerPressEvent(&event);
            }
        }
        mTabletInUse = event.isAccepted();
    }
    else if (event.eventType() == QTabletEvent::TabletMove)
    {
        if (!(event.buttons() & (Qt::LeftButton | Qt::RightButton)) || mTabletInUse)
        {
            mStrokeManager->pointerMoveEvent(&event);
            pointerMoveEvent(&event);
        }
    }
    else if (event.eventType() == QTabletEvent::TabletRelease)
    {
        mTabletReleaseMillisAgo = 0;
        mMouseFilterTimer->start();
        if (mTabletInUse)
        {
            mStrokeManager->pointerReleaseEvent(&event);
            pointerReleaseEvent(&event);
            mStrokeManager->setTabletInUse(false);
            mTabletInUse = false;
        }
    }
    // Always accept so that mouse events are not generated (theoretically)
    event.accept();
}

void ScribbleArea::pointerPressEvent(PointerEvent* event)
{
    bool isCameraLayer = mEditor->layers()->currentLayer()->type() == Layer::CAMERA;
    if ((currentTool()->type() != HAND || isCameraLayer) && (event->button() != Qt::RightButton) && (event->button() != Qt::MiddleButton || isCameraLayer))
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (!layer->visible())
        {
            event->ignore();
            // This needs to be async so that mTabletInUse is set to false before
            // further events are created (modal dialogs do not currently block tablet events)
            QTimer::singleShot(0, this, &ScribbleArea::showLayerNotVisibleWarning);
            return;
        }
    }

    if (event->buttons() & (Qt::MiddleButton | Qt::RightButton) &&
        editor()->tools()->setTemporaryTool(HAND, event->buttons()))
    {
        currentTool()->pointerPressEvent(event);
    }

    const bool isPressed = event->buttons() & Qt::LeftButton;
    if (isPressed && mQuickSizing)
    {
        if (currentTool()->startAdjusting(event->modifiers(), 1))
        {
            return;
        }
    }

    if (event->button() == Qt::LeftButton)
    {
        currentTool()->pointerPressEvent(event);
    }
}

void ScribbleArea::pointerMoveEvent(PointerEvent* event)
{

     updateCanvasCursor();

    if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
    {

        // --- use SHIFT + drag to resize WIDTH / use CTRL + drag to resize FEATHER ---
        if (currentTool()->isAdjusting())
        {
            currentTool()->adjustCursor(event->modifiers());
            return;
        }
    }

    currentTool()->pointerMoveEvent(event);
}

void ScribbleArea::pointerReleaseEvent(PointerEvent* event)
{
    if (currentTool()->isAdjusting())
    {
        currentTool()->stopAdjusting();
        updateCanvasCursor();
        return; // [SHIFT]+drag OR [CTRL]+drag
    }

    currentTool()->pointerReleaseEvent(event);

    editor()->tools()->tryClearTemporaryTool(event->button());
}

void ScribbleArea::handleDoubleClick()
{
    mDoubleClickMillis += 100;

    if (mDoubleClickMillis >= DOUBLE_CLICK_THRESHOLD)
    {
        mDoubleClickMillis = 0;
        mIsFirstClick = true;
        mDoubleClickTimer->stop();
    }
}

void ScribbleArea::tabletReleaseEventFired()
{
    // Under certain circumstances, a mouse press event will fire after a tablet release event.
    // This causes unexpected behaviors for some tools, e.g., the bucket tool.
    // The problem only seems to occur on Windows and only when tapping.
    // Prior to this fix, the event queue would look like this:
    // e.g.: TabletPress -> TabletRelease -> MousePress
    // The following will filter mouse events created after a tablet release event.
    mTabletReleaseMillisAgo += 50;

    if (mTabletReleaseMillisAgo >= MOUSE_FILTER_THRESHOLD) {
        mTabletReleaseMillisAgo = 0;
        mMouseFilterTimer->stop();
    }
}

bool ScribbleArea::isLayerPaintable() const
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return false; }

    return layer->type() == Layer::BITMAP || layer->type() == Layer::VECTOR;
}

void ScribbleArea::mousePressEvent(QMouseEvent* e)
{
    if (mTabletInUse || (mMouseFilterTimer->isActive() && mTabletReleaseMillisAgo < MOUSE_FILTER_THRESHOLD))
    {
        e->ignore();
        return;
    }

    PointerEvent event(e);

    mStrokeManager->pointerPressEvent(&event);

    pointerPressEvent(&event);
    mMouseInUse = event.isAccepted();
}

void ScribbleArea::mouseMoveEvent(QMouseEvent* e)
{
    if (mTabletInUse || (mMouseFilterTimer->isActive() && mTabletReleaseMillisAgo < MOUSE_FILTER_THRESHOLD)) { e->ignore(); return; }
    PointerEvent event(e);

    mStrokeManager->pointerMoveEvent(&event);

    pointerMoveEvent(&event);
}

void ScribbleArea::mouseReleaseEvent(QMouseEvent* e)
{
    if (mTabletInUse || (mMouseFilterTimer->isActive() && mTabletReleaseMillisAgo < MOUSE_FILTER_THRESHOLD)) { e->ignore(); return; }
    PointerEvent event(e);

    mStrokeManager->pointerReleaseEvent(&event);

    pointerReleaseEvent(&event);
    mMouseInUse = (e->buttons() & Qt::RightButton) || (e->buttons() & Qt::LeftButton);
}

void ScribbleArea::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (mStrokeManager->isTabletInUse()) { e->ignore(); return; }
    PointerEvent event(e);
    mStrokeManager->pointerPressEvent(&event);

    currentTool()->pointerDoubleClickEvent(&event);
}

void ScribbleArea::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    mDevicePixelRatio = devicePixelRatioF();
    mCanvas = QPixmap(QSizeF(size() * mDevicePixelRatio).toSize());
    mCanvas.setDevicePixelRatio(mDevicePixelRatio);
    mEditor->view()->setCanvasSize(size());
    mMyPaint->setSurfaceSize(size());

    invalidateCacheForFrame(mEditor->currentFrame());
    invalidatePainterCaches();
    mCanvasPainter.reset();
    mCameraPainter.reset();
}

void ScribbleArea::showLayerNotVisibleWarning()
{
    QMessageBox::warning(this, tr("Warning"),
                         tr("You are trying to modify a hidden layer! Please select another layer (or make the current layer visible)."),
                         QMessageBox::Ok,
                         QMessageBox::Ok);
}

void ScribbleArea::paintBitmapBuffer()
{
    LayerBitmap* layer = static_cast<LayerBitmap*>(mEditor->layers()->currentLayer());
    Q_ASSERT(layer);
    Q_ASSERT(layer->type() == Layer::BITMAP);

    int frameNumber = mEditor->currentFrame();

    // If there is no keyframe at or before the current position,
    // just return (since we have nothing to paint on).
    if (layer->getLastKeyFrameAtPosition(frameNumber) == nullptr)
    {
        updateFrame();
        return;
    }

    BitmapImage* targetImage = currentBitmapImage(layer);

    if (targetImage != nullptr)
    {
        const auto bufferTiles = mBufferTiles;
        targetImage->paste(bufferTiles, mTilesBlitRect, QPainter::CompositionMode::CompositionMode_Source);
    }

    QRect rect = mEditor->view()->mapCanvasToScreen(mTilesBlitRect).toRect();

    update(rect);

    layer->setModified(frameNumber, true);
    
    mTilesBlitRect = BlitRect();
}

void ScribbleArea::clearDrawingBuffer()
{
    mMyPaint->clearSurface();
    mBufferTiles.clear();
    mTilesBlitRect = BlitRect();
    mTiledBuffer.clear();
}

void ScribbleArea::paintCanvasCursor(QPainter& painter)
{
    QTransform view = mEditor->view()->getView();
    QPointF mousePos = currentTool()->isAdjusting() ? currentTool()->getCurrentPressPoint() : currentTool()->getCurrentPoint();
    int centerCal = mCursorImg.width() / 2;

    mTransformedCursorPos = view.map(mousePos);

    // reset matrix
    view.reset();

    painter.setTransform(view);
    mCursorCenterPos.setX(centerCal);
    mCursorCenterPos.setY(centerCal);

    painter.drawPixmap(QPoint(static_cast<int>(mTransformedCursorPos.x() - mCursorCenterPos.x()),
                              static_cast<int>(mTransformedCursorPos.y() - mCursorCenterPos.y())),
                       mCursorImg);

    mCursorCenterPos.setX(centerCal);
    mCursorCenterPos.setY(centerCal);
}

void ScribbleArea::updateCanvasCursor()
{
    qreal scalingFac = static_cast<qreal>(mEditor->view()->scaling());
    float brushWidth = mMyPaint->getBrushState(MyPaintBrushState::MYPAINT_BRUSH_STATE_ACTUAL_RADIUS);

    if (currentTool()->isAdjusting())
    {
        mCursorImg = currentTool()->quickSizeCursor(scalingFac, brushWidth);
    }
    else if (mEditor->preference()->isOn(SETTING::DOTTED_CURSOR))
    {
        mCursorImg = currentTool()->canvasCursor(static_cast<float>(brushWidth), scalingFac, width());
    }
    else
    {
        mCursorImg = QPixmap(); // if the above does not comply, deallocate image
    }

    QPoint translatedPos(static_cast<int>(mTransformedCursorPos.x() - mCursorCenterPos.x()),
                         static_cast<int>(mTransformedCursorPos.y() - mCursorCenterPos.y()));
    update(mCursorImg.rect().adjusted(-1, -1, 1, 1).translated(translatedPos));
}

void ScribbleArea::handleDrawingOnEmptyFrame()
{
    auto layer = mEditor->layers()->currentLayer();

    if (!layer || !layer->isPaintable())
    {
        return;
    }

    if (currentTool()->type() == ERASER) {
        return;
    }

    int frameNumber = mEditor->currentFrame();
    if (layer->getKeyFrameAt(frameNumber)) { return; }

    // Drawing on an empty frame; take action based on preference.
    int action = mPrefs->getInt(SETTING::DRAW_ON_EMPTY_FRAME_ACTION);
    auto previousKeyFrame = layer->getLastKeyFrameAtPosition(frameNumber);
    switch (action)
    {
    case KEEP_DRAWING_ON_PREVIOUS_KEY:
    {
        if (previousKeyFrame == nullptr) {
            mEditor->addNewKey();
        } else {
            onFrameModified(previousKeyFrame->pos());
        }
        break;
    }
    case DUPLICATE_PREVIOUS_KEY:
    {
        if (previousKeyFrame != nullptr) {
            KeyFrame* dupKey = previousKeyFrame->clone();
            layer->addKeyFrame(frameNumber, dupKey);
            mEditor->scrubTo(frameNumber);
            break;
        }
    }
    // if the previous keyframe doesn't exist,
    // an empty keyframe needs to be created, so
    // fallthrough
    case CREATE_NEW_KEY:
        mEditor->addNewKey();

        // Refresh canvas
        drawCanvas(frameNumber, mCanvas.rect());
        break;
    default:
        break;
    }
}

void ScribbleArea::paintEvent(QPaintEvent* event)
{
    int currentFrame = mEditor->currentFrame();
    if (!currentTool()->isActive())
    {
        // --- we retrieve the canvas from the cache; we create it if it doesn't exist
        const int frameNumber = mEditor->layers()->lastFrameAtFrame(currentFrame);

        if (frameNumber < 0)
        {
            drawCanvas(currentFrame, event->rect());
        }
        else
        {
            auto cacheKeyIter = mPixmapCacheKeys.find(static_cast<unsigned>(frameNumber));

            if (cacheKeyIter == mPixmapCacheKeys.end() || !QPixmapCache::find(cacheKeyIter.value(), &mCanvas))
            {
                drawCanvas(currentFrame, event->rect());
                mPixmapCacheKeys[static_cast<unsigned>(currentFrame)] = QPixmapCache::insert(mCanvas);
                //qDebug() << "Repaint canvas!";
            }
            else
            {
                // Simply use the cached canvas from PixmapCache
            }
        }
    }
    else
    {
        prepCanvas(currentFrame);
        prepCameraPainter(currentFrame);
        prepOverlays(currentFrame);

        mCanvasPainter.paintCached(event->rect());
        mCameraPainter.paintCached(event->rect());
    }

    if (currentTool()->type() == MOVE)
    {
        Layer* layer = mEditor->layers()->currentLayer();
        Q_CHECK_PTR(layer);
        if (layer->type() == Layer::VECTOR)
        {
            currentVectorImage(layer)->setModified(true);
        }
    }

    QPainter painter(this);

    // paints the canvas
    painter.setWorldMatrixEnabled(false);

    // In other places we use the blitRect to paint the buffer pixmap, however
    // the main pixmap which needs to be scaled accordingly to DPI, which is not accounted for when using the event rect
    // instead we can set a clipRect to avoid the area being updated needlessly
    painter.setClipRect(event->rect());
    painter.drawPixmap(QPointF(), mCanvas);

    currentTool()->paint(painter);

    if (!editor()->playback()->isPlaying())    // we don't need to display the following when the animation is playing
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = currentVectorImage(layer);
            if (vectorImage != nullptr)
            {
                switch (currentTool()->type())
                {
                case SMUDGE:
                case HAND:
                {
                    auto selectMan = mEditor->select();
                    painter.save();
                    painter.setWorldMatrixEnabled(false);
                    painter.setRenderHint(QPainter::Antialiasing, false);
                    // ----- paints the edited elements
                    QPen pen2(Qt::black, 0.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
                    painter.setPen(pen2);
                    QColor color;
                    // ------------ vertices of the edited curves
                    color = QColor(200, 200, 200);
                    painter.setBrush(color);
                    VectorSelection vectorSelection = selectMan->vectorSelection;
                    for (int k = 0; k < vectorSelection.curve.size(); k++)
                    {
                        int curveNumber = vectorSelection.curve.at(k);

                        for (int vertexNumber = -1; vertexNumber < vectorImage->getCurveSize(curveNumber); vertexNumber++)
                        {
                            QPointF vertexPoint = vectorImage->getVertex(curveNumber, vertexNumber);
                            QRectF rectangle(mEditor->view()->mapCanvasToScreen(vertexPoint) - QPointF(3.0, 3.0), QSizeF(7, 7));
                            if (rect().contains(mEditor->view()->mapCanvasToScreen(vertexPoint).toPoint()))
                            {
                                painter.drawRect(rectangle);
                            }
                        }
                    }
                    // ------------ selected vertices of the edited curves
                    color = QColor(100, 100, 255);
                    painter.setBrush(color);
                    for (int k = 0; k < vectorSelection.vertex.size(); k++)
                    {
                        VertexRef vertexRef = vectorSelection.vertex.at(k);
                        QPointF vertexPoint = vectorImage->getVertex(vertexRef);
                        QRectF rectangle0 = QRectF(mEditor->view()->mapCanvasToScreen(vertexPoint) - QPointF(3.0, 3.0), QSizeF(7, 7));
                        painter.drawRect(rectangle0);
                    }
                    // ----- paints the closest vertices
                    color = QColor(255, 0, 0);
                    painter.setBrush(color);
                    QList<VertexRef> closestVertices = selectMan->closestVertices();
                    if (vectorSelection.curve.size() > 0)
                    {
                        for (int k = 0; k < closestVertices.size(); k++)
                        {
                            VertexRef vertexRef = closestVertices.at(k);
                            QPointF vertexPoint = vectorImage->getVertex(vertexRef);

                            QRectF rectangle = QRectF(mEditor->view()->mapCanvasToScreen(vertexPoint) - QPointF(3.0, 3.0), QSizeF(7, 7));
                            painter.drawRect(rectangle);
                        }
                    }
                    painter.restore();
                    break;
                }
                default:
                {
                    break;
                }
                } // end switch
            }
        }

        paintCanvasCursor(painter);

        mOverlayPainter.paint(painter, rect());

        // paints the selection outline
        if (mEditor->select()->somethingSelected())
        {
            paintSelectionVisuals(painter);
        }
    }

    event->accept();
}

void ScribbleArea::paintSelectionVisuals(QPainter &painter)
{
    Object* object = mEditor->object();

    auto selectMan = mEditor->select();

    QRectF currentSelectionRect = selectMan->mySelectionRect();

    if (currentSelectionRect.isEmpty()) { return; }

    TransformParameters params = { currentSelectionRect, editor()->view()->getView(), selectMan->selectionTransform() };
    mSelectionPainter.paint(painter, object, mEditor->currentLayerIndex(), currentTool(), params);
    emit selectionUpdated();
}

BitmapImage* ScribbleArea::currentBitmapImage(Layer* layer) const
{
    Q_ASSERT(layer->type() == Layer::BITMAP);
    auto bitmapLayer = static_cast<LayerBitmap*>(layer);
    return bitmapLayer->getLastBitmapImageAtFrame(mEditor->currentFrame());
}

VectorImage* ScribbleArea::currentVectorImage(Layer* layer) const
{
    Q_ASSERT(layer->type() == Layer::VECTOR);
    auto vectorLayer = (static_cast<LayerVector*>(layer));
    return vectorLayer->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
}

void ScribbleArea::prepCameraPainter(int frame)
{
    Object* object = mEditor->object();

    mCameraPainter.preparePainter(object,
                                  mEditor->currentLayerIndex(),
                                  frame,
                                  mEditor->view()->getView(),
                                  mEditor->playback()->isPlaying(),
                                  mLayerVisibility,
                                  mPrefs->getFloat(SETTING::LAYER_VISIBILITY_THRESHOLD),
                                  mEditor->view()->getViewScaleInverse());

    OnionSkinPainterOptions onionSkinOptions;
    onionSkinOptions.enabledWhilePlaying = mPrefs->getInt(SETTING::ONION_WHILE_PLAYBACK);
    onionSkinOptions.isPlaying = mEditor->playback()->isPlaying();
    onionSkinOptions.isAbsolute = (mPrefs->getString(SETTING::ONION_TYPE) == "absolute");
    onionSkinOptions.skinPrevFrames = mPrefs->isOn(SETTING::PREV_ONION);
    onionSkinOptions.skinNextFrames = mPrefs->isOn(SETTING::NEXT_ONION);
    onionSkinOptions.colorizePrevFrames = mPrefs->isOn(SETTING::ONION_RED);
    onionSkinOptions.colorizeNextFrames = mPrefs->isOn(SETTING::ONION_BLUE);
    onionSkinOptions.framesToSkinPrev = mPrefs->getInt(SETTING::ONION_PREV_FRAMES_NUM);
    onionSkinOptions.framesToSkinNext = mPrefs->getInt(SETTING::ONION_NEXT_FRAMES_NUM);
    onionSkinOptions.maxOpacity = mPrefs->getInt(SETTING::ONION_MAX_OPACITY);
    onionSkinOptions.minOpacity = mPrefs->getInt(SETTING::ONION_MIN_OPACITY);

    mCameraPainter.setOnionSkinPainterOptions(onionSkinOptions);
}

void ScribbleArea::prepCanvas(int frame)
{
    Object* object = mEditor->object();

    CanvasPainterOptions o;
    o.bAntiAlias = mPrefs->isOn(SETTING::ANTIALIAS);
    o.bThinLines = mPrefs->isOn(SETTING::INVISIBLE_LINES);
    o.bOutlines = mPrefs->isOn(SETTING::OUTLINES);
    o.eLayerVisibility = mLayerVisibility;
    o.fLayerVisibilityThreshold = mPrefs->getFloat(SETTING::LAYER_VISIBILITY_THRESHOLD);
    o.scaling = mEditor->view()->scaling();
    o.cmBufferBlendMode = mEditor->tools()->currentTool()->type() == ToolType::ERASER ? QPainter::CompositionMode_DestinationOut : QPainter::CompositionMode_SourceOver;
    o.bIgnoreCanvasBuffer = currentTool()->type() == POLYLINE;

    OnionSkinPainterOptions onionSkinOptions;
    onionSkinOptions.enabledWhilePlaying = mPrefs->getInt(SETTING::ONION_WHILE_PLAYBACK);
    onionSkinOptions.isPlaying = mEditor->playback()->isPlaying();
    onionSkinOptions.isAbsolute = (mPrefs->getString(SETTING::ONION_TYPE) == "absolute");
    onionSkinOptions.skinPrevFrames = mPrefs->isOn(SETTING::PREV_ONION);
    onionSkinOptions.skinNextFrames = mPrefs->isOn(SETTING::NEXT_ONION);
    onionSkinOptions.colorizePrevFrames = mPrefs->isOn(SETTING::ONION_RED);
    onionSkinOptions.colorizeNextFrames = mPrefs->isOn(SETTING::ONION_BLUE);
    onionSkinOptions.framesToSkinPrev = mPrefs->getInt(SETTING::ONION_PREV_FRAMES_NUM);
    onionSkinOptions.framesToSkinNext = mPrefs->getInt(SETTING::ONION_NEXT_FRAMES_NUM);
    onionSkinOptions.maxOpacity = mPrefs->getInt(SETTING::ONION_MAX_OPACITY);
    onionSkinOptions.minOpacity = mPrefs->getInt(SETTING::ONION_MIN_OPACITY);

    mCanvasPainter.setOnionSkinOptions(onionSkinOptions);
    mCanvasPainter.setOptions(o);

    ViewManager* vm = mEditor->view();
    SelectionManager* sm = mEditor->select();
    mCanvasPainter.setViewTransform(vm->getView(), vm->getViewInverse());
    mCanvasPainter.setTransformedSelection(sm->mySelectionRect().toRect(), sm->selectionTransform());

    mCanvasPainter.setPaintSettings(object, mEditor->layers()->currentLayerIndex(), frame, mBufferTiles, mTilesBlitRect, &mTiledBuffer);
}

void ScribbleArea::drawCanvas(int frame, QRect rect)
{
    prepCanvas(frame);
    prepCameraPainter(frame);
    prepOverlays(frame);
    mCanvasPainter.paint(rect);
    mCameraPainter.paint(rect);
}

void ScribbleArea::prepOverlays(int frame)
{
    OverlayPainterOptions o;

    o.bGrid = mPrefs->isOn(SETTING::GRID);
    o.nGridSizeW = mPrefs->getInt(SETTING::GRID_SIZE_W);
    o.nGridSizeH = mPrefs->getInt(SETTING::GRID_SIZE_H);
    o.bCenter = mPrefs->isOn(SETTING::OVERLAY_CENTER);
    o.bThirds = mPrefs->isOn(SETTING::OVERLAY_THIRDS);
    o.bGoldenRatio = mPrefs->isOn(SETTING::OVERLAY_GOLDEN);
    o.bSafeArea = mPrefs->isOn(SETTING::OVERLAY_SAFE);
    o.bPerspective1 = mPrefs->isOn(SETTING::OVERLAY_PERSPECTIVE1);
    o.bPerspective2 = mPrefs->isOn(SETTING::OVERLAY_PERSPECTIVE2);
    o.bPerspective3 = mPrefs->isOn(SETTING::OVERLAY_PERSPECTIVE3);
    o.nOverlayAngle = mPrefs->getInt(SETTING::OVERLAY_ANGLE);
    o.bActionSafe = mPrefs->isOn(SETTING::ACTION_SAFE_ON);
    o.nActionSafe = mPrefs->getInt(SETTING::ACTION_SAFE);
    o.bShowSafeAreaHelperText = mPrefs->isOn(SETTING::OVERLAY_SAFE_HELPER_TEXT_ON);
    o.bTitleSafe = mPrefs->isOn(SETTING::TITLE_SAFE_ON);
    o.nTitleSafe = mPrefs->getInt(SETTING::TITLE_SAFE);
    o.nOverlayAngle = mPrefs->getInt(SETTING::OVERLAY_ANGLE);
    o.bShowHandle = mEditor->tools()->currentTool()->type() == MOVE && mEditor->layers()->currentLayer()->type() != Layer::CAMERA;

    o.mSinglePerspPoint = mEditor->overlays()->getSinglePerspectivePoint();
    o.mLeftPerspPoint = mEditor->overlays()->getLeftPerspectivePoint();
    o.mRightPerspPoint = mEditor->overlays()->getRightPerspectivePoint();
    o.mMiddlePerspPoint = mEditor->overlays()->getMiddlePerspectivePoint();

    o.nFrameIndex = frame;

    mOverlayPainter.setOptions(o);
    mOverlayPainter.preparePainter(mEditor->layers()->getCameraLayerBelow(mEditor->currentLayerIndex()), palette());

    ViewManager* vm = mEditor->view();
    mOverlayPainter.setViewTransform(vm->getView());
}

void ScribbleArea::loadMPBrush(const QByteArray &content)
{
    mMyPaint->loadBrush(content);

    forceUpdateMyPaintStates();
    updateCanvasCursor();
}

void ScribbleArea::onTileUpdated(TiledBuffer* tiledBuffer, Tile* tile)
{
    Q_UNUSED(tiledBuffer);
    const QRectF& mappedRect = mEditor->view()->getView().mapRect(QRectF(tile->bounds()));
    update(mappedRect.toAlignedRect());
}

void ScribbleArea::onTileCreated(TiledBuffer* tiledBuffer, Tile* tile)
{
    Q_UNUSED(tiledBuffer)
    Layer::LAYER_TYPE layerType = mEditor->layers()->currentLayer()->type();
    if (layerType == Layer::BITMAP) {
        const auto& bitmapImage = currentBitmapImage(mEditor->layers()->currentLayer());
        const QImage& image = *bitmapImage->image();
        tile->load(image, bitmapImage->topLeft());
    } else if (layerType == Layer::VECTOR) {

        // Not used, we only use the buffer to paint the stroke before painting the real vector stroke
    }

    const QRectF& mappedRect = mEditor->view()->getView().mapRect(QRectF(tile->bounds()));
    update(mappedRect.toAlignedRect());
}

void ScribbleArea::updateTile(MPSurface *surface, MPTile *tile)
{
    Q_UNUSED(surface)

    QPoint pos = tile->pos();

    tile->setDirty(true);

    mBufferTiles.insert(QString::number(pos.x())+"_"+QString::number(pos.y()), tile);
    mTilesBlitRect.extend(pos, tile->boundingRect().size());

    const QRectF& mappedRect = mEditor->view()->getView().mapRect(QRectF(pos, tile->boundingRect().size()));
    update(mappedRect.toAlignedRect());
}

void ScribbleArea::loadTile(MPSurface* surface, MPTile* tile)
{
    Q_UNUSED(surface)
    Layer* layer = mEditor->layers()->currentLayer();


    // Polyline is special because the surface must be cleared on every update, given its nature of drawing a long stroke segment.
    // Therefore we only load the mypaint surface with bitmap data when not using the polyline tool.

    // TODO: This code would be better served in StrokeTool  rather than here.
//    if (mIsPainting && currentTool()->type() != ToolType::POLYLINE) {
        const auto& bitmapImage = currentBitmapImage(layer);
        const QImage& image = *bitmapImage->image();
        mMyPaint->loadTile(image, bitmapImage->topLeft(), tile);
//    }
    mTilesBlitRect.extend(tile->pos(), tile->boundingRect().size());
    const QRectF& mappedRect = mEditor->view()->getView().mapRect(QRectF(tile->pos(), tile->boundingRect().size()));
    update(mappedRect.toAlignedRect());
}

void ScribbleArea::clearTile(MPSurface *surface, QRect tileRect)
{
    Q_UNUSED(surface)

    int posX = tileRect.x();
    int posY = tileRect.y();

    mBufferTiles.remove(QString::number(posX)+"_"+QString::number(posY));
    const QRectF& mappedRect = mEditor->view()->getView().mapRect(QRectF(tileRect));
    update(mappedRect.toAlignedRect());
}

/************************************************************************************/
// Stroke Handling

void ScribbleArea::startStroke()
{
    mMyPaint->startStroke();
}

void ScribbleArea::strokeTo(QPointF point, float pressure, float xtilt, float ytilt, double dt)
{
    Q_ASSERT(mEditor->layers()->currentLayer()->type() == Layer::BITMAP);

    mMyPaint->strokeTo(static_cast<float>(point.x()), static_cast<float>(point.y()), pressure, xtilt, ytilt, dt);
}

void ScribbleArea::forceUpdateMyPaintStates()
{
    // Simulate stroke to force states to update
    // this doesn't draw on canvas, because the cursor doesn't move
    mMyPaint->strokeTo(mMyPaint->getBrushState(MYPAINT_BRUSH_STATE_X),
                       mMyPaint->getBrushState(MYPAINT_BRUSH_STATE_Y),
                       mMyPaint->getBrushState(MyPaintBrushState::MYPAINT_BRUSH_STATE_PRESSURE),
                       0,0, 1.0);
    // TODO: deltatime should maybe not be fixed here?
}

void ScribbleArea::endStroke()
{   
    if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP) {
        mMyPaint->endStroke();
    }

    if (mPrefs->isOn(SETTING::PREV_ONION) || mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidateOnionSkinsCacheAround(mEditor->currentFrame());
        invalidatePainterCaches();
    }
    invalidateCacheForFrame(mEditor->currentFrame());
    updateFrame();

    qDebug() << "end stroke";
}

void ScribbleArea::flipSelection(bool flipVertical)
{
    mEditor->select()->flipSelection(flipVertical);
}

void ScribbleArea::drawPath(QPainterPath path, QPen pen, QBrush brush, QPainter::CompositionMode cm)
{
    mTiledBuffer.drawPath(mEditor->view()->mapScreenToCanvas(path), mEditor->view()->mapScreenToCanvas(mCursorImg.rect()).width(), pen, brush, cm, mPrefs->isOn(SETTING::ANTIALIAS));
}

void ScribbleArea::drawPolyline(QPainterPath path, QPen pen, bool useAA)
{
    BlitRect blitRect;

    // In order to clear what was previously dirty, we need to include the previous buffer bound
    // this ensures that we won't see stroke artifacts
    blitRect.extend(mEditor->view()->mapCanvasToScreen(mTiledBuffer.bounds()).toRect());

    QRect updateRect = mEditor->view()->mapCanvasToScreen(path.boundingRect()).toRect();
    // Now extend with the new path bounds mapped to the local coordinate
    blitRect.extend(updateRect);

    mTiledBuffer.clear();
    mTiledBuffer.drawPath(path, mEditor->view()->mapScreenToCanvas(mCursorImg.rect()).width(), pen, Qt::NoBrush, QPainter::CompositionMode_SourceOver, useAA);

    // And update only the affected area
    update(blitRect.adjusted(-1, -1, 1, 1));
}

/************************************************************************************/
// view handling

QPointF ScribbleArea::getCentralPoint()
{
    return mEditor->view()->mapScreenToCanvas(QPointF(width() / 2, height() / 2));
}

void ScribbleArea::applyTransformedSelection()
{

    // FIXME: if the selection is ignored, then undo won't show the correct transformation until the
    // transformation is painted again.
    mCanvasPainter.ignoreTransformedSelection();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr)
    {
        return;
    }

    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())
    {
        if (selectMan->mySelectionRect().isEmpty()) { return; }

        if (layer->type() == Layer::BITMAP)
        {
            handleDrawingOnEmptyFrame();
            BitmapImage* bitmapImage = currentBitmapImage(layer);
            if (bitmapImage == nullptr) { return; }
            BitmapImage transformed = currentBitmapImage(layer)->transformed(selectMan->mySelectionRect().toRect(), selectMan->selectionTransform(), true);
            bitmapImage->clear(selectMan->mySelectionRect());
            bitmapImage->paste(&transformed, QPainter::CompositionMode_SourceOver);
        }
        else if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = currentVectorImage(layer);
            if (vectorImage == nullptr) { return; }

            vectorImage->applySelectionTransformation();
        }

        mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    }

    updateFrame();
}

void ScribbleArea::cancelTransformedSelection()
{
    mCanvasPainter.ignoreTransformedSelection();

    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (layer == nullptr) { return; }

        if (layer->type() == Layer::VECTOR) {

            VectorImage* vectorImage = currentVectorImage(layer);
            vectorImage->setSelectionTransformation(QTransform());
        }

        mEditor->select()->setSelection(selectMan->mySelectionRect());

        selectMan->resetSelectionProperties();
        mOriginalPolygonF = QPolygonF();

        mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
        updateFrame();
    }
}

void ScribbleArea::toggleThinLines()
{
    bool previousValue = mPrefs->isOn(SETTING::INVISIBLE_LINES);
    setEffect(SETTING::INVISIBLE_LINES, !previousValue);
}

void ScribbleArea::setLayerVisibility(LayerVisibility visibility)
{
    mLayerVisibility = visibility;
    mPrefs->set(SETTING::LAYER_VISIBILITY, static_cast<int>(mLayerVisibility));

    invalidateAllCache();
}

void ScribbleArea::increaseLayerVisibilityIndex()
{
    ++mLayerVisibility;
    mPrefs->set(SETTING::LAYER_VISIBILITY, static_cast<int>(mLayerVisibility));

    invalidateAllCache();
}

void ScribbleArea::decreaseLayerVisibilityIndex()
{
    --mLayerVisibility;
    mPrefs->set(SETTING::LAYER_VISIBILITY, static_cast<int>(mLayerVisibility));

    invalidateAllCache();
}

/************************************************************************************/
// tool handling

BaseTool* ScribbleArea::currentTool() const
{
    return editor()->tools()->currentTool();
}

void ScribbleArea::deleteSelection()
{
    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())      // there is something selected
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (layer == nullptr) { return; }

        handleDrawingOnEmptyFrame();

        mEditor->backup(tr("Delete Selection", "Undo Step: clear the selection area."));

        selectMan->clearCurves();
        if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = currentVectorImage(layer);
            Q_CHECK_PTR(vectorImage);
            vectorImage->deleteSelection();
        }
        else if (layer->type() == Layer::BITMAP)
        {
            BitmapImage* bitmapImage = currentBitmapImage(layer);
            Q_CHECK_PTR(bitmapImage);
            bitmapImage->clear(selectMan->mySelectionRect());
        }
        mEditor->setModified(mEditor->currentLayerIndex(), mEditor->currentFrame());
    }
}

void ScribbleArea::clearImage()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (layer->type() == Layer::VECTOR)
    {
        mEditor->backup(tr("Clear Image", "Undo step text"));

        VectorImage* vectorImage = currentVectorImage(layer);
        if (vectorImage != nullptr)
        {
            vectorImage->clear();
        }
        mEditor->select()->clearCurves();
        mEditor->select()->clearVertices();
    }
    else if (layer->type() == Layer::BITMAP)
    {
        mEditor->backup(tr("Clear Image", "Undo step text"));

        BitmapImage* bitmapImage = currentBitmapImage(layer);
        if (bitmapImage == nullptr) return;
        bitmapImage->clear();
        mMyPaint->clearSurface();
    }
    else
    {
        return; // skip updates when nothing changes
    }
    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}

// TODO: move somewhere else... scribblearea is bloated enough already.
void ScribbleArea::paletteColorChanged(QColor color)
{
    mMyPaint->setBrushColor(color);

    for (int i = 0; i < mEditor->layers()->count(); i++)
    {
        Layer* layer = mEditor->layers()->getLayer(i);
        if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getVectorImageAtFrame(mEditor->currentFrame());
            if (vectorImage != nullptr)
            {
                vectorImage->modification();
            }
        }
    }

    invalidateAllCache();
}

void ScribbleArea::brushSettingChanged(BrushSettingType settingType, float value)
{
    qDebug() << "value before mypaint: " << value;

    mMyPaint->setBrushBaseValue(static_cast<MyPaintBrushSetting>(settingType), value);
    forceUpdateMyPaintStates();
    updateCanvasCursor();
}

float ScribbleArea::getBrushSettingBaseValue(BrushSettingType settingType)
{
    return mMyPaint->getBrushSettingBaseValue(static_cast<MyPaintBrushSetting>(settingType));
}

void ScribbleArea::setBrushInputMapping(QVector<QPointF> points, BrushSettingType settingType, BrushInputType inputType)
{
    mMyPaint->setBrushInputMappingPoints(points,
                                         static_cast<MyPaintBrushSetting>(settingType),
                                         static_cast<MyPaintBrushInput>(inputType));
}

const BrushInputMapping ScribbleArea::getBrushInputMapping(BrushSettingType settingType, BrushInputType inputType)
{
    BrushInputMapping inputMapping;
    auto mappingPoints = mMyPaint->getBrushInputMappingPoints(static_cast<MyPaintBrushSetting>(settingType),
                                                              static_cast<MyPaintBrushInput>(inputType));
    MappingControlPoints controlPoints;
    QVector<QPointF> points;

    for (int i = 0; i < mappingPoints->n; i++) {
        const auto point = QPointF(static_cast<qreal>(mappingPoints->xvalues[i]),static_cast<qreal>(mappingPoints->yvalues[i]));
        controlPoints.points << point;
    }
    controlPoints.numberOfPoints = mappingPoints->n;
    inputMapping.controlPoints = controlPoints;
    inputMapping.inputsUsed = mMyPaint->getBrushInputsUsed(static_cast<MyPaintBrushSetting>(settingType));
    inputMapping.baseValue = mMyPaint->getBrushSettingBaseValue(static_cast<MyPaintBrushSetting>(settingType));

    return inputMapping;
}

const BrushSettingInfo ScribbleArea::getBrushSettingInfo(BrushSettingType setting)
{
    const MyPaintBrushSettingInfo* info = mMyPaint->getBrushSettingInfo(static_cast<MyPaintBrushSetting>(setting));

    BrushSettingInfo brushInfo;

    brushInfo.cname = info->cname;
    brushInfo.name = info->name;
    brushInfo.min = info->min;
    brushInfo.max = info->max;
    brushInfo.tooltip = info->tooltip;
    brushInfo.defaultValue = info->def;
    brushInfo.isConstant = info->constant;

    return brushInfo;
}

const BrushInputInfo ScribbleArea::getBrushInputInfo(BrushInputType input)
{
    const MyPaintBrushInputInfo* info = mMyPaint->getBrushInputInfo(static_cast<MyPaintBrushInput>(input));

    BrushInputInfo brushInfo { info->cname,
                static_cast<qreal>(info->hard_min),
                static_cast<qreal>(info->soft_min),
                static_cast<qreal>(info->normal),
                static_cast<qreal>(info->soft_max),
                static_cast<qreal>(info->hard_max),
                info->name,
                info->tooltip };

    return brushInfo;
}
