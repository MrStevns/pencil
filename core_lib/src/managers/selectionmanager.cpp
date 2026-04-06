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
    connect(editor, &Editor::frameModified, this, [=] {
        switch (mWorkingLayer->type())
        {
        case Layer::BITMAP:
            mBitmapSelection.invalidateBitmapCache();
            mBitmapSelection.updateTransformedSelectionState();
        default:
            return;
        }
    });
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
        return mBitmapSelection.resetTransformation();
    default:
        return;
    }
}

void SelectionManager::setTranslation(const QPointF& translation)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.setTranslation(translation);
    default:
        return;
    }
}

void SelectionManager::setRotation(qreal angle)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.setRotation(angle);
    default:
        return;
    }
}

void SelectionManager::setScale(qreal scaleX, qreal scaleY)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.setScale(scaleX, scaleY);
    default:
        return;
    }
}

void SelectionManager::setSelectionTransform(const QTransform& transform)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.setTransform(transform);
    default:
        return;
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
        return mBitmapSelection.currentAnchorPoint();
    default:
        return QPointF();
    }
}

SelectionBitmapEditor* SelectionManager::currentSelectionBitmapEditor()
{
    if (!mBitmapSelection.isValid()) { return nullptr; }

    return &mBitmapSelection;
}

SelectionVectorEditor* SelectionManager::currentSelectionVectorEditor()
{
    if (!mVectorSelection.isValid()) { return nullptr; }

    return &mVectorSelection;
}

QPointF SelectionManager::getSelectionAnchorPoint() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.getSelectionAnchorPoint();
    default:
        return QPointF();
    }
}

QPolygonF SelectionManager::getSelectionPolygon() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.selectionPolygon();
    default:
        return QPolygonF();
    }
}

DragHandle SelectionManager::resolveHandleMode(const QPointF& point, qreal tolerance) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.resolveHandleMode(point, tolerance);
    case Layer::VECTOR:
        return mVectorSelection.resolveHandleMode(point, tolerance);
    default:
        return DragHandle::NONE;
    }
}

bool SelectionManager::somethingSelected() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.somethingSelected();
    case Layer::VECTOR:
        return mVectorSelection.somethingSelected();
    default:
        return false;
    }
}

bool SelectionManager::isOutsideSelectionArea(const QPointF &point, qreal tolerance) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.isOutsideSelectionArea(point, tolerance);
    case Layer::VECTOR:
        return mVectorSelection.isOutsideSelectionArea(point, tolerance);
    default:
        return true;
    }
}

bool SelectionManager::isSelectionValid() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.isSelectionValid();
    default:
        return false;
    }
}

void SelectionManager::maintainAspectRatio(bool state)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.maintainAspectRatio(state);
    default:
        return;
    }
}

void SelectionManager::lockMovementToAxis(bool state)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.lockMovementToAxis(state);
    default:
        return;
    }
}

void SelectionManager::translate(QPointF newPos)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection.translate(newPos);
    default:
        break;
    }
}

void SelectionManager::rotate(qreal angle, qreal lockedAngle)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection.rotate(angle, lockedAngle);
    default:
        break;
    }

}

void SelectionManager::scale(qreal sX, qreal sY)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection.scale(sX, sY);
    default:
        break;
    }
}

QRectF SelectionManager::mySelectionRect() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.selectionRect();
        case Layer::VECTOR:
            return mVectorSelection.selectionRect();
        default:
            return QRectF();
    }
}

qreal SelectionManager::myRotation() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.rotation();
        default:
        return 0;
    }
}

qreal SelectionManager::myScaleX() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.scaleX();
        default:
        return 1;
    }
}

qreal SelectionManager::myScaleY() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.scaleY();
        default:
        return 1;
    }
}

QPointF SelectionManager::myTranslation() const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.translation();
        default:
        return QPointF();
    }
}

QTransform SelectionManager::selectionTransform() const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.transform();
        default:
            return QTransform();
    }
}

QPointF SelectionManager::mapToSelection(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.mapToSelection(point);
        default:
        return QPointF();
    }
}

QPointF SelectionManager::mapFromLocalSpace(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.mapFromLocalSpace(point);
        default:
        return QPointF();
    }
}

QPolygonF SelectionManager::mapToSelection(const QPolygonF& polygon) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.mapToSelection(polygon);
        default:
        return QPolygonF();
    }
}

QPolygonF SelectionManager::mapFromLocalSpace(const QPolygonF& polygon) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mBitmapSelection.mapFromLocalSpace(polygon);
        default:
        return QPolygonF();
    }
}

qreal SelectionManager::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mBitmapSelection.angleFromPoint(point, anchorPoint);
    default:
        return -1;
    }
}

void SelectionManager::setSelection(const QRectF& rect)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection = SelectionBitmapEditor(static_cast<BitmapImage*>(mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame())));
        mBitmapSelection.setSelection(rect.toRect());
        break;
    case Layer::VECTOR:
        mVectorSelection = SelectionVectorEditor(static_cast<VectorImage*>(mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame())));
        mVectorSelection.setSelection(rect);
        break;
    default:
        break;
    }

    emit selectionChanged();
}

void SelectionManager::applyTransformation()
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        // mBitmapSelection.applyTransformation();
        break;
    case Layer::VECTOR:
        mVectorSelection.applyTransformation();
        break;
    default:
        break;
    }
}

void SelectionManager::setTransformAnchor(const QPointF& point)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection.setTransformAnchor(point);
    default:
        break;
    }
}

void SelectionManager::setSmoothTransform(bool smoothTransform)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection.setSmoothTransform(smoothTransform);
    default:
        break;
    }

    emit selectionChanged();
}

void SelectionManager::calculateSelectionTransformation()
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mBitmapSelection.calculateSelectionTransformation();
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
        return mBitmapSelection.flipSelection(flipVertical);
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
        return mBitmapSelection.resetSelectionProperties();
    case Layer::VECTOR:
        return mVectorSelection.resetSelectionProperties();
    default:
        return;
    }
    emit selectionChanged();
}

