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

#include "object.h"
#include "editor.h"

#include <QDebug>

#include "layermanager.h"
#include "soundmanager.h"
#include "backupmanager.h"
#include "viewmanager.h"
#include "selectionmanager.h"

#include "backupelement.h"

#include "layerbitmap.h"
#include "layercamera.h"
#include "layervector.h"
#include "layersound.h"

#include "bitmapimage.h"
#include "vectorimage.h"
#include "soundclip.h"
#include "camera.h"

BackupManager::BackupManager(Editor* editor) : BaseManager(editor, "BackupManager")
{
    qDebug() << "BackupManager: created";
}

BackupManager::~BackupManager()
{
    qDebug() << "BackupManager: destroyed";
}

bool BackupManager::init()
{
    mUndoStack = new QUndoStack(this);
    qDebug() << "BackupManager: init";

    return true;
}

Status BackupManager::load(Object* /*o*/)
{
    return Status::OK;
}

Status BackupManager::save(Object* /*o*/)
{
    return Status::OK;
}

const BackupElement* BackupManager::currentBackup()
{
    if (mUndoStack->count())
    {
        return static_cast<const BackupElement*>(mUndoStack->command(mUndoStack->index()-1));
    }
    else
    {
        return nullptr;
    }
}

void BackupManager::keyAdded(const int& keySpacing, const bool& keyExisted, const QString& description)
{
    if (mLayer == nullptr) { return; }

    AddKeyFrameElement* element = new AddKeyFrameElement(mFrameIndex,
                                                         mLayerId,
                                                         mEmptyFrameSettingVal,
                                                         keySpacing,
                                                         keyExisted,
                                                         description,
                                                         editor());
    mUndoStack->push(element);

    emit updateBackup();
}

