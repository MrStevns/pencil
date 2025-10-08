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
#include "selectionmanager.h"
#include "editor.h"
#include "layer.h"

#include "bitmapimage.h"
#include "vectorimage.h"

#include "mathutils.h"

#include <QVector2D>


SelectionManager::SelectionManager(Editor* editor) : BaseManager(editor, __FUNCTION__)
{
}

SelectionManager::~SelectionManager()
{
}

bool SelectionManager::init()
{
    return true;
}

Status SelectionManager::load(Object*)
{
    resetSelectionProperties();
    return Status::OK;
}

Status SelectionManager::save(Object*)
{
    return Status::OK;
}

void SelectionManager::workingLayerChanged(Layer* workingLayer)
{
    mWorkingLayer = workingLayer;
}

void SelectionManager::resetSelectionTransform()
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.resetTransformation();
    default:
        return;
    }
}

void SelectionManager::setTranslation(const QPointF& translation)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setTranslation(translation);
    default:
        return;
    }
}

void SelectionManager::setRotation(qreal angle)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setRotation(angle);
    default:
        return;
    }
}

void SelectionManager::setScale(qreal scaleX, qreal scaleY)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setScale(scaleX, scaleY);
    default:
        return;
    }
}

void SelectionManager::setSelectionTransform(const QTransform& transform)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setTransform(transform);
    default:
        return;
    }
}

bool SelectionManager::isOutsideSelectionArea(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.isOutsideSelectionArea(point);
    default:
        return true;
    }
}

void SelectionManager::deleteSelection()
{
    emit needDeleteSelection();
}

qreal SelectionManager::selectionTolerance() const
{
    return qAbs(mSelectionTolerance * editor()->viewScaleInversed());
}

QPointF SelectionManager::currentTransformAnchor() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.currentAnchorPoint();
    default:
        return QPointF();
    }
}

QPointF SelectionManager::getSelectionAnchorPoint() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.getSelectionAnchorPoint();
    default:
        return QPointF();
    }
}

QPolygonF SelectionManager::getSelectionPolygon() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.mySelectionPolygon();
    default:
        return QPolygonF();
    }
}

MoveMode SelectionManager::resolveMoveModeForPoint(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.resolveMoveModeForAnchorInRange(point, mSelectionTolerance);
    default:
        return MoveMode::NONE;
    }
}

void SelectionManager::setDragOrigin(const QPointF& point)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setDragOrigin(point);
    default:
        return;
    }
}

void SelectionManager::setMoveModeForAnchorInRange(const QPointF& point)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setMoveMode(resolveMoveModeForPoint(point));
    default:
        return;
    }
}
void SelectionManager::adjustSelection(const QPointF& currentPoint, const QPointF& offset, qreal rotationOffset, int rotationIncrement)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.adjustCurrentSelection(currentPoint, offset, rotationOffset, rotationIncrement);
    default:
        return;
    }
}

bool SelectionManager::somethingSelected() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.somethingSelected();
    default:
        return false;
    }
}

void SelectionManager::maintainAspectRatio(bool state)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.maintainAspectRatio(state);
    default:
        return;
    }
}

void SelectionManager::lockMovementToAxis(bool state)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.lockMovementToAxis(state);
    default:
        return;
    }
}

void SelectionManager::setMoveMode(const MoveMode moveMode)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.setMoveMode(moveMode);
    default:
        return;
    }
}

MoveMode SelectionManager::getMoveMode() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.moveMode();
    default:
        return MoveMode::NONE;
    }
}

void SelectionManager::translate(QPointF newPos)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection.translate(newPos);
    default:
        break;
    }
}

void SelectionManager::rotate(qreal angle, qreal lockedAngle)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection.rotate(angle, lockedAngle);
    default:
        break;
    }

}

void SelectionManager::scale(qreal sX, qreal sY)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection.scale(sX, sY);
    default:
        break;
    }
}

QRectF SelectionManager::mySelectionRect() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.mySelectionRect();
        default:
            return QRectF();
    }
}

qreal SelectionManager::myRotation() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.myRotation();
        default:
        return 0;
    }
}

qreal SelectionManager::myScaleX() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.myScaleX();
        default:
        return 0;
    }
}

qreal SelectionManager::myScaleY() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.myScaleY();
        default:
        return 0;
    }
}

QPointF SelectionManager::myTranslation() const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.myTranslation();
        default:
        return QPointF();
    }
}

QTransform SelectionManager::selectionTransform() const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.myTransform();
        default:
            return QTransform();
    }
}

QPointF SelectionManager::mapToSelection(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.mapToSelection(point);
        default:
        return QPointF();
    }
}

QPointF SelectionManager::mapFromLocalSpace(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.mapFromLocalSpace(point);
        default:
        return QPointF();
    }
}

QPolygonF SelectionManager::mapToSelection(const QPolygonF& polygon) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.mapToSelection(polygon);
        default:
        return QPolygonF();
    }
}

QPolygonF SelectionManager::mapFromLocalSpace(const QPolygonF& polygon) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.mapFromLocalSpace(polygon);
        default:
        return QPolygonF();
    }
}

qreal SelectionManager::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.angleFromPoint(point, anchorPoint);
    default:
        return -1;
    }
}

void SelectionManager::setSelection(const QRectF& rect)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection = SelectionBitmapEditor(static_cast<BitmapImage*>(mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame())));
        bitmapSelection.setSelection(rect.toRect());
    default:
        break;
    }

    emit selectionChanged();
}

void SelectionManager::setTransformAnchor(const QPointF& point)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection.setTransformAnchor(point);
    default:
        break;
    }
}

void SelectionManager::calculateSelectionTransformation()
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection.calculateSelectionTransformation();
    default:
        break;
    }
}

/**
 * @brief ScribbleArea::flipSelection
 * flip selection along the X or Y axis
*/
void SelectionManager::flipSelection(bool flipVertical)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.flipSelection(flipVertical);
    default:
        return;
    }
    emit selectionChanged();
}

void SelectionManager::resetSelectionProperties()
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.resetSelectionProperties();
    default:
        return;
    }
    emit selectionChanged();
}

