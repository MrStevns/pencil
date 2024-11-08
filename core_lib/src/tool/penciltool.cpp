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
#include "penciltool.h"

#include <QSettings>
#include <QPixmap>
#include "pointerevent.h"

#include "layermanager.h"
#include "colormanager.h"
#include "viewmanager.h"
#include "preferencemanager.h"
#include "selectionmanager.h"
#include "undoredomanager.h"

#include "editor.h"
#include "scribblearea.h"
#include "layervector.h"
#include "vectorimage.h"


PencilTool::PencilTool(QObject* parent) : StrokeTool(parent)
{
}

void PencilTool::loadSettings()
{
    StrokeTool::loadSettings();

    mPropertyEnabled[WIDTH] = true;
    mPropertyEnabled[PRESSURE] = true;
    mPropertyEnabled[VECTORMERGE] = false;
    mPropertyEnabled[STABILIZATION] = true;
    mPropertyEnabled[FILLCONTOUR] = true;

    QSettings settings(PENCIL2D, PENCIL2D);
    properties.width = settings.value("pencilWidth", 4).toDouble();
    properties.feather = 50;
    properties.pressure = settings.value("pencilPressure", true).toBool();
    properties.stabilizerLevel = settings.value("pencilLineStabilization", StabilizationLevel::STRONG).toInt();
    properties.useFillContour = false;

    mQuickSizingProperties.insert(Qt::ShiftModifier, WIDTH);
}

void PencilTool::resetToDefault()
{
    setWidth(4.0);
    setFeather(50);
    setUseFeather(false);
    setStabilizerLevel(StabilizationLevel::STRONG);
}

void PencilTool::setWidth(const qreal width)
{
    // Set current property
    properties.width = width;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("pencilWidth", width);
    settings.sync();
}

void PencilTool::setFeather(const qreal feather)
{
    properties.feather = feather;
}

void PencilTool::setUseFeather(const bool usingFeather)
{
    // Set current property
    properties.useFeather = usingFeather;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("brushUseFeather", usingFeather);
    settings.sync();
}

void PencilTool::setInvisibility(const bool)
{
    // force value
    properties.invisibility = 1;
}

void PencilTool::setPressure(const bool pressure)
{
    // Set current property
    properties.pressure = pressure;

    // Update settings
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("pencilPressure", pressure);
    settings.sync();
}

void PencilTool::setPreserveAlpha(const bool preserveAlpha)
{
    // force value
    Q_UNUSED( preserveAlpha )
    properties.preserveAlpha = 0;
}

void PencilTool::setStabilizerLevel(const int level)
{
    properties.stabilizerLevel = level;

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("pencilLineStabilization", level);
    settings.sync();
}

void PencilTool::setUseFillContour(const bool useFillContour)
{
    properties.useFillContour = useFillContour;

    QSettings settings(PENCIL2D, PENCIL2D);
    settings.setValue("FillContour", useFillContour);
    settings.sync();
}

QCursor PencilTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        return QCursor(QPixmap(":icons/general/cursor-pencil.svg"), 4, 14);
    }
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

void PencilTool::pointerPressEvent(PointerEvent *event)
{
    mInterpolator.pointerPressEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    mMouseDownPoint = getCurrentPoint();
    mLastBrushPoint = getCurrentPoint();

    startStroke(event->inputType());

    // note: why are we doing this on device press event?
    if ( !mEditor->preference()->isOn(SETTING::INVISIBLE_LINES) )
    {
        mScribbleArea->toggleThinLines();
    }

    StrokeTool::pointerPressEvent(event);
}

void PencilTool::pointerMoveEvent(PointerEvent* event)
{
    mInterpolator.pointerMoveEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->buttons() & Qt::LeftButton && event->inputType() == mCurrentInputType)
    {
        mCurrentPressure = mInterpolator.getPressure();
        drawStroke(event);
        if (properties.stabilizerLevel != mInterpolator.getStabilizerLevel())
        {
            mInterpolator.setStabilizerLevel(properties.stabilizerLevel);
        }
    }
    StrokeTool::pointerMoveEvent(event);
}

void PencilTool::pointerReleaseEvent(PointerEvent *event)
{
    mInterpolator.pointerReleaseEvent(event);
    if (handleQuickSizing(event)) {
        return;
    }

    if (event->inputType() != mCurrentInputType) return;
    endStroke();

    StrokeTool::pointerReleaseEvent(event);
}

void PencilTool::drawStroke(PointerEvent* event)
{
    StrokeTool::drawStroke(event);
    QList<QPointF> p = mInterpolator.interpolateStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::VECTOR)
    {
        mCurrentWidth = 0; // FIXME: WTF?
        QPen pen(mEditor->color()->frontColor(),
                 1,
                 Qt::DotLine,
                 Qt::RoundCap,
                 Qt::RoundJoin);

        if (p.size() == 4)
        {
            QPainterPath path(p.first());
            path.quadTo(p.first(),
                         p.last());
            mScribbleArea->drawPath(path, pen, Qt::NoBrush, QPainter::CompositionMode_Source);
        }
    }
}
