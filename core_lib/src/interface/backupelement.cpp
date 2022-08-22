/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <QCoreApplication>
#include "QProgressDialog"
#include <QDebug>

#include "layermanager.h"
#include "backupmanager.h"
#include "viewmanager.h"
#include "selectionmanager.h"
#include "canvasmanager.h"

#include "layersound.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "layercamera.h"

#include "editor.h"
#include "backupelement.h"

#include "vectorimage.h"
#include "bitmapimage.h"
#include "soundclip.h"
#include "camera.h"

BackupElement::BackupElement(Editor* editor, QUndoCommand* parent) : QUndoCommand(parent)
{
    qDebug() << "backupElement created";
    mEditor = editor;
}

BackupElement::~BackupElement()
{
}

AddBitmapElement::AddBitmapElement(const BitmapImage* backupBitmap,
                                   const int& backupLayerId,
                                   const DrawOnEmptyFrameAction& frameAction,
                                   QString description,
                                   Editor *editor,
                                   QUndoCommand *parent) : BackupElement(editor, parent)
{

    oldBitmap = backupBitmap->clone();

    oldFrameIndex = oldBitmap->pos();
    newLayerIndex = editor->currentLayerIndex();
    oldLayerId = backupLayerId;

    Layer* layer = editor->layers()->currentLayer();
    newLayerId = layer->id();

    newFrameIndex = editor->currentFrame();
    newFrameIndex = BackupManager::getActiveFrameIndex(layer, newFrameIndex, frameAction);

    newBitmap = static_cast<LayerBitmap*>(layer)->
            getBitmapImageAtFrame(newFrameIndex)->clone();

    auto selectMan = editor->select();
    if (selectMan->somethingSelected()) {
        BitmapImage selectionBitmap = newBitmap->transformed(selectMan->mySelectionRect().toRect(),
                                                              selectMan->selectionTransform(),
                                                              false);

        newBitmap->clear(selectMan->mySelectionRect().toRect());
        newBitmap->paste(&selectionBitmap, QPainter::CompositionMode_SourceOver);
    }

    setText(description);
}

void AddBitmapElement::undo()
{
    Layer* layer = editor()->layers()->findLayerById(oldLayerId);

    const TransformElement* childElem = static_cast<const TransformElement*>(this->child(0));
    if (childElem)
    {
        undoTransform(childElem);
    }
    else
    {
        static_cast<LayerBitmap*>(layer)->replaceLastBitmapAtFrame(oldBitmap);
    }

    editor()->scrubTo(oldLayerId, oldFrameIndex);
}

void AddBitmapElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    const TransformElement* childElem = static_cast<const TransformElement*>(this->child(0));
    if (childElem)
    {
        redoTransform(childElem);
    }
    else
    {
        Layer* layer = editor()->layers()->findLayerById(newLayerId);
        static_cast<LayerBitmap*>(layer)->replaceLastBitmapAtFrame(newBitmap);
    }

    editor()->scrubTo(newLayerId, newFrameIndex);
}

void AddBitmapElement::undoTransform(const TransformElement* childElem)
{

    BitmapImage* oldBitmapClone = oldBitmap->clone();

    // make the cloned bitmap the new canvas image.
    Layer* layer = editor()->layers()->findLayerById(oldLayerId);
    static_cast<LayerBitmap*>(layer)->replaceLastBitmapAtFrame(oldBitmapClone);

    // set selections so the transform will be correct
    auto selectMan = editor()->select();

    selectMan->setSelectionTransform(childElem->oldTransform);
    selectMan->setSelectionRect(childElem->oldSelectionRect);
    selectMan->setTempTransformedSelectionRect(childElem->oldSelectionRectTemp);
    selectMan->setTransformedSelectionRect(childElem->oldTransformedSelectionRect);
    selectMan->setRotation(childElem->oldRotationAngle);
    selectMan->setSomethingSelected(childElem->oldIsSelected);

    editor()->canvas()->paintTransformedSelection(layer, oldBitmapClone, childElem->oldTransform, childElem->oldSelectionRect);
}

void AddBitmapElement::redoTransform(const TransformElement* childElem)
{
    Layer* layer = editor()->layers()->findLayerById(newLayerId);

    BitmapImage* newBitmapClone = newBitmap->clone();

    static_cast<LayerBitmap*>(layer)->replaceLastBitmapAtFrame(newBitmapClone);

    auto selectMan = editor()->select();
    selectMan->setSelectionTransform(childElem->newTransform);
    selectMan->setSelectionRect(childElem->newSelectionRect);
    selectMan->setTempTransformedSelectionRect(childElem->newSelectionRectTemp);
    selectMan->setTransformedSelectionRect(childElem->newTransformedSelectionRect);
    selectMan->setRotation(childElem->newRotationAngle);
    selectMan->setSomethingSelected(childElem->newIsSelected);

    editor()->canvas()->paintTransformedSelection(layer, newBitmapClone, childElem->oldTransform, childElem->oldSelectionRect);
}

AddVectorElement::AddVectorElement(const VectorImage* backupVector,
                                   const int& backupLayerId,
                                   const DrawOnEmptyFrameAction& backupFrameAction,
                                   QString description,
                                   Editor* editor,
                                   QUndoCommand* parent) : BackupElement(editor, parent)
{

    oldVector = backupVector->clone();
    oldFrameIndex = oldVector->pos();

    newLayerIndex = editor->layers()->currentLayerIndex();
    newFrameIndex = editor->currentFrame();

    oldLayerId = backupLayerId;
    Layer* layer = editor->layers()->currentLayer();
    newLayerId = layer->id();

    newFrameIndex = BackupManager::getActiveFrameIndex(layer, newFrameIndex, backupFrameAction);
    newVector = static_cast<LayerVector*>(layer)->
            getVectorImageAtFrame(newFrameIndex)->clone();

    setText(description);
}

