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

// void SelectionManager::resetSelectionTransformProperties()
// {
//     switch (mWorkingLayer->type())
//     {
//     case Layer::BITMAP:
//         bitmapSelection.resetSelectionProperties();
//     case Layer::VECTOR:
//         // vectorSelection
//     default:
//         Q_ASSERT(false);
//     }
//     // mRotatedAngle = 0;
//     // mTranslation = QPointF(0, 0);
//     // mScaleX = 1;
//     // mScaleY = 1;
//     // mAnchorPoint = QPoint();
//     // mSelectionTransform.reset();
// }

void SelectionManager::resetSelectionTransform()
{
    mSelectionTransform.reset();
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

bool SelectionManager::isOutsideSelectionArea(const QPointF& point) const
{
    // return (!mSelectionTransform.map(mSelectionPolygon).containsPoint(point, Qt::WindingFill)) && mMoveMode == MoveMode::NONE;
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
        return bitmapSelection.state().selectionPolygon;
    default:
        return QPolygonF();
    }
}

MoveMode SelectionManager::resolveMoveModeForPoint(const QPointF& point)
{
    QPolygonF selectionPolygon = getSelectionPolygon();
    if (selectionPolygon.count() < 4)
    {
        return MoveMode::NONE;
    }

    QPolygonF projectedPolygon = mapToSelection(selectionPolygon);

    const double calculatedSelectionTol = selectionTolerance();

    MoveMode moveMode = MoveMode::NONE;
    if (QLineF(point, projectedPolygon[0]).length() < calculatedSelectionTol)
    {
        moveMode = MoveMode::TOPLEFT;
    }
    else if (QLineF(point, projectedPolygon[1]).length() < calculatedSelectionTol)
    {
        moveMode = MoveMode::TOPRIGHT;
    }
    else if (QLineF(point, projectedPolygon[2]).length() < calculatedSelectionTol)
    {
        moveMode = MoveMode::BOTTOMRIGHT;
    }
    else if (QLineF(point, projectedPolygon[3]).length() < calculatedSelectionTol)
    {
        moveMode = MoveMode::BOTTOMLEFT;
    }
    else if (projectedPolygon.containsPoint(point, Qt::WindingFill))
    {
        moveMode = MoveMode::MIDDLE;
    }
    else
    {
        moveMode = MoveMode::NONE;
    }

    return moveMode;
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

QPointF SelectionManager::myTranslation() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return bitmapSelection.myTranslation();
        default:
        return QPointF();
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

int SelectionManager::constrainRotationToAngle(const qreal rotatedAngle, const int rotationIncrement) const
{
    return qRound(rotatedAngle / rotationIncrement) * rotationIncrement;
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
    // return qRadiansToDegrees(MathUtils::getDifferenceAngle(mSelectionTransform.map(anchorPoint), point));
}

void SelectionManager::setSelection(const QRectF& rect)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        bitmapSelection.setSelection(static_cast<BitmapImage*>(mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame())), rect.toRect());
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

QPointF SelectionManager::alignPositionToAxis(QPointF currentPoint) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return bitmapSelection.alignedPositionToAxis(currentPoint);
    default:
        return QPointF();
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

