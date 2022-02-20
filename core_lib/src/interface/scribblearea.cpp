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

#include "scribblearea.h"

#include <cmath>
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


ScribbleArea::ScribbleArea(QWidget* parent) : QWidget(parent)
{
    setObjectName("ScribbleArea");

    // Qt::WA_StaticContents ensure that the widget contents are rooted to the top-left corner
    // and don't change when the widget is resized.
    setAttribute(Qt::WA_StaticContents);

    mStrokeManager.reset(new StrokeManager);
}

ScribbleArea::~ScribbleArea()
{
    delete mBufferImg;
}

bool ScribbleArea::init()
{
    mPrefs = mEditor->preference();
    mDoubleClickTimer = new QTimer(this);
    mMyPaint = new MPHandler();

    connect(mMyPaint, &MPHandler::tileAdded, this, &ScribbleArea::updateTile);
    connect(mMyPaint, &MPHandler::tileUpdated, this, &ScribbleArea::updateTile);
    connect(mMyPaint, &MPHandler::tileCleared, this, &ScribbleArea::clearTile);

    connect(mPrefs, &PreferenceManager::optionChanged, this, &ScribbleArea::settingUpdated);
    connect(mDoubleClickTimer, &QTimer::timeout, this, &ScribbleArea::handleDoubleClick);

    connect(mEditor->select(), &SelectionManager::selectionChanged, this, &ScribbleArea::onSelectionChanged);
    connect(mEditor->select(), &SelectionManager::needPaintAndApply, this, &ScribbleArea::applySelectionChanges);
    connect(mEditor->select(), &SelectionManager::needDeleteSelection, this, &ScribbleArea::deleteSelection);

    mDoubleClickTimer->setInterval(50);

    const int curveSmoothingLevel = mPrefs->getInt(SETTING::CURVE_SMOOTHING);
    mCurveSmoothingLevel = curveSmoothingLevel / 20.0; // default value is 1.0

    mQuickSizing = mPrefs->isOn(SETTING::QUICK_SIZING);
    mMakeInvisible = false;

    mIsSimplified = mPrefs->isOn(SETTING::OUTLINES);
    mMultiLayerOnionSkin = mPrefs->isOn(SETTING::MULTILAYER_ONION);

    mLayerVisibility = static_cast<LayerVisibility>(mPrefs->getInt(SETTING::LAYER_VISIBILITY));

    mBufferImg = new BitmapImage;

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
    invalidateLayerPixmapCache();
}

void ScribbleArea::setEffect(SETTING e, bool isOn)
{
    mPrefs->set(e, isOn);
    invalidateLayerPixmapCache();
}

void ScribbleArea::updateMyPaintCanvas(BitmapImage* bitmapImage)
{
    Layer* layer = mEditor->layers()->currentLayer();

    if (!bitmapImage) {
        bitmapImage = currentBitmapImage(layer);
    }

    if (bitmapImage->bounds().isNull()) { return; }

    QImage image = *bitmapImage->image();

    // Autocrop should not be used here, otherwise the image is adjusted slightly from its original position..
    mMyPaint->loadImage(image, bitmapImage->bounds().topLeft());
}

void ScribbleArea::prepareForDrawing()
{
    qDebug() << "prepare for drawing";

    Layer* layer = mEditor->layers()->currentLayer();

    mMyPaint->clearSurface();
    switch(layer->type()) {
        case Layer::BITMAP:
        {
            updateMyPaintCanvas();
            break;
        }
        default:
            break;
    }
}

/************************************************************************************/
// update methods

void ScribbleArea::updateCurrentFrame()
{
    updateFrame(mEditor->currentFrame());
}