void AddVectorElement::undo()
{
    qDebug() << "BackupVectorElement: undo";

    Layer* layer = editor()->layers()->findLayerById(oldLayerId);

    *static_cast<LayerVector*>(layer)->
            getVectorImageAtFrame(oldFrameIndex) = *oldVector;

    editor()->scrubTo(oldLayerId, oldFrameIndex);
}

void AddVectorElement::redo()
{
    qDebug() << "BackupVectorElement: redo";

    if (isFirstRedo) { isFirstRedo = false; return; }

    Layer* layer = editor()->layers()->findLayerById(newLayerId);

    *static_cast<LayerVector*>(layer)->
            getVectorImageAtFrame(newFrameIndex) = *newVector;

    editor()->scrubTo(newLayerId, newFrameIndex);
}

AddKeyFrameElement::AddKeyFrameElement(const int backupFrameIndex,
                                       const int backupLayerId,
                                       const DrawOnEmptyFrameAction& backupFrameAction,
                                       const int backupKeySpacing,
                                       const bool backupKeyExisted,
                                       QString description,
                                       Editor *editor,
                                       QUndoCommand *parent) : BackupElement(editor, parent)
{

    Layer* layer = editor->layers()->currentLayer();
    newLayerIndex = editor->currentLayerIndex();
    newFrameIndex = editor->currentFrame();

    oldFrameIndex = backupFrameIndex;

    oldLayerId = backupLayerId;
    newLayerId = layer->id();

    oldKeyExisted = backupKeyExisted;
    oldKeySpacing = backupKeySpacing;

    emptyFrameSettingVal = editor->preference()->
            getInt(SETTING::DRAW_ON_EMPTY_FRAME_ACTION);
    newFrameIndex = editor->currentFrame();
    newFrameIndex = BackupManager::getActiveFrameIndex(layer, newFrameIndex, backupFrameAction);

    newKey = layer->getLastKeyFrameAtPosition(oldFrameIndex)->clone();
    oldKeyFrames.insert(std::make_pair(oldFrameIndex, newKey));

    bool isSequence = oldKeySpacing > 1;

    if (description.isEmpty() && !isSequence)
    {
        switch (layer->type())
        {
        case Layer::BITMAP: description = QObject::tr("Bitmap: New key"); break;
        case Layer::VECTOR: description = QObject::tr("Vector: New Key"); break;
        case Layer::SOUND: description = QObject::tr("Sound: New Key"); break;
        case Layer::CAMERA: description = QObject::tr("Camera: New Key"); break;
        default: break;
        }
    }
    setText(description);
}

void AddKeyFrameElement::undoSequence()
{
    qDebug() << "oldKeyFrames: " << oldKeyFrames;
    for (auto map : oldKeyFrames)
    {
        qDebug() << "did A key exist before:" << oldKeyExisted;
        if (!oldKeyExisted) {
            editor()->removeKeyAtLayerId(oldLayerId, map.first);
        }
    }
}

void AddKeyFrameElement::undo()
{
    qDebug() << "key remove triggered";
    bool isSequence = oldKeySpacing > 1;
    if (isSequence)
    {
        undoSequence();
    }
    else
    {
        editor()->removeKeyAtLayerId(oldLayerId, oldFrameIndex);
    }
    editor()->updateCurrentFrame();
}

void AddKeyFrameElement::redoSequence()
{
    qDebug() << "nnnew:" << newKeyFrames;
    for (auto map : newKeyFrames)
    {
        newFrameIndex = map.first;
        newKey = map.second;
        editor()->backups()->restoreKey(this);
    }
}

void AddKeyFrameElement::redo()
{
    qDebug() << "undo: new backup frame " << newFrameIndex;
    qDebug() << "undo: newLayer" << newLayerIndex;

    if (isFirstRedo) { isFirstRedo = false; return; }
    bool isSequence = oldKeySpacing > 1;

    if (newFrameIndex > 0)
    {
        if (isSequence)
        {
            redoSequence();
        }
        else
        {
            qDebug() << "restore Addkey triggered";
            editor()->backups()->restoreKey(this);
        }
    }
    editor()->updateCurrentFrame();

}

bool AddKeyFrameElement::mergeWith(const QUndoCommand *other)
{
    qDebug() << "MERGE CHECK!";

    qDebug() << "state of frames: old" << oldKeyFrames;
    qDebug() << "state of frames:: new" << newKeyFrames;
    qDebug() << newKeyFrames;

    bool isSequence = oldKeySpacing > 1;

    if (newKeyFrames.empty())
    {
        newKeyFrames.insert(std::make_pair(oldFrameIndex, newKey));
    }

    const AddKeyFrameElement* element = static_cast<const AddKeyFrameElement*>(other);

    if (!isSequence || element->oldKeySpacing < 2)
    {
        return false;
    }
    qDebug() << "MERGING!";

    oldFrameIndex = element->oldFrameIndex;
    newFrameIndex = element->newFrameIndex;
    newKey = element->newKey;
    oldKeyExisted = element->oldKeyExisted;

    std::map<int, KeyFrame*>frames = static_cast<const AddKeyFrameElement*>(other)->oldKeyFrames;
    for (auto map : frames)
    {
        oldKeyFrames.insert(std::make_pair(map.first, map.second));
    }

    newKeyFrames.insert(std::make_pair(oldFrameIndex, newKey));
    return true;
}


