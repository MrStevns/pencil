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
#include <QPainterPath>
#include "scribblearea.h"
#include "viewmanager.h"
#include "preferencemanager.h"
#include "colormanager.h"
#include "layermanager.h"
#include "toolmanager.h"
#include "editor.h"
#include "mathutils.h"

#include "layerbitmap.h"
#include "layervector.h"

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
}

void StrokeTool::loadSettings()
{
    mQuickSizingEnabled = mEditor->preference()->isOn(SETTING::QUICK_SIZING);
    mCanvasCursorEnabled = mEditor->preference()->isOn(SETTING::CANVAS_CURSOR);

    QSettings pencilSettings(PENCIL2D, PENCIL2D);
    QHash<int, PropertyInfo> info;
    info[StrokeToolProperties::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 24.0 };
    info[StrokeToolProperties::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 48.0 };
    info[StrokeToolProperties::FEATHER_ENABLED] = false;
    info[StrokeToolProperties::PRESSURE_ENABLED] = false;
    info[StrokeToolProperties::INVISIBILITY_ENABLED] = false;
    info[StrokeToolProperties::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG };
    info[StrokeToolProperties::ANTI_ALIASING_ENABLED] = false;
    info[StrokeToolProperties::FILLCONTOUR_ENABLED] = false;

    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    /// Given the way that we update preferences currently, this connection should not be removed
    /// when the tool is not active.
    connect(mEditor->preference(), &PreferenceManager::optionChanged, this, &StrokeTool::onPreferenceChanged);

    connect(&mWidthSizingTool, &RadialOffsetTool::offsetChanged, this, [=](qreal offset) {
        setWidth(offset * 2.0);
    });

    connect(&mFeatherSizingTool, &RadialOffsetTool::offsetChanged, this, [=](qreal offset){
        const qreal inputMin = FEATHER_MIN;
        const qreal inputMax = strokeToolProperties().width() * 0.5;
        const qreal outputMax = FEATHER_MAX;
        const qreal outputMin = inputMin;

        // We map the feather value to a value between the min width and max width
        const qreal mappedValue = MathUtils::map(offset, inputMin, inputMax, outputMax, outputMin);

        setFeather(mappedValue);
    });
}

bool StrokeTool::enteringThisTool()
{
    mActiveConnections.append(connect(mEditor->view(), &ViewManager::viewChanged, this, &StrokeTool::onViewUpdated));
    return true;
}

bool StrokeTool::leavingThisTool()
{
    return BaseTool::leavingThisTool();
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

    mStrokePressures.clear();

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
    mStrokePoints.clear();
    mStrokePressures.clear();
    mStrokeSegment.clear();

    enableCoalescing();

    mEditor->setModified(mEditor->currentLayerIndex(), mEditor->currentFrame());
    mEditor->undoRedo()->record(mUndoSaveState, typeName());
    mScribbleArea->endStroke();
}

StrokeDynamics StrokeTool::createDynamics() const
{
    StrokeDynamics dynamics;

    qreal opacity = (strokeToolProperties().pressureEnabled()) ? (mCurrentPressure * 0.5) : 1.0;
    qreal pressure = (strokeToolProperties().pressureEnabled()) ? mCurrentPressure : 1.0;
    dynamics.canSingleDab = true;
    dynamics.blending = QPainter::CompositionMode_SourceOver;
    dynamics.width = strokeToolProperties().width() * pressure;
    dynamics.pressure = pressure;
    dynamics.opacity = opacity;
    dynamics.dabSpacing = dynamics.width * 0.1;
    dynamics.feather = strokeToolProperties().feather();
    dynamics.color = mEditor->color()->frontColor();
    dynamics.antiAliasingEnabled = strokeToolProperties().AntiAliasingEnabled();

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
    }
    else
    {
        mFirstDraw = false;
    }
}