void ScribbleArea::updateFrame(int frame)
{
    Q_ASSERT(frame >= 0);
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

void ScribbleArea::reloadMyPaint()
{
    mNeedLoadImageToMyPaint = true;
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
    invalidateLayerPixmapCache();
    mEditor->layers()->currentLayer()->clearDirtyFrames();

    update();
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

void ScribbleArea::invalidateLayerPixmapCache()
{
    mCanvasPainter.resetLayerCache();
    update();
}

void ScribbleArea::onPlayStateChanged()
{
    int currentFrame = mEditor->currentFrame();
    if (mPrefs->isOn(SETTING::PREV_ONION) ||
        mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidateLayerPixmapCache();
    }

    updateFrame(currentFrame);
}

void ScribbleArea::onScrubbed(int frameNumber)
{
    // Maybe consider checking whether a stroke was made on the previous frame
    // instead of blindly clearing the mypaint surface
    reloadMyPaint();
    invalidateLayerPixmapCache();
    updateFrame(frameNumber);
}

void ScribbleArea::onFramesModified()
{
    invalidateCacheForDirtyFrames();
    if (mPrefs->isOn(SETTING::PREV_ONION) || mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidateLayerPixmapCache();
    }
    update();
}

void ScribbleArea::onCurrentFrameModified()
{
    onFrameModified(mEditor->currentFrame());
}

void ScribbleArea::onFrameModified(int frameNumber)
{
    mNeedLoadImageToMyPaint = true;
    if (mPrefs->isOn(SETTING::PREV_ONION) || mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidateOnionSkinsCacheAround(frameNumber);
        invalidateLayerPixmapCache();
    }
    invalidateCacheForFrame(frameNumber);
    updateFrame(frameNumber);
}

void ScribbleArea::onDidDraw(int frameNumber)
{
    if (mPrefs->isOn(SETTING::PREV_ONION) || mPrefs->isOn(SETTING::NEXT_ONION)) {
        invalidateOnionSkinsCacheAround(frameNumber);
        invalidateLayerPixmapCache();
    }
    invalidateCacheForFrame(frameNumber);
    updateFrame(frameNumber);
}

void ScribbleArea::onViewChanged()
{
    invalidateAllCache();
}

void ScribbleArea::onLayerChanged()
{
    Layer* layer = mEditor->layers()->currentLayer();
    switch(layer->type())
    {
    case Layer::BITMAP:
        reloadMyPaint();
        break;
    case Layer::VECTOR:
        break;
    default:
        break;
    }

    invalidateAllCache();
}

void ScribbleArea::onSelectionChanged()
{
    update();
}

void ScribbleArea::onOnionSkinTypeChanged()
{
    invalidateAllCache();
}

void ScribbleArea::onObjectLoaded()
{
    reloadMyPaint();
    invalidateAllCache();
}

void ScribbleArea::setModified(const Layer* layer, int frameNumber)
{
    if (layer == nullptr) { return; }

    layer->setModified(frameNumber, true);

    onFrameModified(frameNumber);
}

void ScribbleArea::setModified(int layerNumber, int frameNumber)
{
    Layer* layer = mEditor->object()->getLayer(layerNumber);
    if (layer == nullptr) { return; }

    setModified(layer, frameNumber);
}

bool ScribbleArea::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate) {
        mEditor->tools()->deactivateTemporaryTool();
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

    mKeyboardInUse = true;

    if (isPointerInUse()) { return; } // prevents shortcuts calls while drawing
    if (mEditor->tools()->temporaryToolActive()) { return; } // prevents shortcuts calls while using instant tool

    if (currentTool()->keyPressEvent(event))
    {
        return; // has been handled by tool
    }

    // --- fixed control key shortcuts ---
    if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
    {
        mEditor->tools()->setTemporaryTool(ERASER);
        return;
    }

    // ---- fixed normal keys ----

    auto selectMan = mEditor->select();
    bool isSomethingSelected = selectMan->somethingSelected();
    if (isSomethingSelected) {
        keyEventForSelection(event);
    } else {
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
        paintTransformedSelection();
        break;
    case Qt::Key_Left:
        selectMan->translate(QPointF(-1, 0));
        paintTransformedSelection();
        break;
    case Qt::Key_Up:
        selectMan->translate(QPointF(0, -1));
        paintTransformedSelection();
        break;
    case Qt::Key_Down:
        selectMan->translate(QPointF(0, 1));
        paintTransformedSelection();
        break;
    case Qt::Key_Return:
        applyTransformedSelection();
        mEditor->deselectAll();
        break;
    case Qt::Key_Escape:
        cancelTransformedSelection();
        mEditor->deselectAll();
        break;
    case Qt::Key_Backspace:
        deleteSelection();
        mEditor->deselectAll();
        break;
    case Qt::Key_Space:
        mEditor->tools()->setTemporaryTool(HAND); // just call "setTemporaryTool()" to activate temporarily any tool
        break;
    default:
        event->ignore();
    }
}

void ScribbleArea::keyEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Right:
        mEditor->scrubForward();
        event->ignore();
        break;
    case Qt::Key_Left:
        mEditor->scrubBackward();
        event->ignore();
        break;
    case Qt::Key_Up:
        mEditor->layers()->gotoNextLayer();
        event->ignore();
        break;
    case Qt::Key_Down:
        mEditor->layers()->gotoPreviouslayer();
        event->ignore();
        break;
    case Qt::Key_Return:
        event->ignore();
        break;
    case Qt::Key_Space:
        mEditor->tools()->setTemporaryTool(HAND); // just call "setTemporaryTool()" to activate temporarily any tool
        break;
    default:
        event->ignore();
    }
}