RemoveKeyFrameElement::RemoveKeyFrameElement(const KeyFrame* backupKey,
                                             const int& backupLayerId,
                                             Editor *editor,
                                             QUndoCommand *parent) : BackupElement(editor, parent)
{
    oldFrameIndex = backupKey->pos();
    oldLayerId = backupLayerId;
    oldKey = backupKey->clone();

    Layer* layer = editor->layers()->findLayerById(oldLayerId);

    switch(layer->type())
    {
        case Layer::BITMAP:
        {
            oldBitmap = static_cast<BitmapImage*>(oldKey);
            setText(QObject::tr("Remove Bitmap Key"));
            break;
        }
        case Layer::VECTOR:
        {
            oldVector = static_cast<VectorImage*>(oldKey);
            setText(QObject::tr("Remove Vector Key"));
            break;
        }
        case Layer::SOUND:
        {
            oldClip = static_cast<SoundClip*>(oldKey);
            setText(QObject::tr("Remove Sound Key"));
            break;
        }
        case Layer::CAMERA:
        {
            oldCamera = static_cast<Camera*>(oldKey);
            setText(QObject::tr("Remove Camera key"));
            break;
        }
        default:
            break;
    }
}

void RemoveKeyFrameElement::undo()
{
    qDebug() << "undo: old frame index" << oldFrameIndex;
    qDebug() << "restore key";
    editor()->backups()->restoreKey(this);
}

void RemoveKeyFrameElement::redo()
{
    qDebug() << "redo: old backup frame: " << oldFrameIndex;

    if (isFirstRedo) { isFirstRedo = false; return; }


    qDebug() << "RemoveKeyFrame triggered";
    editor()->removeKeyAtLayerId(oldLayerId, oldFrameIndex);
    editor()->scrubTo(oldLayerId, oldFrameIndex);
}

SelectionElement::SelectionElement(const int backupLayerId,
                                   const int backupFrameIndex,
                                   const VectorSelection& backupVectorSelection,
                                   const SelectionType& backupSelectionType,
                                   const QRectF& backupSelectionRect,
                                   const qreal& backupRotationAngle,
                                   const bool& backupIsSelected,
                                   Editor* editor,
                                   QUndoCommand* parent) : BackupElement(editor, parent),
    layerId(backupLayerId),
    frameIndex(backupFrameIndex)
{
    oldSelectionRect = backupSelectionRect;
    oldRotationAngle = backupRotationAngle;
    oldIsSelected = backupIsSelected;

    oldVectorSelection = backupVectorSelection;

    newSelectionRect = editor->select()->myTransformedSelectionRect();
    newRotationAngle = editor->select()->myRotation();
    newIsSelected = editor->select()->somethingSelected();

    newVectorSelection = editor->select()->vectorSelection;

    selectionType = backupSelectionType;

    if (selectionType == SelectionType::SELECTION) {
        setText(QObject::tr("Select"));
    } else {
        setText(QObject::tr("Deselect"));
    }

}

void SelectionElement::undo()
{
    editor()->scrubTo(layerId, frameIndex);
    if (selectionType == SelectionType::SELECTION) {
        undoSelection();
    } else {
        undoDeselection();
    }
}

void SelectionElement::undoSelection()
{
    auto selectMan = editor()->select();

    selectMan->setSelection(oldSelectionRect);
    selectMan->setRotation(oldRotationAngle);
    selectMan->setSomethingSelected(oldIsSelected);

    Layer* layer = editor()->layers()->findLayerById(layerId);

    editor()->deselectAll();
    if (layer->type() == Layer::VECTOR) {
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getVectorImageAtFrame(frameIndex);
        vectorImage->setSelected(oldVectorSelection.curve, oldVectorSelection.vertex, true);
        selectMan->setSelection(vectorImage->getSelectionRect());
        selectMan->vectorSelection = oldVectorSelection;
    }

    KeyFrame* cKeyFrame = layer->getLastKeyFrameAtPosition(editor()->currentFrame());
    editor()->canvas()->applyTransformedSelection(layer,
                                                  cKeyFrame,
                                                  selectMan->selectionTransform(),
                                                  oldSelectionRect);
}

void SelectionElement::undoDeselection()
{
    auto selectMan = editor()->select();
    selectMan->resetSelectionTransform();
    selectMan->setSelection(oldSelectionRect);
    selectMan->setRotation(oldRotationAngle);
    selectMan->setSomethingSelected(oldIsSelected);

    Layer* layer = editor()->layers()->findLayerById(layerId);
    if (layer->type() == Layer::VECTOR) {
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getVectorImageAtFrame(frameIndex);
        vectorImage->setSelected(oldVectorSelection.curve, oldVectorSelection.vertex, true);
        selectMan->vectorSelection = oldVectorSelection;
    }
}

void SelectionElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    editor()->scrubTo(layerId, frameIndex);
    if (selectionType == SelectionType::SELECTION) {
        redoSelection();
    } else {
        redoDeselection();
    }

}

void SelectionElement::redoSelection()
{
    auto selectMan = editor()->select();

    selectMan->setSelection(newSelectionRect);
    selectMan->setRotation(newRotationAngle);
    selectMan->setSomethingSelected(newIsSelected);
    selectMan->calculateSelectionTransformation();

    Layer* layer = editor()->layers()->findLayerById(layerId);
    if (layer->type() == Layer::VECTOR) {
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getVectorImageAtFrame(frameIndex);
        vectorImage->setSelected(newVectorSelection.curve, newVectorSelection.vertex, true);
        selectMan->setSelection(vectorImage->getSelectionRect());
        selectMan->vectorSelection = newVectorSelection;
    }
}

