/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang
Copyright (C) 2025-2099 Oliver S. larsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef NODETOOL_H
#define NODETOOL_H

#include "basetool.h"

class VectorImage;

class NodeTool : public BaseTool
{
    Q_OBJECT
public:
    explicit NodeTool(QObject* parent = 0);

    ToolType type() const override;
    ToolCategory category() const override { return BASETOOL; }
    bool isActive() const override { return true; }

    void loadSettings() override;

    QCursor cursor() override;

    void pointerPressEvent(PointerEvent *) override;
    void pointerMoveEvent(PointerEvent *) override;
    void pointerReleaseEvent(PointerEvent *) override;

    void applyVectorBuffer(VectorImage *vectorImage);

    void paint(QPainter &painter, const QRect &blitRect) override;

private:
    QPointF offsetFromPressPos(const QPointF& point) const;

    QPointF mPressPoint;
};

#endif // NODETOOL_H
