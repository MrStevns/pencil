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

struct DragState
{
    QPointF startPos;
    qreal dx, dy;

    TransformMode transformMode = TransformMode::NONE;
    DragHandle dragHandle = DragHandle::NONE;

    DragState() = default;
};

class MoveTool : public TransformTool
{
    Q_OBJECT
public:
    explicit MoveTool(QObject* parent);
    QCursor cursor() override;

    QCursor cursorForDragHandle(DragHandle handle) const;
    QCursor cursor(MoveMode mode) const;
    ToolType type() const override;

    ToolProperties& toolProperties() override { return mSettings.toolProperties(); }
    void loadSettings() override;

    void pointerPressEvent(PointerEvent*) override;
    void pointerReleaseEvent(PointerEvent*) override;
    void pointerMoveEvent(PointerEvent*) override;

    bool leavingThisTool() override;
    bool isActive() const override;

    void translateSelection(PointerEvent* event, SelectionBitmapEditor& selectionEditor);
    void rotateSelection(PointerEvent* event, SelectionBitmapEditor& selectionEditor);
    void transformSelection(PointerEvent* event, SelectionBitmapEditor& selectionEditor);

private:

    void resolveCursorState(PointerEvent* event, Layer* layer);
    QCursor cursorForSelectionState(PointerEvent* event, SelectionBitmapEditor& editor);

    void applyTransformation();
    void updateSettings(const SETTING setting);

    void beginInteraction(const QPointF& pos, Qt::KeyboardModifiers keyMod, Layer* layer);
    void beginInteraction(PointerEvent* event, SelectionBitmapEditor& selectionEditor);

    void createVectorSelection(const QPointF& pos, Qt::KeyboardModifiers keyMod, Layer* layer);
    void transformSelection(PointerEvent* event);
    void storeClosestVectorCurve(const QPointF& pos, Layer* layer);

    void setCurveSelected(VectorImage* vectorImage, Qt::KeyboardModifiers keyMod);
    void setAreaSelected(const QPointF& pos, VectorImage* vectorImage, Qt::KeyboardModifiers keyMod);

    Layer* currentPaintableLayer();

    QPointF mCurrentPoint;
    qreal mRotatedAngle = 0.0;
    int mRotationIncrementPref = 0;
    MoveMode mPerspMode;

    DragState mDragState;

    const UndoSaveState* mUndoSaveState = nullptr;

    QCursor mCursorCache;
};

#endif