void ScribbleArea::keyReleaseEvent(QKeyEvent *event)
{
    // Don't handle this event on auto repeat
    //
    if (event->isAutoRepeat()) {
        return;
    }

    mKeyboardInUse = false;

    if (isPointerInUse()) { return; }

    if (mEditor->tools()->temporaryToolActive()) // temporary tool
    {
        currentTool()->keyReleaseEvent(event);
        mEditor->tools()->deactivateTemporaryTool();
        return;
    }
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
    // Don't change view if tool is in use
    if (isPointerInUse()) return;

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::CAMERA && !layer->visible())
    {
        showLayerNotVisibleWarning(); // FIXME: crash when using tablets
        return;
    }

    const QPoint pixels = event->pixelDelta();
    const QPoint angle = event->angleDelta();
    const QPointF offset = mEditor->view()->mapScreenToCanvas(event->posF());

    const qreal currentScale = mEditor->view()->scaling();
    if (!pixels.isNull())
    {
        // XXX: This pixel-based zooming algorithm currently has some shortcomings compared to the angle-based one:
        //      Zooming in is faster than zooming out and scrolling twice with delta x yields different zoom than
        //      scrolling once with delta 2x. Someone with the ability to test this code might want to "upgrade" it.
        const int delta = pixels.y();
        const qreal newScale = currentScale * (1 + (delta * 0.01));
        mEditor->view()->scaleWithOffset(newScale, offset);
    }
    else if (!angle.isNull())
    {
        const int delta = angle.y();
        // 12 rotation steps at "standard" wheel resolution (120/step) result in 100x zoom
        const qreal newScale = currentScale * std::pow(100, delta / (12.0 * 120));
        mEditor->view()->scaleWithOffset(newScale, offset);
    }
    updateCanvasCursor();
    event->accept();
}