void SelectionElement::redoDeselection()
{
    auto selectMan = editor()->select();

    Layer* layer = editor()->layers()->findLayerById(layerId);
    KeyFrame* cKeyFrame = layer->getLastKeyFrameAtPosition(editor()->currentFrame());
    editor()->canvas()->applyTransformedSelection(layer,
                                                  cKeyFrame,
                                                  selectMan->selectionTransform(),
                                                  selectMan->mySelectionRect());

    if (layer->type() == Layer::VECTOR) {
        VectorImage* vectorImage = static_cast<LayerVector*>(layer)->getVectorImageAtFrame(frameIndex);
        vectorImage->setSelected(newVectorSelection.curve, newVectorSelection.vertex, true);
        selectMan->vectorSelection = newVectorSelection;
    }

    editor()->deselectAll();
}

bool SelectionElement::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id())
    {
        return false;
    }

    auto otherSelectionElement = static_cast<const SelectionElement*>(other);
    SelectionType otherType = otherSelectionElement->selectionType;
    if (selectionType == SelectionType::SELECTION && otherType == selectionType) {
        newSelectionRect = otherSelectionElement->newSelectionRect;
        newIsSelected = otherSelectionElement->newIsSelected;
        newRotationAngle = otherSelectionElement->newRotationAngle;
        newVectorSelection = otherSelectionElement->newVectorSelection;

        auto selectMan = editor()->select();
        selectMan->setSelectionRect(newSelectionRect);
        selectMan->setRotation(newRotationAngle);
        selectMan->setSomethingSelected(newIsSelected);
        selectMan->vectorSelection = newVectorSelection;

        return true;
    } else {
        return false;
    }
}

TransformElement::TransformElement(const KeyFrame* backupKeyFrame,
                                   const int backupLayerId,
                                   const DrawOnEmptyFrameAction& backupFrameAction,
                                   const QRectF& backupSelectionRect,
                                   const QRectF& backupTempSelectionRect,
                                   const QRectF& backupTransformedSelectionRect,
                                   const qreal backupRotationAngle,
                                   const qreal backupScaleX,
                                   const qreal backupScaleY,
                                   const bool backupIsSelected,
                                   const QTransform& backupTransform,
                                   const QString& description,
                                   Editor *editor,
                                   QUndoCommand *parent) : BackupElement(editor, parent)
{


    oldLayerId = backupLayerId;
    oldFrameIndex = backupKeyFrame->pos();
    oldSelectionRect = backupSelectionRect;
    oldSelectionRectTemp = backupTempSelectionRect;
    oldTransformedSelectionRect = backupTransformedSelectionRect;
    oldRotationAngle = backupRotationAngle;
    oldIsSelected = backupIsSelected;

    oldTransform = backupTransform;
    oldScaleX = backupScaleX;
    oldScaleY = backupScaleY;

    Layer* newLayer = editor->layers()->currentLayer();
    newLayerId = newLayer->id();
    newFrameIndex = editor->currentFrame();

    auto selectMan = editor->select();
    newSelectionRect = selectMan->mySelectionRect();
    newSelectionRectTemp = selectMan->myTempTransformedSelectionRect();
    newTransformedSelectionRect = selectMan->myTransformedSelectionRect();
    newRotationAngle = selectMan->myRotation();
    newIsSelected = selectMan->somethingSelected();
    newTransform = selectMan->selectionTransform();
    newScaleX = selectMan->myScaleX();
    newScaleY = selectMan->myScaleY();

    Layer* layer = editor->layers()->findLayerById(backupLayerId);

    newFrameIndex = BackupManager::getActiveFrameIndex(layer, newFrameIndex, backupFrameAction);
    KeyFrame* oldKeyFrame = backupKeyFrame->clone();

    switch(layer->type())
    {
        case Layer::BITMAP:
        {
            oldBitmap = static_cast<BitmapImage*>(oldKeyFrame)->clone();
            newBitmap = static_cast<LayerBitmap*>(layer)->getBitmapImageAtFrame(newFrameIndex)->clone();
            break;
        }
        case Layer::VECTOR:
        {
            oldVector = static_cast<VectorImage*>(oldKeyFrame)->clone();
            newVector = static_cast<LayerVector*>(layer)->
                    getVectorImageAtFrame(newFrameIndex)->clone();
            break;
        }
        default:
            break;
    }

    setText(description);
}

void TransformElement::undo()
{
    apply(oldBitmap,
          oldVector,
          oldSelectionRect,
          oldSelectionRectTemp,
          oldTransformedSelectionRect,
          oldRotationAngle,
          oldScaleX,
          oldScaleY,
          oldIsSelected,
          oldTransform,
          oldLayerId);
}

void TransformElement::redo()
{
    if (isFirstRedo) {
        isFirstRedo = false; return;
    }

    apply(newBitmap,
          newVector,
          newSelectionRect,
          newSelectionRectTemp,
          newTransformedSelectionRect,
          newRotationAngle,
          newScaleX,
          newScaleY,
          newIsSelected,
          newTransform,
          newLayerId);
}

