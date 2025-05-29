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

#ifndef BRUSHTOOL_H
#define BRUSHTOOL_H

#include "stroketool.h"
#include <QColor>

class Layer;

class BrushTool : public StrokeTool
{
    Q_OBJECT

public:
    explicit BrushTool(QObject* parent = 0);

    ToolType type() const override;
    ToolCategory category() const override { return STROKETOOL; }

    void loadSettings() override;
    QCursor cursor() override;

    void pointerPressEvent(PointerEvent*) override;

    void drawStroke() override;

    void applyVectorBuffer(VectorImage* image) override;

private:
    void drawDab(const QPointF& point, const StrokeDynamics& dynamics) override;
    void drawPath(const QPainterPath& path, QPen pen, QBrush brush) override;

protected:
    QColor mCurrentPressuredColor;
    qreal mOpacity = 1.0;
};

#endif // BRUSHTOOL_H
