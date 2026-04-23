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

#include "bitmapimage.h"
#include "vectorimage.h"

#include <QDebug>


SelectionManager::SelectionManager(Editor* editor) : BaseManager(editor, __FUNCTION__)
{
    mActiveBitmapEditor = &mNullBitmapEditor;
}

SelectionManager::~SelectionManager()
{
}

bool SelectionManager::init()
{
    connect(editor(), &Editor::scrubbed, this, &SelectionManager::scrubberChanged);

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

    setActiveEditor(mWorkingLayer->id(), editor()->currentFrame());
}

void SelectionManager::scrubberChanged(int framePos)
{
    setActiveEditor(mWorkingLayer->id(), framePos);
}

void SelectionManager::setActiveEditor(int layerId, int framePos)
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP: {
            auto editor = findActiveEditor(layerId, mWorkingLayer->getLastKeyFrameAtPosition(framePos));
            if (editor) {
                mActiveBitmapEditor = editor;
            } else {
                mActiveBitmapEditor = &mNullBitmapEditor;
            }
            break;
        }
        default:
            break;
    }
}

void SelectionManager::createEditor()
{
    KeyFrame* keyframe = mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame());
    if (keyframe == nullptr) { return; }

    switch (mWorkingLayer->type()) {
        case Layer::BITMAP: {
            auto editor = findActiveEditor(mWorkingLayer->id(), keyframe);
            if (!editor) {
                mBitmapEditors.append(
                            BitmapEditorEntry(mWorkingLayer->id(),
                                              SelectionBitmapEditor(static_cast<BitmapImage*>(keyframe))));
                editor = &mBitmapEditors.back().bitmapEditor;
            }
            mActiveBitmapEditor = editor;
            break;
        }
        case Layer::VECTOR:
            Q_ASSERT_X(false, "SelectionManager::createEditor", "Missing Vector Selection Editor implementation");
            // mVectorSelection = SelectionVectorEditor(static_cast<VectorImage*>(keyframe));
            break;
        default:
            break;
    }
}

void SelectionManager::invalidateEditor()
{
    switch (mWorkingLayer->type()) {
        case Layer::BITMAP: {
            mActiveBitmapEditor->invalidate();

            for (int i = 0; i < mBitmapEditors.count(); i += 1) {
                if (&mBitmapEditors[i].bitmapEditor == mActiveBitmapEditor) {
                    mBitmapEditors.removeAt(i);
                    return;
                }
            }
        }
        default:
            break;
    }
}

SelectionBitmapEditor* SelectionManager::findActiveEditor(int layerId, KeyFrame* keyFrame)
{
    for (auto& entry : mBitmapEditors) {
        if (mWorkingLayer->id() == entry.layerID && entry.bitmapEditor.belongsTo(keyFrame->pos())) {
            return &entry.bitmapEditor;
        }
    }

    return nullptr;
}

void SelectionManager::setSelection(const QRectF& rect)
{
    createEditor();
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP: {
        mActiveBitmapEditor->setSelection(rect.toRect());
        break;
    }
    default:
        break;
    }

    emit selectionChanged();
}

void SelectionManager::resetSelectionTransform()
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->resetTransformation();
    default:
        return;
    }
}

void SelectionManager::setTranslation(const QPointF& translation)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->setTranslation(translation);
    default:
        return;
    }
}

void SelectionManager::setRotation(qreal angle)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->setRotation(angle);
    default:
        return;
    }
}

void SelectionManager::setScale(qreal scaleX, qreal scaleY)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->setScale(scaleX, scaleY);
    default:
        return;
    }
}

void SelectionManager::setSelectionTransform(const QTransform& transform)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->setTransform(transform);
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
        return mActiveBitmapEditor->currentAnchorPoint();
    default:
        return QPointF();
    }
}