void TransformElement::apply(const BitmapImage* bitmapImage,
                             const VectorImage* vectorImage,
                             const QRectF& selectionRect,
                             const QRectF& tempRect,
                             const QRectF& transformedRect,
                             const qreal rotationAngle,
                             const qreal scaleX,
                             const qreal scaleY,
                             const bool isSelected,
                             const QTransform& transform,
                             const int layerId)
{

    Layer* layer = editor()->layers()->findLayerById(layerId);
    Layer* currentLayer = editor()->layers()->currentLayer();

    if (layer->type() != currentLayer->type())
    {
        editor()->layers()->setCurrentLayer(layer);
    }

    auto selectMan = editor()->select();
    selectMan->setSelectionTransform(transform);
    selectMan->setSelectionRect(selectionRect);
    selectMan->setTempTransformedSelectionRect(tempRect);
    selectMan->setTransformedSelectionRect(transformedRect);
    selectMan->setRotation(rotationAngle);
    selectMan->setSomethingSelected(isSelected);
    selectMan->setScale(scaleX, scaleY);

    switch(layer->type())
    {
        case Layer::BITMAP:
        {
            if (bitmapImage->isMinimallyBounded()) {
                static_cast<LayerBitmap*>(layer)->replaceLastBitmapAtFrame(bitmapImage);
                KeyFrame* cKeyFrame = layer->getLastKeyFrameAtPosition(editor()->currentFrame());
                editor()->canvas()->paintTransformedSelection(layer,
                                                              cKeyFrame,
                                                              transform,
                                                              selectionRect);
            }
            break;
        }
        case Layer::VECTOR:
        {
            LayerVector* vlayer = static_cast<LayerVector*>(layer);
            vlayer->replaceLastVectorAtFrame(vectorImage);
            VectorImage* vecImage = vlayer->getLastVectorImageAtFrame(editor()->currentFrame(), 0);
            vecImage->setSelectionTransformation(transform);
            editor()->updateCurrentFrame();
            break;
        }
        default:
            break;

    }
}

ImportBitmapElement::ImportBitmapElement(const std::map<int, KeyFrame*, std::greater<int>>& backupCanvasKeyFrames,
                                         const std::map<int, KeyFrame*, std::less<int>>& backupImportedKeyFrames,
                                         const int& backupLayerId,
                                         Editor *editor,
                                         QUndoCommand *parent) : BackupElement(editor, parent)
{

    oldLayerId = backupLayerId;
    newLayerId = editor->layers()->currentLayer()->id();

    importedKeyFrames = backupImportedKeyFrames;
    oldKeyFrames = backupCanvasKeyFrames;

    setText(QObject::tr("Import images/s"));
}

void ImportBitmapElement::undo()
{
    for (auto key : importedKeyFrames)
    {
        editor()->removeKeyAtLayerId(oldLayerId,key.second->pos());
    }

    Layer* layer = editor()->layers()->findLayerById(oldLayerId);

    // we've removed all keyframes + those that were overwritten
    // now re-add the old ones
    LayerBitmap* layerBitmap = static_cast<LayerBitmap*>(layer);
    for (auto key : oldKeyFrames)
    {
        editor()->addKeyFrameToLayer(layer, key.first, true);
        layerBitmap->putBitmapIntoFrame(key.second, key.second->pos());
    }
    editor()->updateCurrentFrame();
}

void ImportBitmapElement::redo()
{
    if (isFirstRedo)
    {
        isFirstRedo = false;
        return;
    }

    Layer* layer = editor()->layers()->findLayerById(newLayerId);

    LayerBitmap* layerBitmap = static_cast<LayerBitmap*>(layer);
    for (auto key : importedKeyFrames)
    {
        editor()->addKeyFrameToLayer(layer, key.first, true);
        layerBitmap->putBitmapIntoFrame(key.second, key.second->pos());
    }
    editor()->updateCurrentFrame();
}

bool ImportBitmapElement::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id()) {
        return false;
    }

    auto element = static_cast<const ImportBitmapElement*>(other);
    newLayerId = element->newLayerId;

    auto importedKeys = element->importedKeyFrames;
    importedKeyFrames.insert(importedKeys.begin(), importedKeys.end());

    return true;
}

CameraMotionElement::CameraMotionElement(const int backupFrameIndex,
                                         const int backupLayerId,
                                         const QPointF& backupTranslation,
                                         const float backupRotation,
                                         const float backupScale,
                                         const QString& description,
                                         Editor* editor,
                                         QUndoCommand* parent) : BackupElement(editor, parent)
{

    frameIndex = backupFrameIndex;
    layerId = backupLayerId;

    oldTranslation = backupTranslation;
    oldRotation = backupRotation;
    oldScale = backupScale;

    ViewManager* viewMgr = editor->view();
    newTranslation = viewMgr->translation();
    newRotation = viewMgr->rotation();
    newScale = viewMgr->scaling();

    if (description.isEmpty()) {
        setText(QObject::tr("Camera: New motion"));
    } else {
        setText(description);
    }

}

void CameraMotionElement::undo()
{
    ViewManager* viewMgr = editor()->view();

    Layer* layer = editor()->layers()->findLayerById(layerId);
    editor()->scrubTo(layer, frameIndex);

    viewMgr->translate(oldTranslation);
    viewMgr->rotate(oldRotation);
    viewMgr->scale(oldScale);
}

void CameraMotionElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    Layer* layer = editor()->layers()->findLayerById(layerId);
    editor()->scrubTo(layer, frameIndex);

    ViewManager* viewMgr = editor()->view();
    viewMgr->translate(newTranslation);
    viewMgr->rotate(newRotation);
    viewMgr->scale(newScale);
}