void ScribbleArea::tabletEvent(QTabletEvent *e)
{
    PointerEvent event(e);

    if (event.pointerType() == QTabletEvent::Eraser)
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
    if ((currentTool()->type() != HAND || isCameraLayer) && (event->button() != Qt::RightButton) && (event->button() != Qt::MidButton || isCameraLayer))
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

    if (event->buttons() & (Qt::MidButton | Qt::RightButton))
    {
        mEditor->tools()->setTemporaryTool(HAND);
        getTool(HAND)->pointerPressEvent(event);
    }

    const bool isPressed = event->buttons() & Qt::LeftButton;
    if (isPressed && mQuickSizing)
    {
        if (currentTool()->startAdjusting(event->modifiers(), 1))
        {
            updateCanvasCursor();
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

    if (event->buttons() == Qt::RightButton)
    {
        setCursor(getTool(HAND)->cursor());
        getTool(HAND)->pointerMoveEvent(event);
        event->accept();
        return;
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

    if (event->buttons() & (Qt::RightButton | Qt::MiddleButton))
    {
        mMouseRightButtonInUse = false;
        return;
    }

    //qDebug() << "release event";
    currentTool()->pointerReleaseEvent(event);

    // ---- last check (at the very bottom of mouseRelease) ----
    if (mEditor->tools()->temporaryToolActive() && !mKeyboardInUse) // temp tool and released all keys ?
    {
        mEditor->tools()->deactivateTemporaryTool();
    }
    updateCanvasCursor();
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

bool ScribbleArea::isLayerPaintable() const
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return false; }

    return layer->type() == Layer::BITMAP || layer->type() == Layer::VECTOR;
}

void ScribbleArea::mousePressEvent(QMouseEvent* e)
{
    if (mTabletInUse)
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
    PointerEvent event(e);

    mStrokeManager->pointerMoveEvent(&event);

    pointerMoveEvent(&event);
}

void ScribbleArea::mouseReleaseEvent(QMouseEvent* e)
{
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
    mCanvas = QPixmap(size());
    mCanvas.fill(Qt::transparent);

    mMyPaint->setSurfaceSize(size());
    mEditor->view()->setCanvasSize(size());

    invalidateCacheForFrame(mEditor->currentFrame());
    invalidateLayerPixmapCache();
}

void ScribbleArea::showLayerNotVisibleWarning()
{
    QMessageBox::warning(this, tr("Warning"),
                         tr("You are trying to modify a hidden layer! Please select another layer (or make the current layer visible)."),
                         QMessageBox::Ok,
                         QMessageBox::Ok);
}

void ScribbleArea::paintBitmapBuffer(QPainter::CompositionMode composition)
{
    LayerBitmap* layer = static_cast<LayerBitmap*>(mEditor->layers()->currentLayer());
    Q_ASSERT(layer);
    Q_ASSERT(layer->type() == Layer::BITMAP);

    int frameNumber = mEditor->currentFrame();

    // If there is no keyframe at or before the current position,
    // just return (since we have nothing to paint on).
    if (layer->getLastKeyFrameAtPosition(frameNumber) == nullptr)
    {
        updateCurrentFrame();
        return;
    }

    // Clear the temporary pixel path
    BitmapImage* targetImage = currentBitmapImage(layer);
    if (targetImage == nullptr) { return; }

    // We use source here because mypaint contains the same image as target image..
    QPainter::CompositionMode cm = composition;
    switch (currentTool()->type())
    {
    case ERASER:
    case BRUSH:
    case PEN:
    case PENCIL:
        if (getTool(currentTool()->type())->properties.preserveAlpha)
        {
            cm = QPainter::CompositionMode_SourceAtop;
        }
        break;
    default: //nothing
        break;
    }

    // adds content from canvas and saves to bitmapimage
    for (MPTile* item : mBufferTiles.values()) {
        QPixmap tilePixmap = item->pixmap();
        targetImage->paste(tilePixmap, item->pos(), cm);
    }

    layer->setModified(frameNumber, true);

    // Update the cache for the last key-frame.
    updateFrame(frameNumber);
    update();
}

void ScribbleArea::clearBitmapBuffer()
{
    mBufferImg->clear();
}

void ScribbleArea::drawLine(QPointF P1, QPointF P2, QPen pen, QPainter::CompositionMode cm)
{
    mBufferImg->drawLine(P1, P2, pen, cm, mPrefs->isOn(SETTING::ANTIALIAS));
}

void ScribbleArea::drawPath(QPainterPath path, QPen pen, QBrush brush, QPainter::CompositionMode cm)
{
    mBufferImg->drawPath(path, pen, brush, cm, mPrefs->isOn(SETTING::ANTIALIAS));
}

void ScribbleArea::refreshBitmap(const QRectF& rect, int rad)
{
    QRectF updatedRect = mEditor->view()->mapCanvasToScreen(rect.normalized().adjusted(-rad, -rad, +rad, +rad));
    update(updatedRect.toRect());
}

void ScribbleArea::refreshVector(const QRectF& rect, int rad)
{
    rad += 1;
    update(rect.normalized().adjusted(-rad, -rad, +rad, +rad).toRect());
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

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawPixmap(QPoint(static_cast<int>(mTransformedCursorPos.x() - mCursorCenterPos.x()),
                              static_cast<int>(mTransformedCursorPos.y() - mCursorCenterPos.y())),
                       mCursorImg);

    // update center of transformed img for rect only
    mTransCursImg = mCursorImg.transformed(view);

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
        mCursorImg = QPixmap(); // if above does not comply, deallocate image
    }

    // update cursor rect
    QPoint translatedPos = QPoint(static_cast<int>(mTransformedCursorPos.x() - mCursorCenterPos.x()),
                                  static_cast<int>(mTransformedCursorPos.y() - mCursorCenterPos.y()));

    update(mTransCursImg.rect().adjusted(-1, -1, 1, 1)
           .translated(translatedPos));
}

void ScribbleArea::handleDrawingOnEmptyFrame()
{
    auto layer = mEditor->layers()->currentLayer();

    if (!layer || !layer->isPaintable())
    {
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
    if (!currentTool()->isActive())
    {
        // --- we retrieve the canvas from the cache; we create it if it doesn't exist
        const int currentFrame = mEditor->currentFrame();
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
                drawCanvas(mEditor->currentFrame(), event->rect());
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
        prepCanvas(mEditor->currentFrame(), event->rect());
        prepOverlays();
        mCanvasPainter.paintCached();
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
    painter.drawPixmap(QPoint(0, 0), mCanvas);

    Layer* layer = mEditor->layers()->currentLayer();

    if (!editor()->playback()->isPlaying())    // we don't need to display the following when the animation is playing
    {
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

        if (currentTool()->isDrawingTool()) {
            paintCanvasCursor(painter);
        }

        mCanvasPainter.renderGrid(painter);
        mOverlayPainter.renderOverlays(painter, editor()->overlays()->getMoveMode());

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
    selectMan->updatePolygons();

    if (selectMan->currentSelectionPolygonF().isEmpty()) { return; }
    if (selectMan->currentSelectionPolygonF().count() < 4) { return; }

    QPolygonF lastSelectionPolygon = editor()->view()->mapPolygonToScreen(selectMan->lastSelectionPolygonF());
    QPolygonF currentSelectionPolygon = editor()->view()->mapPolygonToScreen(selectMan->currentSelectionPolygonF());

    TransformParameters params = { lastSelectionPolygon, currentSelectionPolygon };
    mSelectionPainter.paint(painter, object, mEditor->currentLayerIndex(), currentTool(), params);
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

void ScribbleArea::prepCanvas(int frame, QRect rect)
{
    Object* object = mEditor->object();

    CanvasPainterOptions o;
    o.bPrevOnionSkin = mPrefs->isOn(SETTING::PREV_ONION);
    o.bNextOnionSkin = mPrefs->isOn(SETTING::NEXT_ONION);
    o.bColorizePrevOnion = mPrefs->isOn(SETTING::ONION_RED);
    o.bColorizeNextOnion = mPrefs->isOn(SETTING::ONION_BLUE);
    o.nPrevOnionSkinCount = mPrefs->getInt(SETTING::ONION_PREV_FRAMES_NUM);
    o.nNextOnionSkinCount = mPrefs->getInt(SETTING::ONION_NEXT_FRAMES_NUM);
    o.fOnionSkinMaxOpacity = mPrefs->getInt(SETTING::ONION_MAX_OPACITY);
    o.fOnionSkinMinOpacity = mPrefs->getInt(SETTING::ONION_MIN_OPACITY);
    o.bAntiAlias = mPrefs->isOn(SETTING::ANTIALIAS);
    o.bGrid = mPrefs->isOn(SETTING::GRID);
    o.nGridSizeW = mPrefs->getInt(SETTING::GRID_SIZE_W);
    o.nGridSizeH = mPrefs->getInt(SETTING::GRID_SIZE_H);
    o.bAxis = false;
    o.bThinLines = mPrefs->isOn(SETTING::INVISIBLE_LINES);
    o.bOutlines = mPrefs->isOn(SETTING::OUTLINES);
    o.eLayerVisibility = mLayerVisibility;
    o.fLayerVisibilityThreshold = mPrefs->getFloat(SETTING::LAYER_VISIBILITY_THRESHOLD);
    o.bIsOnionAbsolute = (mPrefs->getString(SETTING::ONION_TYPE) == "absolute");
    o.scaling = mEditor->view()->scaling();
    o.onionWhilePlayback = mPrefs->getInt(SETTING::ONION_WHILE_PLAYBACK);
    o.isPlaying = mEditor->playback()->isPlaying() ? true : false;
    o.cmBufferBlendMode = mEditor->tools()->currentTool()->type() == ToolType::ERASER ? QPainter::CompositionMode_DestinationOut : QPainter::CompositionMode_SourceOver;
    o.tilesToBeRendered = mBufferTiles.values();
    o.isPainting = mIsPainting;

    if (currentTool()->type() == POLYLINE) {
        o.useCanvasBuffer = true;
    } else {
        o.useCanvasBuffer = false;
    }

    mCanvasPainter.setOptions(o);
    mCanvasPainter.setCanvas(&mCanvas);

    ViewManager* vm = mEditor->view();
    mCanvasPainter.setViewTransform(vm->getView());

    mCanvasPainter.setPaintSettings(object, mEditor->layers()->currentLayerIndex(), frame, rect, mBufferImg);
}

void ScribbleArea::drawCanvas(int frame, QRect rect)
{
    mCanvas.fill(Qt::transparent);
    prepCanvas(frame, rect);
    mCanvasPainter.paint();
    prepOverlays();
}

void ScribbleArea::setGaussianGradient(QGradient &gradient, QColor colour, qreal opacity, qreal offset)
{
    if (offset < 0) { offset = 0; }
    if (offset > 100) { offset = 100; }

    int r = colour.red();
    int g = colour.green();
    int b = colour.blue();
    qreal a = colour.alphaF();

    int mainColorAlpha = qRound(a * 255 * opacity);

    // the more feather (offset), the more softness (opacity)
    int alphaAdded = qRound((mainColorAlpha * offset) / 100);

    gradient.setColorAt(0.0, QColor(r, g, b, mainColorAlpha - alphaAdded));
    gradient.setColorAt(1.0, QColor(r, g, b, 0));
    gradient.setColorAt(1.0 - (offset / 100.0), QColor(r, g, b, mainColorAlpha - alphaAdded));
}

void ScribbleArea::renderOverlays()
{
    updateCurrentFrame();
}

void ScribbleArea::prepOverlays()
{
    OverlayPainterOptions o;

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

    o.mRect = getCameraRect().toRect();   // camera rect!
    o.mSinglePerspPoint = mEditor->overlays()->getSinglePerspPoint();
    o.mLeftPerspPoint = mEditor->overlays()->getLeftPerspPoint();
    o.mRightPerspPoint = mEditor->overlays()->getRightPerspPoint();
    o.mMiddlePerspPoint = mEditor->overlays()->getMiddlePerspPoint();

    mOverlayPainter.setOptions(o);

    ViewManager* vm = mEditor->view();
    mOverlayPainter.setViewTransform(vm->getView());
}

void ScribbleArea::loadMPBrush(const QByteArray &content)
{
    mMyPaint->loadBrush(content);

    forceUpdateMyPaintStates();
    updateCanvasCursor();
}

void ScribbleArea::updateTile(MPSurface *surface, MPTile *tile)
{
    Q_UNUSED(surface)

    QPoint pos = tile->pos();

    tile->setDirty(true);

    mBufferTiles.insert(QString::number(pos.x())+"_"+QString::number(pos.y()), tile);
}

void ScribbleArea::clearTile(MPSurface *surface, MPTile *tile)
{
    Q_UNUSED(surface)

    QPointF pos = tile->pos();

    mBufferTiles.remove(QString::number(pos.x())+"_"+QString::number(pos.y()));
}

/************************************************************************************/
// Stroke Handling

void ScribbleArea::startStroke()
{

    if (mNeedLoadImageToMyPaint) {
        qDebug() << "first frame load";
        prepareForDrawing();
        mNeedLoadImageToMyPaint = false;
    }
    mMyPaint->startStroke();
    mIsPainting = true;
}

void ScribbleArea::strokeTo(QPointF point, float pressure, float xtilt, float ytilt, double dt)
{
//    qDebug() << "stroke to: " << point;
    mMyPaint->strokeTo(static_cast<float>(point.x()), static_cast<float>(point.y()), pressure, xtilt, ytilt, dt);

    // update dirty region
    updateDirtyTiles();
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

void ScribbleArea::updateDirtyTiles()
{
    QTransform v = mEditor->view()->getView();
    QHashIterator<QString, MPTile*> i(mBufferTiles);
    int counter = 0;
    while (i.hasNext()) {
        i.next();
        MPTile* tile = i.value();
        const QPointF& tilePos = tile->pos();
        const QSizeF& tileSize = tile->boundingRect().size();
        if (tile->isDirty()) {

            const QRectF& mappedRect = v.mapRect(QRectF(tilePos, tileSize));
            update(mappedRect.toRect());

            tile->setDirty(false);
//            qDebug() << "found dirty tile: ";
//            qDebug() << "tile clean counter" << counter;
            counter++;
        }
    }
}

void ScribbleArea::refreshSurface()
{
    mMyPaint->refreshSurface();
}

void ScribbleArea::endStroke()
{
    clearTilesBuffer();

    mIsPainting = false;
    mMyPaint->endStroke();

    onDidDraw(mEditor->currentFrame());
    qDebug() << "end stroke";
}

void ScribbleArea::clearTilesBuffer()
{
    // clear the temp tiles buffer
    if (!mBufferTiles.isEmpty()) {
        mBufferTiles.clear();
    }
}

void ScribbleArea::flipSelection(bool flipVertical)
{
    mEditor->select()->flipSelection(flipVertical);
    paintTransformedSelection();
}

void ScribbleArea::drawPolyline(QPainterPath path, QPen pen, bool useAA)
{
    QRectF updateRect = mEditor->view()->mapCanvasToScreen(path.boundingRect().toRect()).adjusted(-1, -1, 1, 1);

    // Update region outside updateRect
    QRectF boundingRect = updateRect.adjusted(-width(), -height(), width(), height());
    mBufferImg->clear();
    mBufferImg->drawPath(path, pen, Qt::NoBrush, QPainter::CompositionMode_SourceOver, useAA);
    update(boundingRect.toRect());

}

/************************************************************************************/
// view handling

QRectF ScribbleArea::getCameraRect()
{
    return mCanvasPainter.getCameraRect();
}

QPointF ScribbleArea::getCentralPoint()
{
    return mEditor->view()->mapScreenToCanvas(QPointF(width() / 2, height() / 2));
}

void ScribbleArea::paintTransformedSelection()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr)
    {
        return;
    }

    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())    // there is something selected
    {
        if (layer->type() == Layer::BITMAP)
        {
            mCanvasPainter.setTransformedSelection(selectMan->mySelectionRect().toRect(),
                                                   selectMan->myTempTransformedSelectionRect().toRect(),
                                                   selectMan->selectionTransform(),
                                                   selectMan->transformHasBeenModified());
        }
        else if (layer->type() == Layer::VECTOR)
        {
            // vector transformation
            VectorImage* vectorImage = currentVectorImage(layer);
            vectorImage->setSelectionTransformation(selectMan->selectionTransform());

        }
        setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    }
    updateCurrentFrame();
}

void ScribbleArea::applySelectionChanges()
{
    // we haven't applied our last modifications yet
    // therefore apply the transformed selection first.
    applyTransformedSelection();


    //// might not be needed anymore...
    auto selectMan = mEditor->select();

    // make sure the current transformed selection is valid
    if (!selectMan->myTempTransformedSelectionRect().isValid())
    {
        const QRectF& normalizedRect = selectMan->myTempTransformedSelectionRect().normalized();
        selectMan->setTempTransformedSelectionRect(normalizedRect);
    }
    selectMan->setSelection(selectMan->myTempTransformedSelectionRect());
    paintTransformedSelection();

    // Calculate the new transformation based on the new selection
    selectMan->calculateSelectionTransformation();

    // apply the transformed selection to make the selection modification absolute.
    applyTransformedSelection();
    //// end

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
            BitmapImage transformed = currentBitmapImage(layer)->transformed(selectMan->mySelectionRect().toRect(), selectMan->selectionTransform(), true);
            BitmapImage* bitmapImage = currentBitmapImage(layer);
            bitmapImage->moveSelectionTransform(selectMan->mySelectionRect().toRect(),
                                   selectMan->selectionTransform());
            mMyPaint->clearAreaFromSurface(selectMan->mySelectionRect().toRect());

            QRect movedRect = selectMan->selectionTransform().mapRect(selectMan->mySelectionRect()).toRect();
            mMyPaint->drawImageAt(*transformed.image(),movedRect.topLeft());
        }
        else if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = currentVectorImage(layer);
            vectorImage->applySelectionTransformation();
            selectMan->setSelection(selectMan->myTempTransformedSelectionRect(), false);
        }

        setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    }

    update();
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

        setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
        updateCurrentFrame();
    }
}

