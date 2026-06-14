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
        if (layer->type() == Layer::BITMAP)
        {
            mLastBrushPoint = getCurrentPoint();
        }
        else if (layer->type() == Layer::VECTOR)
        {
            const int currentFrame = mEditor->currentFrame();
            const float distanceFrom = selectMan->selectionTolerance();
            VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(currentFrame);
            if (vectorImage == nullptr) { return; }
            selectMan->setCurves(vectorImage->getCurvesCloseTo(getCurrentPoint(), distanceFrom));
            selectMan->setVertices(vectorImage->getVerticesCloseTo(getCurrentPoint(), distanceFrom));
;
            if (selectMan->closestCurves().size() > 0 || selectMan->closestCurves().size() > 0)      // the user clicks near a vertex or a curve
            {
                // Since startStroke() isn't called, handle empty frame behaviour here.
                // Commented out for now - leads to segfault on mouse-release event.
//                if(emptyFrameActionEnabled())
//                {
//                    mScribbleArea->handleDrawingOnEmptyFrame();
//                }

                //qDebug() << "closestCurves:" << closestCurves << " | closestVertices" << closestVertices;
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


    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (layer->type() != Layer::BITMAP && layer->type() != Layer::VECTOR)
    {
        return;
    }

    auto selectMan = mEditor->select();
    if (event->inputType() == mCurrentInputType) {
        if (event->buttons() & Qt::LeftButton)   // the user is also pressing the mouse (dragging) {
        {
            if (layer->type() == Layer::BITMAP)
            {
                drawStroke(event);
            }
            else //if (layer->type() == Layer::VECTOR)
            {
                if (event->modifiers() != Qt::ShiftModifier)    // (and the user doesn't press shift)
                {
                    VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame());
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
                VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getLastVectorImageAtFrame(mEditor->currentFrame());
                if (vectorImage == nullptr) { return; }

                selectMan->setVertices(vectorImage->getVerticesCloseTo(getCurrentPoint(), selectMan->selectionTolerance()));
                mScribbleArea->update();
            }
        }
    }

    StrokeTool::pointerMoveEvent(event);
}

void SmudgeTool::pointerReleaseEvent(PointerEvent* event)
{
    mInterpolator.pointerReleaseEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->inputType() != mCurrentInputType) return;

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer == nullptr) { return; }

    if (event->button() == Qt::LeftButton)
    {
        mEditor->backup(typeName());

        if (layer->type() == Layer::BITMAP)
        {
            drawStroke(event);
            mScribbleArea->paintBitmapBuffer();
            mScribbleArea->clearDrawingBuffer();
            endStroke();
        }
        else if (layer->type() == Layer::VECTOR)
        {
            VectorImage *vectorImage = ((LayerVector *)layer)->getLastVectorImageAtFrame(mEditor->currentFrame());
            if (vectorImage == nullptr) { return; }
            vectorImage->applySelectionTransformation();

            auto selectMan = mEditor->select();
            selectMan->resetSelectionTransform();
            for (int k = 0; k < selectMan->vectorSelection.curve.size(); k++)
            {
                int curveNumber = selectMan->vectorSelection.curve.at(k);
                vectorImage->curve(curveNumber).smoothCurve();
            }
            mEditor->setModified(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame());
        }
    }

    StrokeTool::pointerReleaseEvent(event);
}

void SmudgeTool::drawStroke(PointerEvent* event)
{
    StrokeTool::drawStroke();

    Layer* layer = mEditor->layers()->currentLayer();
    if (layer->type() == Layer::BITMAP)
    {
        const float pressure = static_cast<float>(mCurrentPressure);

        double dt = calculateDeltaTime(event->timeStamp());

        if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP) {
            mScribbleArea->strokeTo(getCurrentPoint(), pressure, 0.0f,  0.0f, dt);
        } else {
            // Only mypaint utilizes a strokeTo method currently...
        }
    }
}

QPointF SmudgeTool::offsetFromPressPos()
{
    return getCurrentPoint() - getCurrentPressPoint();
}