void StrokeTool::doStroke()
{
    const StrokeDynamics& dynamics = mStrokeDynamics;
    QPointF dabPoint;

    mStroker.begin(mStrokeSegment);
    while (mStroker.nextDab(dynamics, dabPoint)) {
        drawDab(dabPoint, dynamics);
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

void StrokeTool::applyBitmapBuffer(BitmapImage* bitmapImage)
{
    bitmapImage->paste(&mScribbleArea->mTiledBuffer, mStrokeDynamics.blending);
}

void StrokeTool::applyVectorBuffer(VectorImage*)
{
    mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
}

void StrokeTool::setGaussianGradient(QGradient &gradient, QColor color, qreal opacity, qreal offset) const
{
    if (offset < 0) { offset = 0; }
    if (offset > 100) { offset = 100; }

    int r = color.red();
    int g = color.green();
    int b = color.blue();
    qreal a = color.alphaF();

    int mainColorAlpha = qRound(a * 255 * opacity);

    // the more feather (offset), the more softness (opacity)
    int alphaAdded = qRound((mainColorAlpha * offset) / 100);

    gradient.setColorAt(0.0, QColor(r, g, b, mainColorAlpha - alphaAdded));
    gradient.setColorAt(1.0, QColor(r, g, b, 0));
    gradient.setColorAt(1.0 - (offset / 100.0), QColor(r, g, b, mainColorAlpha - alphaAdded));
}

bool StrokeTool::handleQuickSizing(PointerEvent* event)
{
    if (!mQuickSizingEnabled) { return false; }

    if (!mQuickSizingProperties.contains(event->modifiers())) {
        mWidthSizingTool.stopAdjusting();
        mFeatherSizingTool.stopAdjusting();
        return false;
    }

    StrokeToolProperties::Type setting = static_cast<StrokeToolProperties::Type>(mQuickSizingProperties[event->modifiers()]);
    if (event->eventType() == PointerEvent::Press) {
        switch (setting) {
            case StrokeToolProperties::WIDTH_VALUE: {
                mWidthSizingTool.setOffset(strokeToolProperties().width() * 0.5);
                break;
            }
            case StrokeToolProperties::FEATHER_VALUE: {
                const qreal factor = 0.5;
                const qreal cursorRad = strokeToolProperties().width() * factor;

                // Pull feather handle closer to center as feather increases
                const qreal featherWidthFactor = MathUtils::normalize(strokeToolProperties().feather(), FEATHER_MIN, FEATHER_MAX);
                const qreal offset = (cursorRad * featherWidthFactor);
                mFeatherSizingTool.setOffset(offset);
                break;
            }
            default: break;
        }
    }

    switch (setting) {
        case StrokeToolProperties::WIDTH_VALUE: {
            mWidthSizingTool.pointerEvent(event);
            break;
        }
        case StrokeToolProperties::FEATHER_VALUE: {
            mFeatherSizingTool.pointerEvent(event);
            break;
        }
        default: break;
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


QRectF StrokeTool::cursorRect(StrokeToolProperties::Type settingType, const QPointF& point)
{
    const qreal brushWidth = strokeToolProperties().width();
    const qreal brushFeather = strokeToolProperties().feather();

    const QPointF& cursorPos = point;
    const qreal cursorRad = brushWidth * 0.5;
    const QPointF& widthCursorTopLeft = QPointF(cursorPos.x() - cursorRad, cursorPos.y() - cursorRad);

    const QRectF widthCircleRect = QRectF(widthCursorTopLeft, QSizeF(brushWidth, brushWidth));
    if (settingType == StrokeToolProperties::WIDTH_VALUE) {
        return widthCircleRect;
    } else if (settingType == StrokeToolProperties::FEATHER_VALUE) {
        const qreal featherWidthFactor =  MathUtils::normalize(brushFeather, FEATHER_MIN, FEATHER_MAX);
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
    widthOptions.circleRect = cursorRect(StrokeToolProperties::WIDTH_VALUE, mWidthSizingTool.isAdjusting() ? mWidthSizingTool.offsetPoint() : getCurrentPoint());
    widthOptions.showCursor = mCanvasCursorEnabled;
    widthOptions.showCross = true;

    CanvasCursorPainterOptions featherOptions;
    featherOptions.circleRect = cursorRect(StrokeToolProperties::FEATHER_VALUE, mFeatherSizingTool.isAdjusting() ? mFeatherSizingTool.offsetPoint() : getCurrentPoint());
    featherOptions.showCursor = mCanvasCursorEnabled;
    featherOptions.showCross = false;

    if (mFeatherSizingTool.isAdjusting()) {
        widthOptions.circleRect = cursorRect(StrokeToolProperties::WIDTH_VALUE, mFeatherSizingTool.offsetPoint());
    } else if (mWidthSizingTool.isAdjusting()) {
        featherOptions.circleRect = cursorRect(StrokeToolProperties::FEATHER_VALUE, mWidthSizingTool.offsetPoint());
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

    if (strokeToolProperties().featherEnabled()) {
        mFeatherCursorPainter.paint(painter, blitRect);
    }

    mWidthCursorPainter.paint(painter, blitRect);

    painter.restore();
}

void StrokeTool::setFeatherEnabled(bool enabled)
{
    toolProperties().setBaseValue(StrokeToolProperties::FEATHER_ENABLED, enabled);
    emit featherEnabledChanged(enabled);
}

void StrokeTool::setFeather(qreal feather)
{
    toolProperties().setBaseValue(StrokeToolProperties::FEATHER_VALUE, feather);

    qreal newFeather = strokeToolProperties().feather();

    mFeatherSizingTool.setOffset(newFeather);
    emit featherChanged(newFeather);
}

void StrokeTool::setWidth(qreal width)
{
    toolProperties().setBaseValue(StrokeToolProperties::WIDTH_VALUE, width);

    qreal newWidth = strokeToolProperties().width();
    mWidthSizingTool.setOffset(newWidth);

    emit widthChanged(newWidth);
}

void StrokeTool::setPressureEnabled(bool enabled)
{
    toolProperties().setBaseValue(StrokeToolProperties::PRESSURE_ENABLED, enabled);
    emit pressureEnabledChanged(enabled);
}

void StrokeTool::setFillContourEnabled(bool enabled)
{
    toolProperties().setBaseValue(StrokeToolProperties::FILLCONTOUR_ENABLED, enabled);
    emit fillContourEnabledChanged(enabled);
}

void StrokeTool::setAntiAliasingEnabled(bool enabled)
{
    toolProperties().setBaseValue(StrokeToolProperties::ANTI_ALIASING_ENABLED, enabled);
    emit antiAliasingEnabledChanged(enabled);
}

void StrokeTool::setStrokeInvisibleEnabled(bool enabled)
{
    toolProperties().setBaseValue(StrokeToolProperties::INVISIBILITY_ENABLED, enabled);
    emit invisibleStrokeEnabledChanged(enabled);
}