void ScribbleArea::displaySelectionProperties()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }
    if (layer->type() == Layer::VECTOR)
    {
        VectorImage* vectorImage = currentVectorImage(layer);
        //vectorImage->applySelectionTransformation();
        if (currentTool()->type() == MOVE)
        {
            int selectedCurve = vectorImage->getFirstSelectedCurve();
            if (selectedCurve != -1)
            {
                mEditor->tools()->setWidth(vectorImage->curve(selectedCurve).getWidth());
                mEditor->tools()->setFeather(vectorImage->curve(selectedCurve).getFeather());
                mEditor->tools()->setInvisibility(vectorImage->curve(selectedCurve).isInvisible());
                mEditor->tools()->setPressure(vectorImage->curve(selectedCurve).getVariableWidth());
                mEditor->color()->setColorNumber(vectorImage->curve(selectedCurve).getColorNumber());
            }

            int selectedArea = vectorImage->getFirstSelectedArea();
            if (selectedArea != -1)
            {
                mEditor->color()->setColorNumber(vectorImage->mArea[selectedArea].mColorNumber);
            }
        }
    }
}

void ScribbleArea::toggleThinLines()
{
    bool previousValue = mPrefs->isOn(SETTING::INVISIBLE_LINES);
    setEffect(SETTING::INVISIBLE_LINES, !previousValue);
}

