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

#ifndef POLYLINETOOL_H
#define POLYLINETOOL_H

#include <QPointF>

#include "basetool.h"
#include "radialoffsettool.h"
#include "canvascursorpainter.h"

class PolylineTool : public BaseTool
{
    Q_OBJECT
public:
    explicit PolylineTool(QObject* parent = 0);

    ToolType type() const override;
    ToolCategory category() const override { return BASETOOL; }

    void createSettings(ToolSettings *) override;
    void loadSettings() override;
    QCursor cursor() override;

    void setWidth(qreal width);
    void setAntiAliasingEnabled(bool enabled);
    void setUseBezier(bool useBezier);
    void setClosePath(bool closePath);

    void onPreferenceChanged(SETTING setting) override;

    void paint(QPainter& painter, const QRect& blitRect) override;

    bool handleQuickSizingEvent(PointerEvent* event);

    void pointerPressEvent(PointerEvent*) override;
    void pointerReleaseEvent(PointerEvent*) override;
    void pointerMoveEvent(PointerEvent* event) override;
    void pointerDoubleClickEvent(PointerEvent*) override;

    bool keyPressEvent(QKeyEvent* event) override;
    bool keyReleaseEvent(QKeyEvent* event) override;

    void updateCanvas(const QRect& blitRect);

    void clearToolData() override;

    bool leavingThisTool() override;

    bool isActive() const override;

signals:
    void bezierPathEnabledChanged(bool useBezier);
    void closePathChanged(bool closePath);
    void widthChanged(qreal value);
    void antiAliasingEnabledChanged(bool enabled);

private:
    void updateCanvasCursor(const QPointF& currentPoint);
    void drawPolyline(QList<QPointF> points, QPointF endPoint);
    void removeLastPolylineSegment();
    void cancelPolyline();
    void endPolyline(QList<QPointF> points);

    QHash<Qt::KeyboardModifiers, int> mQuickSizingProperties;

    bool mQuickSizingEnabled = false;
    bool mClosedPathOverrideEnabled = false;

    CanvasCursorPainter mCursorPainter;
    RadialOffsetTool mSizingTool;

    PolylineSettings* mSettings = nullptr;
    QList<QPointF> mPoints;

    QPointF mCurrentPoint;

    const qreal WIDTH_MIN = 1.;
    const qreal WIDTH_MAX = 200.;
};

#endif // POLYLINETOOL_H
