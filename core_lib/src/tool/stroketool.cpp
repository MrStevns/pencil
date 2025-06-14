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

#include "stroketool.h"

#include <QKeyEvent>
#include "scribblearea.h"

#include "viewmanager.h"
#include "preferencemanager.h"
#include "selectionmanager.h"
#include "toolmanager.h"
#include "colormanager.h"
#include "layermanager.h"

#include "editor.h"
#include "mathutils.h"
#include "beziercurve.h"
#include "vectorimage.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "layer.h"

#include "QPainterPath"

#include "canvascursorpainter.h"

#ifdef Q_OS_MAC
extern "C" {
    void detectWhichOSX();
    void disableCoalescing();
    void enableCoalescing();
}
#else
extern "C" {
    void detectWhichOSX() {}
    void disableCoalescing() {}
    void enableCoalescing() {}
}
#endif

const qreal StrokeTool::FEATHER_MIN = 1.;
const qreal StrokeTool::FEATHER_MAX = 99.;
const qreal StrokeTool::WIDTH_MIN = 1.;
const qreal StrokeTool::WIDTH_MAX = 200.;

// ---- shared static variables ---- ( only one instance for all the tools )
bool StrokeTool::mQuickSizingEnabled = false;

StrokeTool::StrokeTool(QObject* parent) : BaseTool(parent)
{
    detectWhichOSX();
}

StrokeTool::~StrokeTool()
{
    if (mStrokeSettings) {
        // Technically this is probably not neccesary since a tool exists for the entire
        // lifetime of the program.
        delete(mStrokeSettings);
        mStrokeSettings = nullptr;
    }
}

void StrokeTool::createSettings(ToolSettings* settings)
{
    if (settings == nullptr) {
        mStrokeSettings = new StrokeSettings();
    } else {
        mStrokeSettings = static_cast<StrokeSettings*>(settings);
    }
    BaseTool::createSettings(mStrokeSettings);
}

void StrokeTool::loadSettings()
{
    mQuickSizingEnabled = mEditor->preference()->isOn(SETTING::QUICK_SIZING);
    mCanvasCursorEnabled = mEditor->preference()->isOn(SETTING::CANVAS_CURSOR);

    QSettings settings(PENCIL2D, PENCIL2D);
    QHash<int, PropertyInfo> info;
    info[StrokeSettings::WIDTH_VALUE] = { 1.0, 100.0, 24.0 };
    info[StrokeSettings::FEATHER_VALUE] = { 1.0, 99.0, 48.0 };
    info[StrokeSettings::FEATHER_ENABLED] = false;
    info[StrokeSettings::PRESSURE_ENABLED] = false;
    info[StrokeSettings::INVISIBILITY_ENABLED] = false;
    info[StrokeSettings::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG };
    info[StrokeSettings::ANTI_ALIASING_ENABLED] = false;
    info[StrokeSettings::FILLCONTOUR_ENABLED] = false;

    mStrokeSettings->load(typeName(), settings, info);

    mStrokeDynamics = createDynamics();

    mWidthSizingTool.setup(Qt::ShiftModifier);

    if (mStrokeSettings->featherEnabled()) {
        mFeatherSizingTool.setup(Qt::ControlModifier);
    }

    connect(&mWidthSizingTool, &RadialOffsetTool::offsetChanged, this, [=](qreal offset) {
        setWidth(offset * 2.0);
    });

    connect(&mFeatherSizingTool, &RadialOffsetTool::offsetChanged, this, [=](qreal offset){
        auto featherInfo = mStrokeSettings->getInfo(StrokeSettings::FEATHER_VALUE);
        const qreal inputMin = featherInfo.minReal();
        const qreal inputMax = mStrokeSettings->width() * 0.5;
        const qreal outputMax = featherInfo.maxReal();
        const qreal outputMin = inputMin;

        // We map the feather value to a value between the min width and max width
        const qreal mappedValue = MathUtils::map(offset, inputMin, inputMax, outputMin, outputMax);

        setFeather(mappedValue);
    });
}

bool StrokeTool::enteringThisTool()
{
    mActiveConnections.append(connect(mEditor->view(), &ViewManager::viewChanged, this, &StrokeTool::onViewUpdated));
    return true;
}

void StrokeTool::onPreferenceChanged(SETTING setting)
{
    if (setting == SETTING::QUICK_SIZING) {
        mQuickSizingEnabled = mEditor->preference()->isOn(setting);
    } else if (setting == SETTING::CANVAS_CURSOR) {
        mCanvasCursorEnabled = mEditor->preference()->isOn(setting);
    }
}

void StrokeTool::onViewUpdated()
{
    updateCanvasCursor();
}

QPainter::CompositionMode StrokeTool::compositionMode() const
{
    return mStrokeDynamics.blending;
}

