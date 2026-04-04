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

#ifndef MOVETOOL_H
#define MOVETOOL_H

#include "transformtool.h"
#include "perspectivemode.h"
#include "preferencemanager.h"
#include "undoredomanager.h"

#include "selectionbitmapeditor.h"

class Layer;
class VectorImage;

class MoveTool : public TransformTool
{
    struct DragState
    {
        QPointF startPos;
        qreal dx, dy;
        DragHandle dragHandle = DragHandle::NONE;

        DragState() = default;
    };

    struct TransformState
    {
        QPointF currentPoint;
        qreal rotatedAngle = 0.0;
        TransformMode transformMode = TransformMode::NONE;
    };

    struct BitmapTool
    {
        TransformState transformState;
        DragState dragState;

        const UndoSaveState* undoSaveState = nullptr;
    };

    struct VectorTool
    {
        TransformState transformState;
        DragState dragState;

        const UndoSaveState* undoSaveState = nullptr;
    };

    struct PerspectiveOverlayTool
    {
        PerspectiveMode perspectiveMode;
    };

    Q_OBJECT
public:
    explicit MoveTool(QObject* parent);
    QCursor cursor() override;

    QCursor cursorForDragHandle(DragHandle handle) const;
    QCursor cursorForPerspective(PerspectiveMode mode) const;

    ToolType type() const override;

    ToolProperties& toolProperties() override { return mSettings.toolProperties(); }
    void loadSettings() override;

    void pointerPressEvent(PointerEvent*) override;
    void pointerReleaseEvent(PointerEvent*) override;
    void pointerMoveEvent(PointerEvent*) override;

    bool leavingThisTool() override;
    bool isActive() const override;

    void translateSelection(const PointerEvent* event, const DragState& dragState, SelectionTransformEditor& selectionEditor);
    void rotateSelection(const PointerEvent* event, const TransformState& transformState, SelectionTransformEditor& selectionEditor);
    void scaleAroundAnchorPoint(const PointerEvent* evet, const DragState& dragState, const QPolygonF& selectionPolygon, SelectionTransformEditor& selectionEditor);

private: // Bitmap
    void bitmapToolTransformSelection(const PointerEvent* event, const BitmapTool& tool);
    void bitmapToolSetDragState(PointerEvent* event, const SelectionBitmapEditor& selectionEditor, BitmapTool& tool);
    void bitmapToolPressEvent(PointerEvent* event, BitmapTool& tool);
    void bitmapToolMoveEvent(PointerEvent* event, BitmapTool& tool);
    void bitmapToolReleaseEvent(PointerEvent* event, BitmapTool& tool);

private: // Vector
    // void vectorToolSetDragState(PointerEvent* event, const SelectionVectorEditor& selectionEditor, VectorTool& tool);
    void vectorToolPressEvent(PointerEvent* event, VectorTool& tool);
    void vectorToolCreateSelection(const QPointF& pos, Qt::KeyboardModifiers keyMod);
    void vectorToolStoreClosestCurve(const QPointF& pos, Layer* layer);

    void vectorToolSetCurveSelected(VectorImage* vectorImage, Qt::KeyboardModifiers keyMod);
    void vectorToolSetAreaSelected(const QPointF& pos, VectorImage* vectorImage, Qt::KeyboardModifiers keyMod);

private: // Perspective Overlay
    void pressEventPerspectiveTool(PointerEvent* event, PerspectiveOverlayTool& tool);

private:
    void setTransformMode(const PointerEvent* event, const DragHandle& dragHandle, const SelectionTransformEditor& selectionEditor, TransformState& transformState);
    void applyTransformation();
    void updateSettings(const SETTING setting);

    Layer* currentPaintableLayer();

    int mRotationIncrementPref = 0;

    QCursor mCursorCache;
    bool mCursorCacheInvalid = true;

    BitmapTool mBitmapTool;
    VectorTool mVectorTool;
    PerspectiveOverlayTool mPerspectiveTool;
};

#endif