bool CameraMotionElement::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id() || other->text() != text())
    {
        return false;
    }

    newTranslation = static_cast<const CameraMotionElement*>(other)->newTranslation;
    newRotation = static_cast<const CameraMotionElement*>(other)->newRotation;
    newScale = static_cast<const CameraMotionElement*>(other)->newScale;

    ViewManager* viewMgr = editor()->view();
    viewMgr->translate(newTranslation);
    viewMgr->rotate(newRotation);
    viewMgr->scale(newScale);
    return true;
}

AddLayerElement::AddLayerElement(Layer* backupLayer,
                                 Editor* editor,
                                 QUndoCommand* parent) : BackupElement(editor, parent)
{


    oldLayer = backupLayer;
    oldLayerId = backupLayer->id();

    Layer* layer = editor->layers()->currentLayer();
    newLayerType = layer->type();
    newLayerId = layer->id();

    switch(layer->type())
    {
        case Layer::BITMAP:
        {
            newLayer = new LayerBitmap(newLayerId, layer->object());
            break;
        }
        case Layer::VECTOR:
        {
            newLayer = new LayerVector(newLayerId, layer->object());
            break;
        }
        case Layer::SOUND:
        {
            newLayer = new LayerSound(newLayerId, layer->object());
            break;
        }
        case Layer::CAMERA:
        {
            newLayer = new LayerCamera(newLayerId, layer->object());
            break;
        }
        default:
            Q_ASSERT(false);
    }
    newLayerName = layer->name();


    setText(QObject::tr("New Layer"));
}

void AddLayerElement::undo()
{
    qDebug() << "undo";
    qDebug() << "oldLayerId:" << oldLayerId;
    qDebug() << "newLayerId:" << newLayerId;
    editor()->layers()->deleteLayerWithId(newLayerId);

}

void AddLayerElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    switch(newLayer->type())
    {
        case Layer::BITMAP:
        {
            editor()->layers()->createBitmapLayer(newLayerName);
            break;
        }
        case Layer::VECTOR:
        {
            editor()->layers()->createVectorLayer(newLayerName);
            break;
        }
        case Layer::SOUND:
        {
            editor()->layers()->createSoundLayer(newLayerName);
            break;
        }
        case Layer::CAMERA:
        {
            editor()->layers()->createCameraLayer(newLayerName);
            break;
        }
        default:
            break;
    }

}

DeleteLayerElement::DeleteLayerElement(const QString& backupLayerName,
                                       const Layer::LAYER_TYPE& backupType,
                                       const std::map<int, KeyFrame*, std::greater<int> >& backupLayerKeys,
                                       const int& backupFrameIndex,
                                       const int& backupLayerIndex,
                                       const int& backupLayerId,
                                       Editor* editor,
                                       QUndoCommand* parent) : BackupElement(editor, parent)
{


    oldFrameIndex = backupFrameIndex;
    oldLayerIndex = backupLayerIndex;
    oldLayerName = backupLayerName;
    oldLayerKeys = backupLayerKeys;
    oldLayerType = backupType;
    oldLayerId = backupLayerId;

    switch(oldLayerType)
    {
        case Layer::BITMAP:
        {
            setText(QObject::tr("Delete Bitmap Layer"));
            break;
        }
        case Layer::VECTOR:
        {
            setText(QObject::tr("Delete Vector Layer"));
            break;
        }
        case Layer::SOUND:
        {
            setText(QObject::tr("Delete Sound Layer"));
            break;
        }
        case Layer::CAMERA:
        {
            setText(QObject::tr("Delete Camera Layer"));
            break;
        }
        default:
            break;
    }
}

void DeleteLayerElement::undo()
{
    editor()->backups()->restoreLayerKeys(this);
}

void DeleteLayerElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    editor()->layers()->deleteLayerWithId(oldLayerId);

}

RenameLayerElement::RenameLayerElement(const QString& backupLayerName,
                                       const int& backupLayerId,
                                       Editor *editor,
                                       QUndoCommand *parent) : BackupElement(editor, parent)
{

    oldLayerName = backupLayerName;
    oldLayerId = backupLayerId;

    Layer* layer = editor->layers()->currentLayer();
    newLayerId = layer->id();
    newLayerName = layer->name();

    setText(QObject::tr("Rename layer"));
}

void RenameLayerElement::undo()
{
    Layer* layer = editor()->layers()->findLayerById(oldLayerId);
    editor()->layers()->renameLayer(layer, oldLayerName);
}

void RenameLayerElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }
    Layer* layer = editor()->layers()->findLayerById(newLayerId);
    editor()->layers()->renameLayer(layer, newLayerName);

}

CameraPropertiesElement::CameraPropertiesElement(const QString& backupLayerName,
                                                 const QRect& backupViewRect,
                                                 const int& backupLayerId,
                                                 Editor *editor,
                                                 QUndoCommand *parent) : BackupElement(editor, parent)
{

    oldLayerId = backupLayerId;
    oldViewRect = backupViewRect;
    oldLayerName = backupLayerName;

    LayerCamera* layer = static_cast<LayerCamera*>(editor->layers()->currentLayer());

    newLayerId = layer->id();
    newLayerName = layer->name();
    newViewRect = layer->getViewRect();

    if (oldViewRect != newViewRect)
    {
        setText(QObject::tr("Edit Camera Properties"));
    }
    else
    {
        setText(QObject::tr("Rename Layer"));
    }

}

void CameraPropertiesElement::undo()
{
    LayerManager* lmgr = editor()->layers();
    LayerCamera* layer = static_cast<LayerCamera*>(lmgr->findLayerById(oldLayerId));

    lmgr->renameLayer(layer, oldLayerName);
    layer->setViewRect(oldViewRect);
    editor()->updateCurrentFrame();

}