QPointF StrokeTool::getCurrentPressPixel() const
{
    return mInterpolator.getCurrentPressPixel();
}

QPointF StrokeTool::getCurrentPressPoint() const
{
    return mEditor->view()->mapScreenToCanvas(mInterpolator.getCurrentPressPixel());
}

QPointF StrokeTool::getCurrentPixel() const
{
    return mInterpolator.getCurrentPixel();
}

QPointF StrokeTool::getCurrentPoint() const
{
    return mEditor->view()->mapScreenToCanvas(getCurrentPixel());
}

QPointF StrokeTool::getLastPixel() const
{
    return mInterpolator.getLastPixel();
}

QPointF StrokeTool::getLastPoint() const
{
    return mEditor->view()->mapScreenToCanvas(getLastPixel());
}

void StrokeTool::startStroke(PointerEvent::InputType inputType)
{
    if (emptyFrameActionEnabled())
    {
        mScribbleArea->handleDrawingOnEmptyFrame();
    }

    mStrokeDynamics = createDynamics();

    mFirstDraw = true;

    mStrokePoints.clear();

    //Experimental
    QPointF startStroke = mInterpolator.interpolateStart(getCurrentPixel());
    startStroke = mEditor->view()->mapScreenToCanvas(startStroke);
    mStrokePoints << startStroke;

    mStroker.begin(startStroke);

    mStrokePressures.clear();
    mStrokePressures << mInterpolator.getPressure();

    mCurrentInputType = inputType;
    mUndoSaveState = mEditor->undoRedo()->state(UndoRedoRecordType::KEYFRAME_MODIFY);

    disableCoalescing();
}

bool StrokeTool::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Alt:
        if (mEditor->tools()->setTemporaryTool(EYEDROPPER, {}, Qt::AltModifier))
        {
            return true;
        }
        break;
    case Qt::Key_Space:
        if (mEditor->tools()->setTemporaryTool(HAND, Qt::Key_Space, Qt::NoModifier))
        {
            return true;
        }
        break;
    }
    return BaseTool::keyPressEvent(event);
}

bool StrokeTool::emptyFrameActionEnabled()
{
    return true;
}

void StrokeTool::endStroke()
{
    applyKeyFrameBuffer();

    mInterpolator.interpolateEnd();
    mStrokePressures << mInterpolator.getPressure();
    mStrokePoints.clear();
    mStrokePressures.clear();
    mStrokeSegment.clear();

    mStroker.end();

    enableCoalescing();

    mEditor->setModified(mEditor->currentLayerIndex(), mEditor->currentFrame());
    mEditor->undoRedo()->record(mUndoSaveState, typeName());
    mScribbleArea->endStroke();
}

StrokeDynamics StrokeTool::createDynamics() const
{
    StrokeDynamics dynamics;

    qreal opacity = (mStrokeSettings->pressureEnabled()) ? (mCurrentPressure * 0.5) : 1.0;
    qreal pressure = (mStrokeSettings->pressureEnabled()) ? mCurrentPressure : 1.0;
    dynamics.canSingleDab = true;
    dynamics.blending = QPainter::CompositionMode_SourceOver;
    dynamics.width = mStrokeSettings->width() * pressure;
    dynamics.pressure = pressure;
    dynamics.opacity = opacity;
    dynamics.dabSpacing = dynamics.width * 0.1;
    dynamics.feather = mStrokeSettings->feather();
    dynamics.color = mEditor->color()->frontColor();
    dynamics.antiAliasingEnabled = mStrokeSettings->AntiAliasingEnabled();

    return dynamics;
}

void StrokeTool::drawStroke()
{
    QPointF pixel = getCurrentPixel();
    mStrokeDynamics = createDynamics();
    if (pixel != getLastPixel() || !mFirstDraw)
    {
        mStrokeSegment = mInterpolator.interpolateStroke();

        for (QPointF& point : mStrokeSegment) {
            point = mEditor->view()->mapScreenToCanvas(point);
        }

        mStroker.append(mStrokeSegment);
        mStrokePressures << mInterpolator.getPressure();
    }
    else
    {
        mFirstDraw = false;
    }
}

void StrokeTool::doStroke()
{
    const StrokeDynamics& dynamics = mStrokeDynamics;
    for (const QPointF& point : mStroker.segment(dynamics)) {
        drawDab(point, dynamics);
    }
}

void StrokeTool::doPath(const QList<QPointF>& points, QBrush brush, QPen pen)
{
    if (points.size() < 4) { return; }

    QPainterPath path;
    QPointF startPoint = points[0];
    path.moveTo(startPoint);
    mStrokePoints << startPoint;
    for (int i = 1; i < points.count(); i += 1) {
        const QPointF& point = points[i];
        path.lineTo(point);
        mStrokePoints << point;
    }

    drawPath(path, pen, brush);
}