void ScribbleArea::toggleOutlines()
{
    mIsSimplified = !mIsSimplified;
    setEffect(SETTING::OUTLINES, mIsSimplified);
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

BaseTool* ScribbleArea::getTool(ToolType eToolType)
{
    return editor()->tools()->getTool(eToolType);
}

void ScribbleArea::setCurrentTool(ToolType eToolMode)
{
    if (currentTool() != nullptr && eToolMode != currentTool()->type())
    {
        qDebug() << "Set Current Tool" << BaseTool::TypeName(eToolMode);
        if (BaseTool::TypeName(eToolMode) == "")
        {
            // tool does not exist
            //Q_ASSERT_X( false, "", "" );
            return;
        }

        if (currentTool()->type() == MOVE)
        {
            paintTransformedSelection();
            mEditor->deselectAll();
        }
        else if (currentTool()->type() == POLYLINE)
        {
            mEditor->deselectAll();
        }
    }

    // change cursor
    setCursor(currentTool()->cursor());
    updateCanvasCursor();
    //qDebug() << "fn: setCurrentTool " << "call: setCursor()" << "current tool" << currentTool()->typeName();
}

void ScribbleArea::deleteSelection()
{
    auto selectMan = mEditor->select();
    if (selectMan->somethingSelected())      // there is something selected
    {
        Layer* layer = mEditor->layers()->currentLayer();
        if (layer == nullptr) { return; }

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
            mMyPaint->clearAreaFromSurface(selectMan->mySelectionRect().toRect());
        }
        setModified(mEditor->currentLayerIndex(), mEditor->currentFrame());
    }
}