void CameraPropertiesElement::redo()
{

    if (isFirstRedo) { isFirstRedo = false; return; }

    LayerManager* lmgr = editor()->layers();
    LayerCamera* layer = static_cast<LayerCamera*>(lmgr->findLayerById(newLayerId));

    if (layer->name() != newLayerName)
    {
        lmgr->renameLayer(layer, newLayerName);
    }
    if (layer->getViewRect() != newViewRect)
    {
        layer->setViewRect(newViewRect);
    }
    editor()->updateCurrentFrame();
}

MoveFramesElement::MoveFramesElement(const int backupLayerId,
                                     const int backupScrubberFrameIndex,
                                     const int backupOffset,
                                     const bool wasSelected,
                                     const QList<int> selectedFrameIndexes,
                                     Editor* editor,
                                     QUndoCommand* parent) : BackupElement(editor, parent),
    offset(backupOffset)
{
    scrubberIndex = backupScrubberFrameIndex;

    layerId = backupLayerId;
    oldSelectedFrameIndexes = selectedFrameIndexes;
    newSelectedFrameIndexes = editor->layers()->findLayerById(layerId)->getSelectedFramesByPos();

    framesSelected = wasSelected;
    if (backupOffset < 0) {
        setText(QObject::tr("Move frame/s backward"));
    } else {
        setText(QObject::tr("Move frame/s forward"));
    }
}

void MoveFramesElement::undo()
{
    qDebug() << "UNDO";

    Layer* layer = editor()->layers()->findLayerById(layerId);
    if (!framesSelected) {
        qDebug() << "old index: " << scrubberIndex;
        qDebug() << "new index: " << scrubberIndex+offset;
        applyToSingle(layer, scrubberIndex, scrubberIndex+offset);
        editor()->scrubTo(layerId, scrubberIndex);
    } else {
        applyToMulti(layer, -offset, newSelectedFrameIndexes);
        editor()->layers()->setCurrentLayer(layer);
    }

    editor()->updateView();
}

void MoveFramesElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    Layer* layer = editor()->layers()->findLayerById(layerId);
    if (!framesSelected) {

        qDebug() << "old index: " << scrubberIndex;
        qDebug() << "new index: " << scrubberIndex+offset;
        applyToSingle(layer, scrubberIndex+offset, scrubberIndex);
        editor()->scrubTo(layerId, scrubberIndex+offset);
    } else {
        applyToMulti(layer, offset, oldSelectedFrameIndexes);
        editor()->layers()->setCurrentLayer(layer);
    }

    editor()->updateView();
}


void MoveFramesElement::applyToSingle(Layer* layer, const int oldFrameIndex, const int newFrameIndex)
{
    layer->swapKeyFrames(oldFrameIndex, newFrameIndex);
}

void MoveFramesElement::applyToMulti(Layer* layer, const int offset, const QList<int> selectedFrameIndexes)
{
    layer->setFramesSelected(selectedFrameIndexes, true);
    layer->moveSelectedFrames(offset);
    layer->deselectAll();
}

//SelectFramesElement::SelectFramesElement(const SelectionType selectionType,
//                                         const int backupOldLayerId,
//                                         const int backupFrameIndex,
//                                         const QList<int> backupFrameIndexes,
//                                         const QList<int> backupChangedSelectedIndexes,
//                                         const bool backupIsFrameSelected,
//                                         Editor* editor,
//                                         QUndoCommand* parent) : BackupElement (editor, parent),
//    oldLayerId(backupOldLayerId),
//    frameIndex(backupFrameIndex),
//    oldIsSelected(backupIsFrameSelected),
//    oldFrameIndexes(backupFrameIndexes),
//    selectionType(selectionType)
//{
//    oldChangedIndexes = backupChangedSelectedIndexes;

//    Layer* layer = editor->layers()->currentLayer();
////    newSelectedFrameIndexes = layer->getSelectedFrameIndexes();

//    newFrameIndexes = layer->getSelectedFrameIndexes();
//    newChangedIndexes = newFrameIndexes;
//    newLayerId = layer->id();


//    oldChangedIndexes = getUniqueFrames(oldChangedIndexes, backupFrameIndexes);
////    qDebug() << "old filtered:" << oldChangedIndexes;

//    if (selectionType == SelectionType::SELECTION) {
//        setText(QObject::tr("Select frame/s"));
//    } else {
//        setText(QObject::tr("Deselect frame/s"));
//    }
//}

//QList<int> SelectFramesElement::getUniqueFrames(const QList<int> frameIndexes, const QList<int> compareIndxes)
//{
//    QList<int> filteredFrames;
//    for (int i : frameIndexes) {
//        if (!compareIndxes.contains(i)) {
//            filteredFrames.append(i);
//        }
//    }

//    if (filteredFrames.count() > 1) {
//        moreFramesSelected = true;
//    } else {
//        moreFramesSelected = false;
//    }
//    return filteredFrames;
//}

////void SelectFramesElement::apply(const bool moreFramesSelected,
////                                const int layerId,
////                                const QList<int> changedFrameIndexes,
////                                const QList<int> undoFrameIndexes,
////                                const QList<int> redoFrameIndexes,
////                                const SelectionType& selectionType)
////{
////    Layer* layer = editor()->layers()->findLayerById(layerId);

////    if (selectionType == SelectionType::SELECTION) {

////        if (moreFramesSelected && changedFrameIndexes != undoFrameIndexes) {
////            layer->setFramesSelected(changedFrameIndexes, false);
////            layer->setFramesSelected(redoFrameIndexes, true);
////        } else {
////            layer->setFramesSelected(redoFrameIndexes, false);
////        }
////    } else {
////        layer->setFramesSelected(undoFrameIndexes, true);
////    }
////    editor()->updateTimeLine();
////    editor()->layers()->setCurrentLayer(layer);
////}

