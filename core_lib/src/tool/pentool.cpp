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
#include "pentool.h"

#include <QPixmap>
#include <QSettings>

#include "vectorimage.h"
#include "layervector.h"
#include "colormanager.h"
#include "layermanager.h"
#include "viewmanager.h"
#include "undoredomanager.h"
#include "selectionmanager.h"
#include "editor.h"
#include "scribblearea.h"
#include "blitrect.h"
#include "pointerevent.h"


PenTool::PenTool(QObject* parent) : StrokeTool(parent)
{
}

void PenTool::loadSettings()
{
    StrokeTool::loadSettings();

    QSettings settings(PENCIL2D, PENCIL2D);

    mPropertyUsed[StrokeSettings::WIDTH_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::PRESSURE_ENABLED] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeSettings::ANTI_ALIASING_ENABLED] = { Layer::BITMAP };
    mPropertyUsed[StrokeSettings::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };

    QHash<int, PropertyInfo> info;

    info[StrokeSettings::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 12.0 };
    info[StrokeSettings::PRESSURE_ENABLED] = true;
    info[StrokeSettings::ANTI_ALIASING_ENABLED] = true;
    info[StrokeSettings::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG };

    mStrokeSettings->load(typeName(), settings, info);

    if (mStrokeSettings->requireMigration(settings, 1)) {
        mStrokeSettings->setBaseValue(StrokeSettings::WIDTH_VALUE, settings.value("penWidth", 12.0).toReal());
        mStrokeSettings->setBaseValue(StrokeSettings::PRESSURE_ENABLED, settings.value("penPressure", true).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::ANTI_ALIASING_ENABLED, settings.value("penAA", true).toBool());
        mStrokeSettings->setBaseValue(StrokeSettings::STABILIZATION_VALUE, settings.value("penLineStablization", StabilizationLevel::STRONG).toInt());

        settings.remove("penWidth");
        settings.remove("penPressure");
        settings.remove("penAA");
        settings.remove("penLineStablization");
    }

    mInterpolator.setStabilizerLevel(mStrokeSettings->stabilizerLevel());

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeSettings::WIDTH_VALUE);
}

QCursor PenTool::cursor()
{
    if (mEditor->preference()->isOn(SETTING::TOOL_CURSOR))
    {
        return QCursor(QPixmap(":icons/general/cursor-pen.svg"), 5, 14);
    }
    return QCursor(QPixmap(":icons/general/cross.png"), 10, 10);
}

StrokeDynamics PenTool::createDynamics() const
{
    StrokeDynamics dynamics = StrokeTool::createDynamics();
    dynamics.opacity = 1.0;

    return dynamics;
}

void PenTool::drawStroke()
{
    StrokeTool::drawStroke();

    Layer* layer = mEditor->layers()->currentLayer();

    if (layer->type() == Layer::BITMAP)
    {
        doStroke();
    }
    else if (layer->type() == Layer::VECTOR)
    {

        const StrokeDynamics& dynamics = createDynamics();
        QPen pen(dynamics.color,
                 dynamics.width,
                 Qt::SolidLine,
                 Qt::RoundCap,
                 Qt::RoundJoin);

        doPath(mStrokeSegment, Qt::NoBrush, pen);
    }
}

void PenTool::drawDab(const QPointF& point, const StrokeDynamics& dynamics)
{
    mScribbleArea->drawPen(point,
                           dynamics.width,
                           dynamics.color,
                           mStrokeSettings->AntiAliasingEnabled());
}

void PenTool::drawPath(const QPainterPath& path, QPen pen, QBrush brush)
{
    mScribbleArea->drawPath(path, pen, brush, QPainter::CompositionMode_Source);
}

void PenTool::applyVectorBuffer(VectorImage* vectorImage)
{
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(mStrokeSettings->width());
    curve.setFeather(mStrokeSettings->feather());
    curve.setFilled(false);
    curve.setInvisibility(mStrokeSettings->invisibilityEnabled());
    curve.setVariableWidth(mStrokeSettings->pressureEnabled());
    curve.setColorNumber(mEditor->color()->frontColorNumber());

    vectorImage->addCurve(curve, mEditor->view()->scaling(), false);

    if (vectorImage->isAnyCurveSelected() || mEditor->select()->somethingSelected())
    {
        mEditor->deselectAll();
    }

    vectorImage->setSelected(vectorImage->getLastCurveNumber(), true);

    StrokeTool::applyVectorBuffer(vectorImage);
}