SelectionBitmapEditor* SelectionManager::activeBitmapEditor()
{
    if (!mActiveBitmapEditor->isValid()) { return nullptr; }

    return mActiveBitmapEditor;
}

QPointF SelectionManager::getSelectionAnchorPoint() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->getSelectionAnchorPoint();
    default:
        return QPointF();
    }
}

QPolygonF SelectionManager::getSelectionPolygon() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->selectionPolygon();
    default:
        return QPolygonF();
    }
}

DragHandle SelectionManager::resolveHandleMode(const QPointF& point, qreal tolerance) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->resolveHandleMode(point, tolerance);
    default:
        return DragHandle::NONE;
    }
}

bool SelectionManager::somethingSelected() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->somethingSelected();
    default:
        return false;
    }
}

bool SelectionManager::isOutsideSelectionArea(const QPointF &point, qreal tolerance) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->isOutsideSelectionArea(point, tolerance);
    default:
        return false;
    }
}

bool SelectionManager::isSelectionValid() const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->isSelectionValid();
    default:
        return false;
    }
}

void SelectionManager::maintainAspectRatio(bool state)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->maintainAspectRatio(state);
    default:
        return;
    }
}

void SelectionManager::lockMovementToAxis(bool state)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->lockMovementToAxis(state);
    default:
        return;
    }
}

void SelectionManager::translate(QPointF newPos)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->translate(newPos);
    default:
        break;
    }
}

void SelectionManager::rotate(qreal angle, qreal lockedAngle)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->rotate(angle, lockedAngle);
    default:
        break;
    }

}

void SelectionManager::scale(qreal sX, qreal sY)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->scale(sX, sY);
    default:
        break;
    }
}

QRectF SelectionManager::mySelectionRect() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->selectionRect();
        default:
            return QRectF();
    }
}

qreal SelectionManager::myRotation() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->rotation();
        default:
            return 0;
    }
}

qreal SelectionManager::myScaleX() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->scaleX();
        default:
            return 1;
    }
}

qreal SelectionManager::myScaleY() const {
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->scaleY();
        default:
            return 1;
    }
}

QPointF SelectionManager::myTranslation() const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->translation();
        default:
            return QPointF();
    }
}

QTransform SelectionManager::selectionTransform() const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->transform();
        default:
            return QTransform();
    }
}

QPointF SelectionManager::mapToSelection(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->mapToSelection(point);
        default:
            return QPointF();
    }
}

QPointF SelectionManager::mapFromLocalSpace(const QPointF& point) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->mapFromLocalSpace(point);
        default:
            return QPointF();
    }
}

QPolygonF SelectionManager::mapToSelection(const QPolygonF& polygon) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->mapToSelection(polygon);
        default:
            return QPolygonF();
    }
}

QPolygonF SelectionManager::mapFromLocalSpace(const QPolygonF& polygon) const
{
    switch (mWorkingLayer->type())
    {
        case Layer::BITMAP:
            return mActiveBitmapEditor->mapFromLocalSpace(polygon);
        default:
            return QPolygonF();
    }
}

qreal SelectionManager::angleFromPoint(const QPointF& point, const QPointF& anchorPoint) const
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        return mActiveBitmapEditor->angleFromPoint(point, anchorPoint);
    default:
        return -1;
    }
}

void SelectionManager::setTransformAnchor(const QPointF& point)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mActiveBitmapEditor->setTransformAnchor(point);
        break;
    default:
        break;
    }
}

void SelectionManager::setSmoothTransform(bool smoothTransform)
{
    switch (mWorkingLayer->type())
    {
    case Layer::BITMAP:
        mActiveBitmapEditor->setSmoothTransform(smoothTransform);
        break;
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
        mActiveBitmapEditor->calculateSelectionTransformation();
        break;
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
        return mActiveBitmapEditor->flipSelection(flipVertical);
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
        return mActiveBitmapEditor->resetSelectionProperties();
    default:
        return;
    }
    emit selectionChanged();
}