void StrokeTool::applyKeyFrameBuffer()
{
    Layer* currentLayer = mEditor->layers()->currentLayer();
    int currentFrame = mEditor->currentFrame();

    if (currentLayer->type() == Layer::BITMAP) {
        BitmapImage* bitmapImage = static_cast<LayerBitmap*>(currentLayer)->getLastBitmapImageAtFrame(currentFrame);
        if (bitmapImage) {
            applyBitmapBuffer(bitmapImage);
        }
    } else if (currentLayer->type() == Layer::VECTOR) {
        VectorImage* vectorImage = static_cast<LayerVector*>(currentLayer)->getLastVectorImageAtFrame(currentFrame, 0);
        if (vectorImage && mStrokePoints.count() > 0) {
            applyVectorBuffer(vectorImage);
        }
    }
}

void StrokeTool::applyVectorBuffer(VectorImage*)
{
    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}

void StrokeTool::applyBitmapBuffer(BitmapImage* bitmapImage)
{
    bitmapImage->paste(&mScribbleArea->mTiledBuffer, mStrokeDynamics.blending);
}

bool StrokeTool::handleQuickSizing(PointerEvent* event)
{
    if (!mQuickSizingEnabled || !mQuickSizingProperties.contains(event->modifiers())) {
        return false;
    }

    if (event->eventType() == PointerEvent::Press) {
        switch (mQuickSizingProperties.value(event->modifiers())) {
            case StrokeSettings::WIDTH_VALUE: {
                mWidthSizingTool.setOffset(mStrokeSettings->width() * 0.5);
                break;
            }
            case StrokeSettings::FEATHER_VALUE: {
                const qreal factor = 0.5;
                const qreal cursorRad = mStrokeSettings->width() * factor;
                auto info = mStrokeSettings->getInfo(StrokeSettings::FEATHER_VALUE);
                const qreal featherWidthFactor = 1 - MathUtils::normalize(mStrokeSettings->feather(), info.minReal(), info.maxReal());
                const qreal offset = (cursorRad * featherWidthFactor);
                mFeatherSizingTool.setOffset(offset);
                break;
            }
        }
    }

    mWidthSizingTool.pointerEvent(event);
    if (mStrokeSettings->featherEnabled()) {
        mFeatherSizingTool.pointerEvent(event);
    }

    updateCanvasCursor();
    return true;
}

void StrokeTool::pointerPressEvent(PointerEvent* event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    startStroke(event->inputType());

    updateCanvasCursor();
}

void StrokeTool::pointerMoveEvent(PointerEvent* event)
{
    mCurrentPressure = event->pressure();
    mInterpolator.pointerMoveEvent(event);

    if (handleQuickSizing(event)) {
        return;
    }

    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        drawStroke();
    }

    updateCanvasCursor();
}

void StrokeTool::pointerReleaseEvent(PointerEvent* event)
{
    mInterpolator.pointerReleaseEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->inputType() != mCurrentInputType) return;

    mEditor->backup(typeName());

    qreal distance = QLineF(getCurrentPoint(), getCurrentPressPoint()).length();
    const StrokeDynamics& dynamics = mStrokeDynamics;
    if (distance < 1 && dynamics.canSingleDab)
    {
        drawDab(getCurrentPressPoint(), dynamics);
    }
    else
    {
        drawStroke();
    }

    endStroke();

    updateCanvasCursor();
}

bool StrokeTool::enterEvent(QEnterEvent*)
{
    mCanvasCursorEnabled = mEditor->preference()->isOn(SETTING::CANVAS_CURSOR);
    return true;
}

bool StrokeTool::leaveEvent(QEvent*)
{
    if (isActive())
    {
        return false;
    }

    mCanvasCursorEnabled = false;
    updateCanvasCursor();
    return true;
}

QRectF StrokeTool::cursorRect(StrokeSettings::Type settingType, const QPointF& point)
{
    const qreal brushWidth = mStrokeSettings->width();
    const qreal brushFeather = mStrokeSettings->feather();

    const QPointF& cursorPos = point;
    const qreal cursorRad = brushWidth * 0.5;
    const QPointF& widthCursorTopLeft = QPointF(cursorPos.x() - cursorRad, cursorPos.y() - cursorRad);

    const QRectF widthCircleRect = QRectF(widthCursorTopLeft, QSizeF(brushWidth, brushWidth));
    if (settingType == StrokeSettings::WIDTH_VALUE) {
        return widthCircleRect;
    } else if (settingType == StrokeSettings::FEATHER_VALUE) {
        const qreal featherWidthFactor =  1 - MathUtils::normalize(brushFeather, 0.0, FEATHER_MAX);
        QRectF featherRect = QRectF(widthCircleRect.center().x() - (cursorRad * featherWidthFactor),
                                     widthCircleRect.center().y() - (cursorRad * featherWidthFactor),
                                     brushWidth * featherWidthFactor,
                                     brushWidth * featherWidthFactor);

        // Adjust the feather rect so it doesn't colide with the width rect;
        // as this cancels out both circles when painted
        return featherRect.adjusted(2, 2, -2, -2);
    }

    return QRectF();
}

