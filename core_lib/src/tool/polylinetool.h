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
#include "stroketool.h"
#include "blitrect.h"

class PolylineTool : public StrokeTool
{
    Q_OBJECT
public:
    explicit PolylineTool(QObject* parent = 0);
    ToolType type() override;
    void loadSettings() override;
    void saveSettings() override;
    QCursor cursor() override;
    void resetToDefault() override;

    void pointerPressEvent(PointerEvent*) override;
    void pointerReleaseEvent(PointerEvent* event) override;
    void pointerMoveEvent(PointerEvent* event) override;
    void pointerDoubleClickEvent(PointerEvent* event) override;

    void pointerPressOnVector(PointerEvent*);
    void pointerPressOnBitmap(PointerEvent*);

    bool keyPressEvent(QKeyEvent* event) override;
    bool keyReleaseEvent(QKeyEvent* event) override;

    void clearToolData() override;

    void setWidth(const qreal width) override;
    void setFeather(const qreal feather) override;
    void setClosedPath(const bool closed) override;

    bool leavingThisTool() override;

    bool isActive() const override;

protected:
    double calculateDeltaTime(quint64 time) override;

private:
    QList<QPointF> mPoints;
    QPointF previousPoint;
    bool mClosedPathOverrideEnabled = false;

    void updateDirtyRect(QList<QPointF> linePoints, BlitRect dirtyRect);
    void drawPolyline(QList<QPointF> points, QPointF endPoint, quint64 timeStamp);
    void removeLastPolylineSegment();
    void cancelPolyline();
    void endPolyline(QList<QPointF> points, quint64 timeStamp);
};

#endif // POLYLINETOOL_H