//void SelectFramesElement::undo()
//{

////    apply(moreFramesSelected,
////          newLayerId,
////          frameIndex,
////          oldNewlyFrameIndexes,
////          oldFrameIndexes,
////          newFrameIndexes,
////          selectionType);
//    Layer* layer = editor()->layers()->findLayerById(newLayerId);

//    if (selectionType == SelectionType::SELECTION) {


//        // FIXME: sometimes (using modifiers presumably) move and selections are not always undo/redo able...
//        if (moreFramesSelected && oldChangedIndexes != oldFrameIndexes) {
//            layer->deselectAll();
//            layer->setFramesSelected(oldFrameIndexes, true);
//        } else {
//            qDebug() << " \n newFrameIdx: " << newFrameIndexes;
//            qDebug() << "old indexes: " << oldFrameIndexes;
//            qDebug() << "oldChangedIndexes: " << oldChangedIndexes;
//            qDebug() << "newChanged Indexes" << newChangedIndexes;
////            layer->setFramesSelected(newFrameIndexes, false);
//            layer->deselectAll();
//            layer->setFramesSelected(oldFrameIndexes);
//        }
//    } else {
//        layer->setFramesSelected(oldFrameIndexes, true);
//    }

//    qDebug() << layer->getSelectedFrameIndexes();
//    editor()->updateTimeLine();
//    editor()->layers()->setCurrentLayer(layer);
//}

//void SelectFramesElement::redo()
//{
//    if (isFirstRedo) { isFirstRedo = false; return; }

////    apply(moreFramesSelected,
////          newLayerId,
////          newSelectedFrameIndexes,
////          newFrameIndexes,
////          oldFrameIndexes,
////          selectionType);

//    Layer* layer = editor()->layers()->findLayerById(newLayerId);

//    if (selectionType == SelectionType::SELECTION) {
//        if (moreFramesSelected && newChangedIndexes != newFrameIndexes) {
////            layer->setFramesSelected(newChangedIndexes, false);
//            layer->deselectAll();
//            layer->setFramesSelected(newFrameIndexes, true);
//        } else {
//            layer->deselectAll();
//            layer->setFramesSelected(newFrameIndexes, true);
//        }
//    } else {
//        layer->setFramesSelected(newFrameIndexes, true);
//    }
//    editor()->updateTimeLine();
//    editor()->layers()->setCurrentLayer(layer);
//}

//bool SelectFramesElement::mergeWith(const QUndoCommand *other)
//{
//    const SelectFramesElement* otherElem = static_cast<const SelectFramesElement*>(other);

//    if (otherElem->id() != id()) {
//        return false;
//    }

////    setText(otherElem->text());
////    oldFrameIndexes = otherElem->oldFrameIndexes;
//    newFrameIndexes = otherElem->newFrameIndexes;
//    frameIndex = otherElem->frameIndex;
////    oldLayerId = otherElem->oldLayerId;
//    newLayerId = otherElem->newLayerId;
////    oldNewlyFrameIndexes = otherElem->oldNewlyFrameIndexes;
//    newChangedIndexes = otherElem->newChangedIndexes;
//    newChangedIndexes = getUniqueFrames(newChangedIndexes, newFrameIndexes);

//    if (newChangedIndexes.isEmpty() && newFrameIndexes.isEmpty()) {
//        setObsolete(true);
//////        return false;
//    }
////    selectionType = otherElem->selectionType;

//    // TODO: figure out to merge select deselect...
////    oldIsSelected = otherElem->oldIsSelected;
//    return true;
//}

FlipViewElement::FlipViewElement(const bool& backupFlipState,
                                 const Direction& backupFlipDirection,
                                 Editor *editor,
                                 QUndoCommand *parent) : BackupElement(editor, parent)
{


    isFlipped = backupFlipState;
    direction = backupFlipDirection;

    if (direction == Direction::HORIZONTAL)
    {
        setText(QObject::tr("Flip View X"));
    }
    else
    {
        setText(QObject::tr("Flip View Y"));
    }
}

void FlipViewElement::undo()
{
    if (direction == Direction::VERTICAL)
    {
        editor()->view()->flipVertical(!isFlipped);
    }
    else
    {
        editor()->view()->flipHorizontal(!isFlipped);
    }
}

void FlipViewElement::redo()
{

    if (isFirstRedo) { isFirstRedo = false; return; }

    if (direction == Direction::VERTICAL)
    {
        editor()->view()->flipVertical(isFlipped);
    }
    else
    {
        editor()->view()->flipHorizontal(isFlipped);
    }
}

MoveLayerElement::MoveLayerElement(const int& backupOldLayerIndex,
                                   const int& backupNewLayerIndex,
                                   Editor* editor,
                                   QUndoCommand* parent) : BackupElement(editor, parent)
{
    oldLayerIndex = backupOldLayerIndex;
    newLayerIndex = backupNewLayerIndex;

    setText(QObject::tr("Move layer"));
}

void MoveLayerElement::undo()
{
    editor()->swapLayers(newLayerIndex, oldLayerIndex);
    editor()->layers()->setCurrentLayer(oldLayerIndex);
}

void MoveLayerElement::redo()
{
    if (isFirstRedo) { isFirstRedo = false; return; }

    editor()->swapLayers(oldLayerIndex, newLayerIndex);
    editor()->layers()->setCurrentLayer(newLayerIndex);

}