void ScribbleArea::clearCanvas()
{
    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (layer->type() == Layer::VECTOR)
    {
        mEditor->backup(tr("Clear Image", "Undo step text"));

        currentVectorImage(layer)->clear();
        mEditor->select()->clearCurves();
        mEditor->select()->clearVertices();
    }
    else if (layer->type() == Layer::BITMAP)
    {
        mEditor->backup(tr("Clear Image", "Undo step text"));
        currentBitmapImage(layer)->clear();
    }
    else
    {
        return; // skip updates when nothing changes
    }

    setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
    mMyPaint->clearSurface();
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

void ScribbleArea::floodFillError(int errorType)
{
    QString message, error;
    if (errorType == 1) { message = tr("There is a gap in your drawing (or maybe you have zoomed too much)."); }
    if (errorType == 2 || errorType == 3) message = tr("Sorry! This doesn't always work."
                                                       "Please try again (zoom a bit, click at another location... )<br>"
                                                       "if it doesn't work, zoom a bit and check that your paths are connected by pressing F1.).");

    if (errorType == 1) { error = tr("Out of bound.", "Bucket tool fill error message"); }
    if (errorType == 2) { error = tr("Could not find a closed path.", "Bucket tool fill error message"); }
    if (errorType == 3) { error = tr("Could not find the root index.", "Bucket tool fill error message"); }
    QMessageBox::warning(this, tr("Flood fill error"), tr("%1<br><br>Error: %2").arg(message, error), QMessageBox::Ok, QMessageBox::Ok);
    mEditor->deselectAll();
}
