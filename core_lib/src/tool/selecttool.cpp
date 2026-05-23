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
#include "selecttool.h"
#include <QSettings>
#include "pointerevent.h"
#include "vectorimage.h"
#include "editor.h"
#include "layervector.h"
#include "scribblearea.h"
#include "layermanager.h"
#include "toolmanager.h"
#include "selectionmanager.h"
#include "undoredomanager.h"

SelectTool::SelectTool(QObject* parent) : TransformTool(parent)
{
}

void SelectTool::loadSettings()
{
    QSettings pencilSettings(PENCIL2D, PENCIL2D);

    QHash<int, PropertyInfo> info;

    mPropertyUsed[TransformToolProperties::SHOWSELECTIONINFO_ENABLED] = { Layer::BITMAP, Layer::VECTOR };

    info[TransformToolProperties::SHOWSELECTIONINFO_ENABLED] = false;
    toolProperties().insertProperties(info);
    toolProperties().loadFrom(typeName(), pencilSettings);

    if (toolProperties().requireMigration(pencilSettings, ToolProperties::VERSION_1)) {
        toolProperties().setBaseValue(TransformToolProperties::SHOWSELECTIONINFO_ENABLED, pencilSettings.value("ShowSelectionInfo", false).toBool());
    }
}

QCursor SelectTool::cursor()
{
    QCursor cursor = Qt::ArrowCursor;
    // Don't update cursor while we're moving the selection
    if (mScribbleArea->isPointerInUse()) { return QCursor(mCursorPixmapCache); }

    Layer* currentLayer = mEditor->layers()->currentLayer();
    if (currentLayer == nullptr) return cursor;
    if (!currentLayer->isPaintable()) { return cursor; }

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            cursor = createCursorForDragHandle(mBitmapTool.dragState.dragHandle);
            break;
        case Layer::VECTOR:
            cursor = createCursorForDragHandle(mVectorTool.dragState.dragHandle);
            break;
        default:
            break;
    }

    return cursor;
}

QCursor SelectTool::createCursorForDragHandle(const DragHandle& dragHandle)
{
    mCursorPixmapCache.fill(QColor(255, 255, 255, 0));
    QPainter cursorPainter(&mCursorPixmapCache);
    cursorPainter.setRenderHint(QPainter::Antialiasing);

    switch(dragHandle)
    {
    case DragHandle::TOP_LEFT:
    case DragHandle::BOTTOM_RIGHT:
    {
        cursorPainter.drawPixmap(QPoint(6,6),QPixmap("://icons/general/cursor-diagonal-left.svg"));
        break;
    }
    case DragHandle::TOP_RIGHT:
    case DragHandle::BOTTOM_LEFT:
    {
        cursorPainter.drawPixmap(QPoint(6,6),QPixmap("://icons/general/cursor-diagonal-right.svg"));
        break;
    }
    case DragHandle::CENTER:
    {
        cursorPainter.drawPixmap(QPoint(6,6),QPixmap("://icons/general/cursor-move.svg"));
        break;
    }
    case DragHandle::NONE:
        cursorPainter.drawPixmap(QPoint(3,3), QPixmap(":icons/general/cross.png"));
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
    return QCursor(mCursorPixmapCache);
}

void SelectTool::pointerPressEvent(PointerEvent* event)
{
    Layer* currentLayer = mEditor->layers()->currentLayer();
    if (currentLayer == nullptr) return;
    if (!currentLayer->isPaintable()) { return; }
    if (event->button() != Qt::LeftButton) { return; }

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            bitmapToolPressEvent(event, mBitmapTool);
            break;
        case Layer::VECTOR:
            vectorToolPressEvent(event, mVectorTool);
            break;
        default:
            break;
    }
}

void SelectTool::pointerMoveEvent(PointerEvent* event)
{
    Layer* currentLayer = mEditor->layers()->currentLayer();
    if (currentLayer == nullptr) { return; }
    if (!currentLayer->isPaintable()) { return; }

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            bitmapToolMoveEvent(event, mBitmapTool);
            break;
        case Layer::VECTOR:
            vectorToolMoveEvent(event, mVectorTool);
            break;
        default:
            break;
    }
}

void SelectTool::pointerReleaseEvent(PointerEvent* event)
{
    Layer* currentLayer = mEditor->layers()->currentLayer();

    if (currentLayer == nullptr) return;
    if (event->button() != Qt::LeftButton) return;

    switch (currentLayer->type())
    {
        case Layer::BITMAP:
            bitmapToolReleaseEvent(event, mBitmapTool);
            break;
        case Layer::VECTOR:
            vectorToolReleaseEvent(event, mVectorTool);
            break;
        default:
            break;
    }
}

bool SelectTool::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Alt:
        if (mEditor->tools()->setTemporaryTool(MOVE, {}, Qt::AltModifier))
        {
            return true;
        }
        break;
    default:
        break;
    }

    // Follow the generic behavior anyway
    return TransformTool::keyPressEvent(event);
}
