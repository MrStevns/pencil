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
#include "smudgetool.h"
#include <QPixmap>
#include <QSettings>

#include "pointerevent.h"
#include "vectorimage.h"
#include "editor.h"
#include "scribblearea.h"

#include "layermanager.h"
#include "viewmanager.h"
#include "selectionmanager.h"
#include "undoredomanager.h"

#include "layerbitmap.h"
#include "layervector.h"
#include "blitrect.h"

SmudgeTool::SmudgeTool(QObject* parent) : StrokeTool(parent)
{
    toolMode = 0; // tool mode
}

ToolType SmudgeTool::type() const
{
    return SMUDGE;
}

void SmudgeTool::loadSettings()
{
    StrokeTool::loadSettings();

    QHash<int, PropertyInfo> info;
    QSettings pencilSettings(PENCIL2D, PENCIL2D);
    mPropertyUsed[StrokeToolProperties::WIDTH_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeToolProperties::FEATHER_VALUE] = { Layer::BITMAP };

    info[StrokeToolProperties::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 24.0 };
    info[StrokeToolProperties::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 48.0 };

    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    if (toolProperties().requireMigration(pencilSettings, ToolProperties::VERSION_1)) {
        toolProperties().setBaseValue(StrokeToolProperties::WIDTH_VALUE, pencilSettings.value("smudgeWidth", 24.0).toReal());
        toolProperties().setBaseValue(StrokeToolProperties::FEATHER_VALUE, pencilSettings.value("smudgeFeather", 48.0).toReal());

        pencilSettings.remove("smudgeWidth");
        pencilSettings.remove("smudgeFeather");
    }

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeToolProperties::WIDTH_VALUE);
    mQuickSizingProperties.insert(Qt::ControlModifier, StrokeToolProperties::FEATHER_VALUE);

    setStablizationLevel(1);
}

bool SmudgeTool::emptyFrameActionEnabled()
{
    // Disabled till we get it working for vector layers...
    return false;
}

QCursor SmudgeTool::cursor()
{
    if (toolMode == 0) { //normal mode
        return QCursor(QPixmap(":icons/general/cursor-smudge.svg"), 4, 18);

    }
    else { // blured mode
        return QCursor(QPixmap(":icons/general/cursor-smudge-liquify.svg"), 4, 18);
    }
}

bool SmudgeTool::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Alt)
    {
        toolMode = 1; // alternative mode
        mScribbleArea->setCursor(cursor()); // update cursor
        return true;
    }
    return StrokeTool::keyPressEvent(event);
}

bool SmudgeTool::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Alt)
    {
        toolMode = 0; // default mode
        mScribbleArea->setCursor(cursor()); // update cursor
        return true;
    }
    return StrokeTool::keyReleaseEvent(event);
}

StrokeDynamics SmudgeTool::createDynamics() const
{
    StrokeDynamics dynamics = StrokeTool::createDynamics();

    dynamics.dabSpacing = 1.0;
    dynamics.canSingleDab = false;
    dynamics.width = mSettings.width();
    dynamics.feather = mSettings.feather();
    dynamics.opacity = 1.0;
    dynamics.color = QColor(255,255,255);

    return dynamics;
}

void SmudgeTool::pointerPressEvent(PointerEvent* event)
{
    StrokeTool::pointerPressEvent(event);

    mMaskImage = createMask();
    mLastDab = QPoint();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr || !layer->isPaintable()) { return; }

    BitmapImage *sourceImage = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(mEditor->currentFrame(), 0);
    if (sourceImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing

    // TODO: Figure out a better way to copy the target image
    mTargetImage = sourceImage->copy();
}

void SmudgeTool::pointerReleaseEvent(PointerEvent *event)
{
    StrokeTool::pointerReleaseEvent(event);

    mLastDab = QPoint();
}

QImage SmudgeTool::createMask() const
{
    StrokeDynamics dynamics = createDynamics();
    qreal brushRadius = 0.5 * dynamics.width;
    QRadialGradient radialGrad(brushRadius, brushRadius, brushRadius);
    setGaussianGradient(radialGrad, dynamics.color, dynamics.opacity, dynamics.feather);

    QImage mask(dynamics.width, dynamics.width, QImage::Format_Alpha8);
    mask.fill(Qt::transparent);

    QPainter maskPainter(&mask);
    maskPainter.setPen(Qt::NoPen);
    maskPainter.setBrush(radialGrad);
    maskPainter.drawEllipse(0,0, dynamics.width, dynamics.width);

    return mask;
}

