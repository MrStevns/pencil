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
#include "movemode.h"
#include "preferencemanager.h"
#include "undoredomanager.h"

#include "selectionbitmapeditor.h"

class Layer;
class VectorImage;

class MoveTool : public TransformTool
{

    enum InteractionMode
    {
        NONE,
        SELECTION,
        PERSPECTIVE_OVERLAY
    };

    struct DragState
    {
        QPointF startPos;
        qreal dx, dy;

        TransformMode transformMode = TransformMode::NONE;
        DragHandle dragHandle = DragHandle::NONE;

        DragState() = default;
    };

    struct BitmapTool
    {
        QPointF currentPoint;
        qreal rotatedAngle = 0.0;

        DragState dragState;

        const UndoSaveState* undoSaveState = nullptr;
    };

    struct VectorTool
    {
        QPointF mCurrentPoint;
        qreal mRotatedAngle = 0.0;
        int mRotationIncrementPref = 0;

        DragState mDragState;

        const UndoSaveState* mUndoSaveState = nullptr;
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

    void translateSelection(PointerEvent* event, const DragState& dragState, SelectionEditor& selectionEditor);
    void rotateSelection(PointerEvent* event, const DragState& dragState, qreal previousRotation, SelectionEditor& selectionEditor);
    void transformSelection(PointerEvent* event, BitmapTool& tool);

private:

    void setDragStateBitmapTool(PointerEvent* event, BitmapTool& tool, const SelectionBitmapEditor& selectionEditor);
    InteractionMode resolveInteractionMode() const;

    void pressEventPerspectiveTool(PointerEvent* event, PerspectiveOverlayTool& tool);
    void pressEventVectorTool(PointerEvent* event, VectorTool& tool);

    void pressEventBitmapTool(PointerEvent* event, BitmapTool& tool);
    void moveEventBitmapTool(PointerEvent* event, BitmapTool& tool);
    void releaseEventBitmapTool(PointerEvent* event, BitmapTool& tool);

    // void resolveCursorState(PointerEvent* event, Layer* layer);
    // QCursor cursorForSelectionState(PointerEvent* event);

    void applyTransformation();
    void updateSettings(const SETTING setting);

    void beginInteraction(const QPointF& pos, Qt::KeyboardModifiers keyMod, Layer* layer);
    void beginInteraction(PointerEvent* event, SelectionBitmapEditor& selectionEditor);

    void createVectorSelection(const QPointF& pos, Qt::KeyboardModifiers keyMod, Layer* layer);
    void storeClosestVectorCurve(const QPointF& pos, Layer* layer);

    void setCurveSelected(VectorImage* vectorImage, Qt::KeyboardModifiers keyMod);
    void setAreaSelected(const QPointF& pos, VectorImage* vectorImage, Qt::KeyboardModifiers keyMod);

    Layer* currentPaintableLayer();

    int mRotationIncrementPref = 0;

    QCursor mCursorCache;
    bool mCursorCacheInvalid = true;

    BitmapTool mBitmapTool;
    VectorTool mVectorTool;
    PerspectiveOverlayTool mPerspectiveTool;
};

#endif
