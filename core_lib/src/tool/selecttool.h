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

#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "transformtool.h"
#include "perspectivemode.h"
#include "undoredomanager.h"

#include "layer.h"

#include <QRectF>

class Layer;
class SelectionManager;

class SelectTool : public TransformTool
{
    Q_OBJECT

    struct DragState
    {
        QPointF anchorOriginPoint;
        QPointF dragFromPoint;

        DragHandle dragHandle = DragHandle::NONE;
    };

    struct BitmapTool
    {
        QRectF selectionRect;
        bool selectionSet = false;

        DragState dragState;
        const UndoSaveState* undoState = nullptr;
    };

    struct VectorTool
    {
        QPointF anchorOriginPoint;
        QRectF selectionRect;
        bool selectionSet = false;

        DragState dragState;
        const UndoSaveState* undoState = nullptr;
    };

public:
    explicit SelectTool(QObject* parent = nullptr);

    ToolType type() const override { return SELECT; }

    void loadSettings() override;
    QCursor cursor() override;

private: // Bitmap
    void bitmapToolPressEvent(PointerEvent* event, BitmapTool& tool);
    void bitmapToolMoveEvent(PointerEvent* event, BitmapTool& tool);
    void bitmapToolReleaseEvent(PointerEvent* event, BitmapTool& tool) const;

    QRectF bitmapToolDragSelection(const QPointF& currentPoint, const DragState& dragState) const;

private: // Vector
    void vectorToolPressEvent(PointerEvent* event, VectorTool& tool);
    void vectorToolMoveEvent(PointerEvent* event, VectorTool& tool);
    void vectorToolReleaseEvent(PointerEvent* event, VectorTool& tool);

    void vectorToolSetSelection();

protected:
    void pointerPressEvent(PointerEvent*) override;
    void pointerReleaseEvent(PointerEvent*) override;
    void pointerMoveEvent(PointerEvent*) override;

    bool keyPressEvent(QKeyEvent* event) override;
private:

    QCursor createCursorForDragHandle(const DragHandle& dragHandle);

    QPixmap mCursorPixmapCache = QPixmap(24, 24);

    BitmapTool mBitmapTool;
    VectorTool mVectorTool;
};

#endif
