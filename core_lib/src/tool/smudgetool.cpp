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
    qreal mTempWidth = mSettings.width();
    dynamics.width = mSettings.width() + 0.0 * mSettings.feather();
    dynamics.feather = qMax(0.0, mTempWidth - 0.5 * mSettings.feather()) / dynamics.width;
    dynamics.opacity = 1.0;
    dynamics.color = QColor(255,255,255);

    return dynamics;
}

void SmudgeTool::pointerPressEvent(PointerEvent* event)
{
    StrokeTool::pointerPressEvent(event);

    mMaskImage = createMask();
    mLastDab = getCurrentPoint().toPoint();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr || !layer->isPaintable()) { return; }

    BitmapImage *sourceImage = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(mEditor->currentFrame(), 0);
    if (sourceImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing

    // TODO: Figure out a better way to copy the target image
    mTargetImage = sourceImage->copy();
    mDisplacementBuffer.resize(mTargetImage.width() * mTargetImage.height());
    mDisplacementBuffer.fill(QPointF());
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

    // qreal brushStep = 2.0;
    // qreal distance = QLineF(mLastDab, getCurrentPoint()).length();
    // int steps = qRound(distance / brushStep);

    // StrokeDynamics dynamics = createDynamics();

    // QPointF sourcePoint = mLastDab;
    // for (int i = 0; i < steps; i++)
    // {
    //     mTargetImage.paste(&mScribbleArea->mTiledBuffer);
    //     QPointF targetPoint = mLastDab + (i + 1) * (brushStep) * (getCurrentPoint() - mLastDab) / distance;
    //     liquifyBrush(&mTargetImage,
    //               sourcePoint,
    //               targetPoint,
    //               dynamics);

    //     if (i == (steps - 1))
    //     {
    //         mLastDab = targetPoint.toPoint();
    //     }
    //     sourcePoint = targetPoint;
    // }
}

void SmudgeTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    QPoint dabPoint = point.toPoint();
    if (!mLastDab.isNull() && mLastDab != dabPoint) {
        if (toolMode == 0) {
            mTargetImage.paste(&mScribbleArea->mTiledBuffer, QPainter::CompositionMode_SourceOver);
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
    BitmapImage bmiTmpClip(trgRect.toRect(), Qt::transparent);
    // bmiTmpClip.drawRect(trgRect, Qt::NoPen, radialGrad, QPainter::CompositionMode_Source, dynamics.antiAliasingEnabled);

    BitmapImage maskImage(trgRect.toRect(), Qt::black);
    maskImage.image()->setAlphaChannel(mMaskImage);
    // bmiTmpClip.drawRect(trgRect, Qt::NoPen, radialGrad, QPainter::CompositionMode_Source, dynamics.antiAliasingEnabled);

    // Slide texture/pixels of the source image
    qreal factorGrad;

    for (int yb = bmiTmpClip.top(); yb < bmiTmpClip.bottom(); yb++)
    {
        for (int xb = bmiTmpClip.left(); xb < bmiTmpClip.right(); xb++)
        {
            QColor color;
            color.setRgba(maskImage.pixel(xb, yb));
            factorGrad = static_cast<qreal>(qAlpha(maskImage.constScanLine(xb, yb))) / 255.0; // any from r g b a is ok

            int maskX = xb - trgRect.left();
            int maskY = yb - trgRect.top();

            QPointF &disp = mDisplacementBuffer[maskY * bmiSource_->width() + maskX];
            disp += delta * factorGrad;

            // Sample source image at displaced position
            int srcX = qRound(xb - disp.x());
            int srcY = qRound(yb - disp.y());

            QRgb sourceColor = bmiSource_->pixel(srcX, srcY);

            if (sourceColor == 0) {
                bmiTmpClip.setPixel(xb, yb, qRgba(0,0,0,0));
                continue;
            }

            color.setRgba(sourceColor);

            int sourceA = qAlpha(sourceColor);
            if (sourceA == 0)
            {
                bmiTmpClip.setPixel(xb, yb, 0);
                continue;
            }

            int r = qRed(sourceColor) * factorGrad;
            int g = qGreen(sourceColor) * factorGrad;
            int b = qBlue(sourceColor) * factorGrad;
            int a = sourceA * factorGrad;

            bmiTmpClip.setPixel(xb, yb, qRgba(r, g, b, a));
        }
    }
    // mTestImage = *bmiTmpClip.image();
    mScribbleArea->mTiledBuffer.drawImage(*bmiTmpClip.image(), bmiTmpClip.bounds(), dynamics.blending, dynamics.antiAliasingEnabled);

    // mScribbleArea->update();
}