void SmudgeTool::drawStroke()
{
    StrokeTool::drawStroke();

    // Layer* layer = mEditor->layers()->currentLayer();
    // if (layer == nullptr || !layer->isPaintable()) { return; }

    // BitmapImage *sourceImage = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(mEditor->currentFrame(), 0);
    // if (sourceImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing

    // // TODO: Figure out a better way to copy the target image
    // mTargetImage = sourceImage->copy();

    doStroke();
}

void SmudgeTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    QPoint dabPoint = point.toPoint();
    if (!mLastDab.isNull() && mLastDab != dabPoint) {
        mTargetImage.paste(&mScribbleArea->mTiledBuffer, QPainter::CompositionMode_SourceOver);
        if (toolMode == 0) {
            blurBrush(&mTargetImage,
                      mLastDab,
                      dabPoint,
                      dynamics);
        } else {
            liquifyBrush(&mTargetImage,
                         mLastDab,
                         dabPoint,
                         dynamics);
        }
    }
    mLastDab = dabPoint;
}

void SmudgeTool::blurBrush(BitmapImage *bmiSource_, QPoint prevPoint, QPoint currentPoint, const StrokeDynamics& dynamics)
{
    qreal brushWidth = dynamics.width;

    QRect prevRect(prevPoint.x() - 0.5 * brushWidth, prevPoint.y() - 0.5 * brushWidth, brushWidth, brushWidth);
    QRect currentRect(currentPoint.x() - 0.5 * brushWidth, currentPoint.y() - 0.5 * brushWidth, brushWidth, brushWidth);

    BitmapImage bmiSrcClip = bmiSource_->copy(prevRect);
    bmiSrcClip.image()->setAlphaChannel(mMaskImage);

    mScribbleArea->mTiledBuffer.drawImage(*bmiSrcClip.image(), currentRect, dynamics.blending, dynamics.antiAliasingEnabled);
}

void SmudgeTool::liquifyBrush(BitmapImage *bmiSource_, QPointF srcPoint_, QPointF thePoint_, const StrokeDynamics& dynamics)
{
    qreal brushWidth = dynamics.width;
    QPointF delta = (thePoint_ - srcPoint_); // increment vector
    QRectF trgRect(thePoint_.x() - 0.5 * brushWidth, thePoint_.y() - 0.5 * brushWidth, brushWidth, brushWidth);

    QRadialGradient radialGrad(thePoint_, 0.5 * brushWidth);
    setGaussianGradient(radialGrad, QColor(255, 255, 255, 255), dynamics.opacity, dynamics.feather);

    // Create gradient brush
    BitmapImage bmiTmpClip;
    bmiTmpClip.drawRect(trgRect, Qt::NoPen, radialGrad, QPainter::CompositionMode_Source, dynamics.antiAliasingEnabled);

    // Slide texture/pixels of the source image
    qreal factor, factorGrad;

    for (int yb = bmiTmpClip.top(); yb < bmiTmpClip.bottom(); yb++)
    {
        for (int xb = bmiTmpClip.left(); xb < bmiTmpClip.right(); xb++)
        {
            QColor color;
            color.setRgba(bmiTmpClip.pixel(xb, yb));
            factorGrad = color.alphaF(); // any from r g b a is ok

            int xa = xb - factorGrad * delta.x();
            int ya = yb - factorGrad * delta.y();

            color.setRgba(bmiSource_->pixel(xa, ya));
            factor = color.alphaF();

            if (factor > 0.0)
            {
                color.setRed(color.red() / factor);
                color.setGreen(color.green() / factor);
                color.setBlue(color.blue() / factor);
                color.setAlpha(255); // Premultiplied color

                color.setRed(color.red()*factorGrad);
                color.setGreen(color.green()*factorGrad);
                color.setBlue(color.blue()*factorGrad);
                color.setAlpha(255 * factorGrad); // Premultiplied color

                bmiTmpClip.setPixel(xb, yb, color.rgba());
            }
            else
            {
                bmiTmpClip.setPixel(xb, yb, qRgba(255, 255, 255, 0));
            }
        }
    }
    mScribbleArea->mTiledBuffer.drawImage(*bmiTmpClip.image(), bmiTmpClip.bounds(), dynamics.blending, dynamics.antiAliasingEnabled);
}