void BackupManager::keyAdded(const QString& description)
{
    if (mLayer == nullptr) { return; }

    AddKeyFrameElement* element = new AddKeyFrameElement(mFrameIndex,
                                                         mLayerId,
                                                         mEmptyFrameSettingVal,
                                                         false,
                                                         false,
                                                         description,
                                                         editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::importBitmap(const std::map<int, KeyFrame*, std::greater<int>>& canvasKeys,
                                 const std::map<int, KeyFrame*, std::less<int>>& importedKeys)
{
    if (mLayer == nullptr) { return; }
    if (mLayer->type() != Layer::BITMAP) { return; }

    ImportBitmapElement* element = new ImportBitmapElement(canvasKeys,
                                                         importedKeys,
                                                         mLayerId,
                                                         editor());

    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::keyRemoved()
{
    if (mLayer == nullptr) { return; }
    if (mKeyframe == nullptr) { return; }

    RemoveKeyFrameElement* element = new RemoveKeyFrameElement(mKeyframe,
                                                               mLayerId,
                                                               editor());
    mUndoStack->push(element);
    emit updateBackup();

}

void BackupManager::bitmap(const QString& description)
{
    if (mBitmap == nullptr) { return; }
    AddBitmapElement* element = new AddBitmapElement(mBitmap,
                                                     mLayerId,
                                                     mEmptyFrameSettingVal,
                                                     description,
                                                     editor());

    if (mIsSelected)
    {
        new TransformElement(mKeyframe,
                             mLayerId,
                             mEmptyFrameSettingVal,
                             mSelectionRect,
                             mTempSelectionRect,
                             mTransformedSelectionRect,
                             mSelectionRotationAngle,
                             mSelectionScaleX,
                             mSelectionScaleY,
                             mIsSelected,
                             mSelectionTransform,
                             description,
                             editor(), element);
    }
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::vector(const QString& description)
{
    if (mVector == nullptr) { return; }
    AddVectorElement* element = new AddVectorElement(mVector,
                                                     mLayerId,
                                                     mEmptyFrameSettingVal,
                                                     description,
                                                     editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::selection()
{
    SelectionElement* element = new SelectionElement(mLayerId,
                                                     mFrameIndex,
                                                     mVectorSelection,
                                                     SelectionType::SELECTION,
                                                     mSelectionRect,
                                                     mSelectionRotationAngle,
                                                     mIsSelected,
                                                     editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::deselect()
{
    SelectionElement* element = new SelectionElement(mLayerId,
                                                     mFrameIndex,
                                                     mVectorSelection,
                                                     SelectionType::DESELECT,
                                                     mTransformedSelectionRect,
                                                     mSelectionRotationAngle,
                                                     mIsSelected,
                                                     editor());
    mUndoStack->push(element);

    emit updateBackup();
}

void BackupManager::transform(const QString& description)
{
    if (!mIsSelected) { return; }
    TransformElement* element = new TransformElement(mKeyframe,
                                                     mLayerId,
                                                     mEmptyFrameSettingVal,
                                                     mSelectionRect,
                                                     mTempSelectionRect,
                                                     mTransformedSelectionRect,
                                                     mSelectionRotationAngle,
                                                     mSelectionScaleX,
                                                     mSelectionScaleY,
                                                     mIsSelected,
                                                     mSelectionTransform,
                                                     description,
                                                     editor());
    mUndoStack->push(element);
    emit updateBackup();
}


/**
 * @brief Get the frame index for the keyframe which is being painted to
 *
 * @param layer
 * @param frameIndex <- current frame
 * @param usingPreviousFrameAction <- This is whether DRAW_ON_EMPTY_FRAME_ACTION is active
 * @return frameindex
 */
int BackupManager::getActiveFrameIndex(Layer* layer, const int frameIndex, const DrawOnEmptyFrameAction& frameAction)
{
    int activeFrameIndex = frameIndex;
    if (!layer->keyExists(frameIndex)) {
        if (frameAction == DrawOnEmptyFrameAction::KEEP_DRAWING_ON_PREVIOUS_KEY)
        {
            activeFrameIndex = layer->getPreviousKeyFramePosition(frameIndex);
        }
    }
    return activeFrameIndex;
}

void BackupManager::restoreLayerKeys(const BackupElement* backupElement)
{

    const DeleteLayerElement* lastBackupLayerElem = static_cast<const DeleteLayerElement*>(backupElement);
    LayerManager* layerMgr = editor()->layers();
    Layer* layer = nullptr;

    int oldFrameIndex = lastBackupLayerElem->oldFrameIndex;
    int layerIndex = lastBackupLayerElem->oldLayerIndex;
    int layerId = lastBackupLayerElem->oldLayerId;
    QString layerName = lastBackupLayerElem->oldLayerName;

    switch(lastBackupLayerElem->oldLayerType)
    {
        case Layer::BITMAP:
        {
            layerMgr->createBitmapLayerContainingKeyFrames(lastBackupLayerElem->oldLayerKeys,
                                                                     layerId,
                                                                     layerIndex,
                                                                     layerName);
            break;
        }
        case Layer::VECTOR:
        {

           layerMgr->createVectorLayerContainingKeyFrames(lastBackupLayerElem->oldLayerKeys,
                                                                    layerId,
                                                                    layerIndex,
                                                                    layerName);
            break;
        }
        case Layer::SOUND:
        {
            layer = layerMgr->createSoundLayerAt(layerId,
                                                         layerIndex,
                                                          layerName);
            for (auto map : lastBackupLayerElem->oldLayerKeys)
            {
                int frameIndex = map.second->pos();
                editor()->sound()->loadSound(layer, frameIndex, map.second->fileName());
            }
            break;
        }
        case Layer::CAMERA:
        {
            layerMgr->createCameraLayerContainingKeyFrames(lastBackupLayerElem->oldLayerKeys,
                                                           layerId,
                                                           layerIndex,
                                                           layerName);
            break;
        }
        default:
            break;
    }
    editor()->scrubTo(oldFrameIndex);
}

void BackupManager::restoreKey(const BackupElement* backupElement)
{
    Layer* layer = nullptr;
    KeyFrame* keyFrame = nullptr;
    int frame = 0;
    int layerIndex = 0;
    int layerId = 0;

    if (backupElement->type() == ADD_KEY_MODIF)
    {
        const AddKeyFrameElement* lastBackupElement = static_cast<const AddKeyFrameElement*>(backupElement);
        layerIndex = lastBackupElement->newLayerIndex;
        frame = lastBackupElement->newFrameIndex;
        layerId = lastBackupElement->newLayerId;
        keyFrame = lastBackupElement->newKey;
        layer = object()->findLayerById(layerId);

        restoreKey(layerId, frame, keyFrame);
        editor()->scrubTo(layerId, frame);
    }
    else // REMOVE_KEY_MODIF
    {
        const RemoveKeyFrameElement* lastBackupElement = static_cast<const RemoveKeyFrameElement*>(backupElement);
        layerIndex = lastBackupElement->oldLayerIndex;
        frame = lastBackupElement->oldFrameIndex;
        layerId = lastBackupElement->oldLayerId;
        keyFrame = lastBackupElement->oldKey;
        layer = editor()->layers()->findLayerById(layerId);

        restoreKey(layerId, frame, keyFrame);
        editor()->scrubTo(layerId, frame);
    }
}

void BackupManager::restoreKey(const int& layerId, const int& frame, KeyFrame *keyFrame)
{
    Layer* layer = editor()->layers()->findLayerById(layerId);

    if (!layer->keyExists(frame))
    {
        editor()->addKeyFrameToLayerId(layerId, frame, true);
    }

    switch(layer->type())
    {
        case Layer::BITMAP:
        {
            static_cast<LayerBitmap*>(layer)->putBitmapIntoFrame(keyFrame, frame);
            break;
        }
        case Layer::VECTOR:
        {
            static_cast<LayerVector*>(layer)->putVectorImageIntoFrame(keyFrame, frame);
            break;
        }
        case Layer::SOUND:
        {
            editor()->sound()->loadSound(layer, frame, keyFrame->fileName());
            break;
        }
        case Layer::CAMERA:
        {
            static_cast<LayerCamera*>(layer)->putCameraIntoFrame(keyFrame, frame);
            break;
        }
        default:
            break;
    }
    editor()->updateView();
}

void BackupManager::cameraMotion(const QString& description)
{
    if (mLayer == nullptr) { return; }

    CameraMotionElement* element = new CameraMotionElement(mFrameIndex,
                                                           mLayerId,
                                                           mTranslation,
                                                           mRotation,
                                                           mScale,
                                                           description,
                                                           editor());
    mUndoStack->push(element);

    emit updateBackup();
}

void BackupManager::layerAdded()
{
    AddLayerElement* element = new AddLayerElement(mLayer, editor());
    mUndoStack->push(element);

    emit updateBackup();
}

void BackupManager::layerDeleted(const std::map<int, KeyFrame*, std::greater<int> >& oldKeys)
{

    DeleteLayerElement* element = new DeleteLayerElement(mLayerName,
                                                         mLayerType,
                                                         oldKeys,
                                                         mFrameIndex,
                                                         mLayerIndex,
                                                         mLayerId,
                                                         editor());
    mUndoStack->push(element);

    emit updateBackup();
}

void BackupManager::layerRenamed()
{
    RenameLayerElement* element = new RenameLayerElement(mLayerName,
                                                         mLayerId,
                                                         editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::cameraProperties(const QRect& backupViewRect)
{
    CameraPropertiesElement* element = new CameraPropertiesElement(mLayerName,
                                                                   backupViewRect,
                                                                   mLayerId,
                                                                   editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::frameDeselected(const int frameIndex)
{
    frameDeselected(QList<int>({frameIndex}), frameIndex);
}

void BackupManager::frameDeselected(const QList<int> newDeselectedIndexes, const int frameIndex)
{

//    SelectFramesElement* element = new SelectFramesElement(SelectionType::DESELECT,
//                                                           mLayerId,
//                                                           frameIndex,
//                                                           mFrameIndexes,
//                                                           newDeselectedIndexes,
//                                                           false,
//                                                           editor());

//    mUndoStack->push(element);
//    emit updateBackup();
}

void BackupManager::frameSelected(const QList<int> newSelectedIndexes, const int frameIndex, const bool isSelected)
{

//    SelectFramesElement* element = new SelectFramesElement(SelectionType::SELECTION,
//                                                           mLayerId,
//                                                           frameIndex,
//                                                           mFrameIndexes,
//                                                           newSelectedIndexes,
//                                                           isSelected,
//                                                           editor());

//    mUndoStack->push(element);
//    emit updateBackup();
}

void BackupManager::frameMoved(const int offset)
{
    MoveFramesElement* element = new MoveFramesElement(mLayerId,
                                                       mFrameIndex,
                                                       offset,
                                                       false,
                                                       QList<int>(),
                                                       editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::framesMoved(const int offset,
                                const int scrubberFrameIndex)
{
    MoveFramesElement* element = new MoveFramesElement(mLayerId,
                                                       scrubberFrameIndex,
                                                       offset,
                                                       true,
                                                       mFrameIndexes,
                                                       editor());
    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::flipView(const bool& backupIsFlipped, const Direction& backupFlipDirection)
{
    FlipViewElement* element = new FlipViewElement(backupIsFlipped,
                                                   backupFlipDirection,
                                                   editor());

    mUndoStack->push(element);
    emit updateBackup();
}

void BackupManager::toggleSetting(bool /*backupToggleState*/, const SETTING& /*backupType*/)
{
//    ToggleSettingElement* element = new ToggleSettingElement(backupToggleState,
//                                                             backupType,
//                                                             editor());

//    mUndoStack->push(element);
//    emit updateBackup();
}

void BackupManager::layerMoved(const int& backupNewLayerIndex)
{
    MoveLayerElement* element = new MoveLayerElement(mLayerIndex,
                                                     backupNewLayerIndex,
                                                     editor());
    mUndoStack->push(element);
    emit updateBackup();
}

/**
 * @brief BackupManager::saveStates
 * This method should be called prior to a backup taking place.
 */
void BackupManager::saveStates()
{
    mBitmap = nullptr;
    mVector = nullptr;
    mCamera = nullptr;
    mClip = nullptr;
    mKeyframe = nullptr;

    mLayer = editor()->layers()->currentLayer();
    mLayerId = mLayer->id();

    mEmptyFrameSettingVal = static_cast<DrawOnEmptyFrameAction>(editor()->preference()->getInt(SETTING::DRAW_ON_EMPTY_FRAME_ACTION));

    mFrameIndex = editor()->currentFrame();
    mFrameIndex = BackupManager::getActiveFrameIndex(mLayer, mFrameIndex, mEmptyFrameSettingVal);

    auto selectMan = editor()->select();
    mIsSelected = selectMan->somethingSelected();
    mSelectionRect = selectMan->mySelectionRect();
    mTempSelectionRect = selectMan->myTempTransformedSelectionRect();
    mTransformedSelectionRect = selectMan->myTransformedSelectionRect();
    mSelectionRotationAngle = selectMan->myRotation();
    mMoveOffset = selectMan->getTransformOffset();
    mSelectionTransform = selectMan->selectionTransform();
    mSelectionScaleX = selectMan->myScaleX();
    mSelectionScaleY = selectMan->myScaleY();
    mMoveMode = selectMan->getMoveMode();
    mVectorSelection = selectMan->vectorSelection;

    mFrameIndexes = mLayer->getSelectedFramesByPos();

    mLayerName = mLayer->name();
    mLayerIndex = editor()->currentLayerIndex();
    mLayerType = mLayer->type();

    ViewManager* viewMgr = editor()->view();
    mTranslation = viewMgr->translation();
    mScale = viewMgr->scaling();
    mRotation = viewMgr->rotation();

    if (mLayer->keyExists(mFrameIndex))
    {
        mKeyframe = mLayer->getLastKeyFrameAtPosition(mFrameIndex)->clone();
    }
    else if (mLayer->getKeyFrameWhichCovers(mFrameIndex) != nullptr)
    {
        mKeyframe = mLayer->getKeyFrameWhichCovers(mFrameIndex)->clone();
    }

    switch(mLayer->type())
    {
        case Layer::BITMAP:
        {
            mBitmap = static_cast<BitmapImage*>(mKeyframe);
            break;
        }
        case Layer::VECTOR:
        {
            mVector = static_cast<VectorImage*>(mKeyframe);
            break;
        }
        case Layer::SOUND:
        {
            mClip = static_cast<SoundClip*>(mKeyframe);
            break;
        }
        case Layer::CAMERA:
        {
            mCamera = static_cast<Camera*>(mKeyframe);
            break;
        }
        default:
            break;
    }
}
