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
    QSettings settings(PENCIL2D, PENCIL2D);
    mPropertyUsed[StrokeSettings::WIDTH_VALUE] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::FEATHER_VALUE] = { Layer::BITMAP };

    info[StrokeSettings::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 24.0 };
    info[StrokeSettings::FEATHER_VALUE] = { FEATHER_MIN, FEATHER_MAX, 48.0 };
    info[StrokeSettings::STABILIZATION_VALUE] = StabilizationLevel::SIMPLE;

    mStrokeSettings->load(typeName(), settings, info);

    if (mStrokeSettings->requireMigration(settings, 1)) {
        mStrokeSettings->setBaseValue(StrokeSettings::WIDTH_VALUE, settings.value("smudgeWidth", 24.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::FEATHER_VALUE, settings.value("smudgeFeather", 48.0).toReal());

        settings.remove("smudgeWidth");
        settings.remove("smudgeFeather");
    }

    mInterpolator.setStabilizerLevel(mStrokeSettings->stabilizerLevel());

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeSettings::WIDTH_VALUE);
    mQuickSizingProperties.insert(Qt::ControlModifier, StrokeSettings::FEATHER_VALUE);
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

void SmudgeTool::pointerPressEvent(PointerEvent* event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    Layer* layer = mEditor->layers()->currentLayer();
    auto selectMan = mEditor->select();
    if (layer == nullptr) { return; }

    if (event->button() == Qt::LeftButton)
    {
        startStroke(event->inputType());
        if (layer->type() == Layer::VECTOR)
        {
            const int currentFrame = mEditor->currentFrame();
            const float distanceFrom = selectMan->selectionTolerance();
            VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(currentFrame, 0);
            if (vectorImage == nullptr) { return; }
            selectMan->setCurves(vectorImage->getCurvesCloseTo(getCurrentPoint(), distanceFrom));
            selectMan->setVertices(vectorImage->getVerticesCloseTo(getCurrentPoint(), distanceFrom));
;
            if (selectMan->closestCurves().size() > 0 || selectMan->closestCurves().size() > 0)      // the user clicks near a vertex or a curve
            {
                if (event->modifiers() != Qt::ShiftModifier && !vectorImage->isSelected(selectMan->closestVertices()))
                {
                    mEditor->deselectAll();
                }

                vectorImage->setSelected(selectMan->closestVertices(), true);
                selectMan->vectorSelection.add(selectMan->closestCurves());
                selectMan->vectorSelection.add(selectMan->closestVertices());

                emit mEditor->frameModified(mEditor->currentFrame());
            }
            else
            {
                mEditor->deselectAll();
            }
        }
    }

    StrokeTool::pointerPressEvent(event);
}

void SmudgeTool::pointerMoveEvent(PointerEvent* event)
{
    mInterpolator.pointerMoveEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->inputType() != mCurrentInputType) return;

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (layer->type() != Layer::BITMAP && layer->type() != Layer::VECTOR)
    {
        return;
    }

    auto selectMan = mEditor->select();
    if (event->buttons() & Qt::LeftButton)   // the user is also pressing the mouse (dragging) {
    {
        if (layer->type() == Layer::BITMAP)
        {
            drawStroke();
        }
        else //if (layer->type() == Layer::VECTOR)
        {
            if (event->modifiers() != Qt::ShiftModifier)    // (and the user doesn't press shift)
            {
                VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
                if (vectorImage == nullptr) { return; }
                // transforms the selection

                BlitRect blit;

                // Use the previous dirty bound and extend it with the current dirty bound
                // this ensures that we won't get painting artifacts
                blit.extend(vectorImage->getBoundsOfTransformedCurves().toRect());
                selectMan->setSelectionTransform(QTransform().translate(offsetFromPressPos().x(), offsetFromPressPos().y()));
                vectorImage->setSelectionTransformation(selectMan->selectionTransform());
                blit.extend(vectorImage->getBoundsOfTransformedCurves().toRect());

                // And now tell the widget to update the portion in local coordinates
                mScribbleArea->update(mEditor->view()->mapCanvasToScreen(blit).toRect().adjusted(-1, -1, 1, 1));
            }
        }
    }
    else     // the user is moving the mouse without pressing it
    {
        if (layer->type() == Layer::VECTOR)
        {
            VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame(), 0);
            if (vectorImage == nullptr) { return; }

            selectMan->setVertices(vectorImage->getVerticesCloseTo(getCurrentPoint(), selectMan->selectionTolerance()));
            mScribbleArea->update();
        }
    }

    StrokeTool::pointerMoveEvent(event);
}

void SmudgeTool::pointerReleaseEvent(PointerEvent* event)
{
    StrokeTool::pointerReleaseEvent(event);
}

void SmudgeTool::applyVectorBuffer(VectorImage *vectorImage)
{
    vectorImage->applySelectionTransformation();

    auto selectMan = mEditor->select();
    selectMan->resetSelectionTransform();
    for (int k = 0; k < selectMan->vectorSelection.curve.size(); k++)
    {
        int curveNumber = selectMan->vectorSelection.curve.at(k);
        vectorImage->curve(curveNumber).smoothCurve();
    }
}

StrokeDynamics SmudgeTool::createDynamics() const
{
    StrokeDynamics dynamics;

    dynamics.dabSpacing = 1.0;
    dynamics.canSingleDab = false;
    dynamics.width = mStrokeSettings->width();
    dynamics.feather = qMax(0.0, dynamics.width - 0.5 * mStrokeSettings->feather()) / dynamics.width;
    dynamics.opacity = 1.0;

    return dynamics;
}

void SmudgeTool::drawStroke()
{
    StrokeTool::drawStroke();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr || !layer->isPaintable()) { return; }

    BitmapImage *sourceImage = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(mEditor->currentFrame(), 0);
    if (sourceImage == nullptr) { return; } // Can happen if the first frame is deleted while drawing

    // TODO: Figure out a better way to copy the target image
    mTargetImage = sourceImage->copy();
    QList<QPointF> p = mInterpolator.interpolateStroke();

    mTargetImage.paste(&mScribbleArea->mTiledBuffer);
    doStroke();
}

void SmudgeTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    if (toolMode == 0) {
    mScribbleArea->blurBrush(&mTargetImage,
                             getLastPoint(),
                             point,
                             dynamics.width,
                             dynamics.feather,
                             dynamics.opacity);
    } else {
        mScribbleArea->liquifyBrush(&mTargetImage,
                                    getLastPoint(),
                                    point,
                                    dynamics.width,
                                    dynamics.feather,
                                    dynamics.opacity);
    }
}

QPointF SmudgeTool::offsetFromPressPos()
{
    return getCurrentPoint() - getCurrentPressPoint();
}

