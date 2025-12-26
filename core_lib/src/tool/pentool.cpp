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

    QSettings pencilSettings(PENCIL2D, PENCIL2D);

    mPropertyUsed[StrokeToolProperties::WIDTH_VALUE] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeToolProperties::PRESSURE_ENABLED] = { Layer::BITMAP, Layer::VECTOR };
    mPropertyUsed[StrokeToolProperties::ANTI_ALIASING_ENABLED] = { Layer::BITMAP };
    mPropertyUsed[StrokeToolProperties::STABILIZATION_VALUE] = { Layer::BITMAP, Layer::VECTOR };

    QHash<int, PropertyInfo> info;

    info[StrokeToolProperties::WIDTH_VALUE] = { WIDTH_MIN, WIDTH_MAX, 12.0 };
    info[StrokeToolProperties::PRESSURE_ENABLED] = true;
    info[StrokeToolProperties::ANTI_ALIASING_ENABLED] = true;
    info[StrokeToolProperties::STABILIZATION_VALUE] = { StabilizationLevel::NONE, StabilizationLevel::STRONG, StabilizationLevel::STRONG };

    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    if (toolProperties().requireMigration(pencilSettings, ToolProperties::VERSION_1)) {
        toolProperties().setBaseValue(StrokeToolProperties::WIDTH_VALUE, pencilSettings.value("penWidth", 12.0).toReal());
        toolProperties().setBaseValue(StrokeToolProperties::PRESSURE_ENABLED, pencilSettings.value("penPressure", true).toBool());
        toolProperties().setBaseValue(StrokeToolProperties::ANTI_ALIASING_ENABLED, pencilSettings.value("penAA", true).toBool());
        toolProperties().setBaseValue(StrokeToolProperties::STABILIZATION_VALUE, pencilSettings.value("penLineStablization", StabilizationLevel::STRONG).toInt());

        pencilSettings.remove("penWidth");
        pencilSettings.remove("penPressure");
        pencilSettings.remove("penAA");
        pencilSettings.remove("penLineStablization");
    }

    mQuickSizingProperties.insert(Qt::ShiftModifier, StrokeToolProperties::WIDTH_VALUE);
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
                           mSettings.AntiAliasingEnabled());
}

void PenTool::drawPath(const QPainterPath& path, QPen pen, QBrush brush)
{
    mScribbleArea->drawPath(path, pen, brush, QPainter::CompositionMode_Source);
}

void PenTool::applyVectorBuffer(VectorImage* vectorImage)
{
    qreal tol = mScribbleArea->getCurveSmoothing() / mEditor->view()->scaling();

    BezierCurve curve(mStrokePoints, mStrokePressures, tol);
    curve.setWidth(mSettings.width());
    curve.setFeather(mSettings.feather());
    curve.setFilled(false);
    curve.setInvisibility(mSettings.invisibilityEnabled());
    curve.setVariableWidth(mSettings.pressureEnabled());
    curve.setColorNumber(mEditor->color()->frontColorNumber());

    vectorImage->addCurve(curve, mEditor->view()->scaling(), false);

    if (vectorImage->isAnyCurveSelected() || mEditor->select()->somethingSelected())
    {
        mEditor->deselectAll();
    }

    vectorImage->setSelected(vectorImage->getLastCurveNumber(), true);

    StrokeTool::applyVectorBuffer(vectorImage);
}