void StrokeTool::updateCanvasCursor()
{
    CanvasCursorPainterOptions widthOptions;
    widthOptions.circleRect = cursorRect(StrokeSettings::WIDTH_VALUE, mWidthSizingTool.isAdjusting() ? mWidthSizingTool.offsetPoint() : getCurrentPoint());
    widthOptions.showCursor = mCanvasCursorEnabled;
    widthOptions.showCross = true;

    CanvasCursorPainterOptions featherOptions;
    featherOptions.circleRect = cursorRect(StrokeSettings::FEATHER_VALUE, mFeatherSizingTool.isAdjusting() ? mFeatherSizingTool.offsetPoint() : getCurrentPoint());
    featherOptions.showCursor = mCanvasCursorEnabled;
    featherOptions.showCross = false;

    if (mFeatherSizingTool.isAdjusting()) {
        widthOptions.circleRect = cursorRect(StrokeSettings::WIDTH_VALUE, mFeatherSizingTool.isAdjusting() ? mFeatherSizingTool.offsetPoint() : getCurrentPoint());
    } else if (mWidthSizingTool.isAdjusting()) {
        featherOptions.circleRect = cursorRect(StrokeSettings::FEATHER_VALUE, mWidthSizingTool.isAdjusting() ? mWidthSizingTool.offsetPoint() : getCurrentPoint());
    }

    mWidthCursorPainter.preparePainter(widthOptions);
    mFeatherCursorPainter.preparePainter(featherOptions);

    const QRect& dirtyRect = mWidthCursorPainter.dirtyRect();

    // We know that the width rect is bigger than the feather rect
    // so we don't need to change this
    const QRect& updateRect = widthOptions.circleRect.toAlignedRect();

    // Adjusted to account for some pixel bleeding outside the update rect
    mScribbleArea->update(mEditor->view()->getView().mapRect(updateRect.united(dirtyRect).adjusted(-2, -2, 2, 2)));
    mWidthCursorPainter.clearDirty();
}

void StrokeTool::paint(QPainter& painter, const QRect& blitRect)
{
    painter.save();
    painter.setTransform(mEditor->view()->getView());

    if (mStrokeSettings->featherEnabled()) {
        mFeatherCursorPainter.paint(painter, blitRect);
    }

    mWidthCursorPainter.paint(painter, blitRect);

    painter.restore();
}

void StrokeTool::setStablizationLevel(int level)
{
    mStrokeSettings->setBaseValue(StrokeSettings::STABILIZATION_VALUE, level);
    mInterpolator.setStabilizerLevel(level);
    emit stabilizationLevelChanged(level);
}

void StrokeTool::setFeatherEnabled(bool enabled)
{
    mStrokeSettings->setBaseValue(StrokeSettings::FEATHER_ENABLED, enabled);
    emit featherEnabledChanged(enabled);
}

void StrokeTool::setFeather(qreal feather)
{
    mStrokeSettings->setBaseValue(StrokeSettings::FEATHER_VALUE, feather);

    qreal newFeather = mStrokeSettings->feather();

    mFeatherSizingTool.setOffset(newFeather);
    emit featherChanged(newFeather);
}

void StrokeTool::setWidth(qreal width)
{
    mStrokeSettings->setBaseValue(StrokeSettings::WIDTH_VALUE, width);

    qreal newWidth = mStrokeSettings->width();
    mWidthSizingTool.setOffset(newWidth);
    emit widthChanged(newWidth);
}

void StrokeTool::setPressureEnabled(bool enabled)
{
    mStrokeSettings->setBaseValue(StrokeSettings::PRESSURE_ENABLED, enabled);
    emit pressureEnabledChanged(enabled);
}

void StrokeTool::setFillContourEnabled(bool enabled)
{
    mStrokeSettings->setBaseValue(StrokeSettings::FILLCONTOUR_ENABLED, enabled);
    emit fillContourEnabledChanged(enabled);
}

void StrokeTool::setAntiAliasingEnabled(bool enabled)
{
    mStrokeSettings->setBaseValue(StrokeSettings::ANTI_ALIASING_ENABLED, enabled);
    emit antiAliasingEnabledChanged(enabled);
}

void StrokeTool::setStrokeInvisibleEnabled(bool enabled)
{
    mStrokeSettings->setBaseValue(StrokeSettings::INVISIBILITY_ENABLED, enabled);
    emit InvisibleStrokeEnabledChanged(enabled);
}
