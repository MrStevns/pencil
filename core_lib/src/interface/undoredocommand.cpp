/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2008-2009 Mj Mendoza IV
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include <QDebug>

#include "layermanager.h"
#include "selectioneditor.h"
#include "selectionmanager.h"

#include "layersound.h"
#include "layerbitmap.h"
#include "layervector.h"
#include "layer.h"

#include "editor.h"
#include "undoredocommand.h"

UndoRedoCommand::UndoRedoCommand(Editor* editor, QUndoCommand* parent) : QUndoCommand(parent)
{
    qDebug() << "backupElement created";
    mEditor = editor;
}

UndoRedoCommand::~UndoRedoCommand()
{
}

KeyFrameRemoveCommand::KeyFrameRemoveCommand(const KeyFrame* undoKeyFrame,
                                         int undoLayerId,
                                         const QString &description,
                                         Editor *editor,
                                         QUndoCommand *parent) : UndoRedoCommand(editor, parent)
{
    this->undoKeyFrame = undoKeyFrame->clone();
    this->undoLayerId = undoLayerId;

    this->redoLayerId = editor->layers()->currentLayer()->id();
    this->redoPosition = editor->currentFrame();

    setText(description);
}

KeyFrameRemoveCommand::~KeyFrameRemoveCommand()
{
    delete undoKeyFrame;
}

void KeyFrameRemoveCommand::undo()
{
    UndoRedoCommand::undo();

    Layer* layer = editor()->layers()->findLayerById(undoLayerId);
    if (layer == nullptr) {
        // Until we support layer deletion recovery, we mark the command as
        // obsolete as soon as it's been
        return setObsolete(true);
    }

    layer->addKeyFrame(undoKeyFrame->pos(), undoKeyFrame->clone());

    emit editor()->frameModified(undoKeyFrame->pos());
    editor()->layers()->notifyAnimationLengthChanged();
    editor()->scrubTo(undoKeyFrame->pos());
}

void KeyFrameRemoveCommand::redo()
{
    UndoRedoCommand::redo();

    if (isFirstRedo()) { setFirstRedo(false); return; }

    Layer* layer = editor()->layers()->findLayerById(redoLayerId);
    layer->removeKeyFrame(redoPosition);

    emit editor()->frameModified(redoPosition);
    editor()->layers()->notifyAnimationLengthChanged();
    editor()->scrubTo(redoPosition);
}

KeyFrameAddCommand::KeyFrameAddCommand(int undoPosition,
                                       int undoLayerId,
                                       const QString &description,
                                       Editor *editor,
                                       QUndoCommand *parent)
    : UndoRedoCommand(editor, parent)
{
    this->undoPosition = undoPosition;
    this->undoLayerId = undoLayerId;

    this->redoLayerId = editor->layers()->currentLayer()->id();
    this->redoPosition = editor->currentFrame();

    setText(description);
}

KeyFrameAddCommand::~KeyFrameAddCommand()
{
}

void KeyFrameAddCommand::undo()
{
    UndoRedoCommand::undo();

    Layer* layer = editor()->layers()->findLayerById(undoLayerId);
    if (!layer) {
        return setObsolete(true);
    }

    layer->removeKeyFrame(undoPosition);

    emit editor()->frameModified(undoPosition);
    editor()->layers()->notifyAnimationLengthChanged();
    editor()->layers()->setCurrentLayer(layer);
    editor()->scrubTo(undoPosition);
}

void KeyFrameAddCommand::redo()
{
    UndoRedoCommand::redo();

    // Ignore automatic redo when added to undo stack
    if (isFirstRedo()) { setFirstRedo(false); return; }

    Layer* layer = editor()->layers()->findLayerById(redoLayerId);
    if (!layer) {
        return setObsolete(true);
    }

    layer->addNewKeyFrameAt(redoPosition);

    emit editor()->frameModified(redoPosition);
    editor()->layers()->notifyAnimationLengthChanged();
    editor()->layers()->setCurrentLayer(layer);
    editor()->scrubTo(redoPosition);
}

MoveKeyFramesCommand::MoveKeyFramesCommand(int offset,
                                         QList<int> listOfPositions,
                                         int undoLayerId,
                                         const QString& description,
                                         Editor* editor,
                                         QUndoCommand *parent)
    : UndoRedoCommand(editor, parent)
{
    this->frameOffset = offset;
    this->positions = listOfPositions;

    this->undoLayerId = undoLayerId;
    this->redoLayerId = editor->layers()->currentLayer()->id();

    setText(description);
}

void MoveKeyFramesCommand::undo()
{
    UndoRedoCommand::undo();

    Layer* undoLayer = editor()->layers()->findLayerById(undoLayerId);

    if (!undoLayer) {
        return setObsolete(true);
    }

    for (int position : qAsConst(positions)) {
        undoLayer->moveKeyFrame(position + frameOffset, -frameOffset);
    }

    emit editor()->framesModified();
}

void MoveKeyFramesCommand::redo()
{
    UndoRedoCommand::redo();

    // Ignore automatic redo when added to undo stack
    if (isFirstRedo()) { setFirstRedo(false); return; }

    Layer* redoLayer = editor()->layers()->findLayerById(redoLayerId);

    if (!redoLayer) {
        return setObsolete(true);
    }

    for (int position : qAsConst(positions)) {
        redoLayer->moveKeyFrame(position, frameOffset);
    }

    emit editor()->framesModified();
}
BitmapReplaceCommand::BitmapReplaceCommand(const BitmapImage* undoBitmap,
                             const int undoLayerId,
                             const QString& description,
                             Editor *editor,
                             QUndoCommand *parent) : UndoRedoCommand(editor, parent)
{

    this->undoBitmap = *undoBitmap;
    this->undoLayerId = undoLayerId;

    Layer* layer = editor->layers()->currentLayer();
    redoLayerId = layer->id();
    redoBitmap = *static_cast<LayerBitmap*>(layer)->
            getBitmapImageAtFrame(editor->currentFrame());

    setText(description);
}

void BitmapReplaceCommand::undo()
{
    UndoRedoCommand::undo();

    Layer* layer = editor()->layers()->findLayerById(undoLayerId);
    if (!layer) {
        return setObsolete(true);
    }

    static_cast<LayerBitmap*>(layer)->replaceKeyFrame(&undoBitmap);

    editor()->scrubTo(undoBitmap.pos());
}

void BitmapReplaceCommand::redo()
{
    UndoRedoCommand::redo();

    // Ignore automatic redo when added to undo stack
    if (isFirstRedo()) { setFirstRedo(false); return; }

    Layer* layer = editor()->layers()->findLayerById(redoLayerId);
    if (!layer) {
        return setObsolete(true);
    }

    static_cast<LayerBitmap*>(layer)->replaceKeyFrame(&redoBitmap);

    editor()->scrubTo(redoBitmap.pos());
}

VectorReplaceCommand::VectorReplaceCommand(const VectorImage* undoVector,
                                   const int undoLayerId,
                                   const QString& description,
                                   Editor* editor,
                                   QUndoCommand* parent) : UndoRedoCommand(editor, parent)
{

    this->undoVector = *undoVector;
    this->undoLayerId = undoLayerId;
    Layer* layer = editor->layers()->currentLayer();
    redoLayerId = layer->id();
    redoVector = *static_cast<LayerVector*>(layer)->
            getVectorImageAtFrame(editor->currentFrame());

    setText(description);
}

void VectorReplaceCommand::undo()
{
    UndoRedoCommand::undo();

    Layer* layer = editor()->layers()->findLayerById(undoLayerId);
    if (!layer) {
        return setObsolete(true);
    }

    static_cast<LayerVector*>(layer)->replaceKeyFrame(&undoVector);

    editor()->scrubTo(undoVector.pos());
}

void VectorReplaceCommand::redo()
{
    UndoRedoCommand::redo();

    // Ignore automatic redo when added to undo stack
    if (isFirstRedo()) { setFirstRedo(false); return; }

    Layer* layer = editor()->layers()->findLayerById(redoLayerId);
    if (!layer) {
        return setObsolete(true);
    }

    static_cast<LayerVector*>(layer)->replaceKeyFrame(&redoVector);

    editor()->scrubTo(redoVector.pos());
}

TransformCommand::TransformCommand(int undoLayerId,
                                   int undoKeyPos,
                                   SelectionEditor* undoSelectionEditor,
                                   const QString& description,
                                   Editor* editor,
                                   QUndoCommand *parent) : UndoRedoCommand(editor, parent)
{
    this->undoLayerId = undoLayerId;
    this->undoKeyPos = undoKeyPos;

    this->redoLayerId = editor->layers()->currentLayer()->id();
    this->redoKeyPos = editor->currentFrame();

    // this->undoAnchor = undoSelectionEditor->currentTransformAnchor();
    // this->undoRotationAngle = undoSelectionEditor->myRotation();
    // this->undoScaleX = undoSelectionEditor->myScaleX();
    // this->undoScaleY = undoSelectionEditor->myScaleY();
    // this->undoTranslation = undoSelectionEditor->myTranslation();

    Layer* layer = editor->layers()->findLayerById(redoLayerId);
    SelectionEditor* currentSelectionEditor = nullptr;
    if (layer->type() == Layer::BITMAP) {
        currentSelectionEditor = static_cast<BitmapImage*>(layer->getKeyFrameAt(redoKeyPos))->selectionEditor();
        // selectionEditor->setSelection();
    }
    else
    {
        // TODO: implement for vector
        // selectionEditor = static_cast<VectorImage*>(layer->getKeyFrameAt(undoKeyPos))->selectionEditor();
    }

    if (currentSelectionEditor) {
        // this->redoSelectionRect = currentSelectionEditor->mySelectionRect();
        this->redoAnchor = currentSelectionEditor->currentTransformAnchor();
        this->redoRotationAngle = currentSelectionEditor->myRotation();
        this->redoScaleX = currentSelectionEditor->myScaleX();
        this->redoScaleY = currentSelectionEditor->myScaleY();
        this->redoTranslation = currentSelectionEditor->myTranslation();

        this->undoAnchor = currentSelectionEditor->currentTransformAnchor();
        this->undoRotationAngle = currentSelectionEditor->myRotation();
        this->undoScaleX = currentSelectionEditor->myScaleX();
        this->undoScaleY = currentSelectionEditor->myScaleY();
        this->undoTranslation = currentSelectionEditor->myTranslation();

        // this->undoSelectionRect = currentSelectionEditor->mapToSelection(QPolygonF(QRectF(undoSelectionEditor->mySelectionRect()))).boundingRect();
    }

    setText(description);
}

void TransformCommand::undo()
{
    UndoRedoCommand::undo();

    apply(undoLayerId,
          undoKeyPos,
          undoSelectionRect,
          undoTranslation,
          undoRotationAngle,
          undoScaleX,
          undoScaleY,
          undoAnchor);
}

void TransformCommand::redo()
{
    UndoRedoCommand::redo();

    // // Ignore automatic redo when added to undo stack
    if (isFirstRedo()) { setFirstRedo(false); return; }

    apply(redoLayerId,
          redoKeyPos,
          redoSelectionRect,
          redoTranslation,
          redoRotationAngle,
          redoScaleX,
          redoScaleY,
          redoAnchor);
}

void TransformCommand::apply(int layerId,
                             int keyPos,
                             const QRectF& selectionRect,
                             const QPointF& translation,
                             const qreal rotationAngle,
                             const qreal scaleX,
                             const qreal scaleY,
                             const QPointF& selectionAnchor)
{
    Layer* layer = editor()->layers()->findLayerById(layerId);
    SelectionEditor* selectionEditor = nullptr;
    if (layer->type() == Layer::BITMAP) {
        selectionEditor = static_cast<BitmapImage*>(layer->getKeyFrameAt(keyPos))->selectionEditor();
    } else {
        // TODO: implement for vector
        // selectionEditor = static_cast<VectorImage*>(layer->getKeyFrameAt(undoKeyPos))->selectionEditor();
    }

    if (selectionEditor) {
        selectionEditor->setSelection(selectionRect);
        selectionEditor->setTranslation(translation);
        selectionEditor->setRotation(rotationAngle);
        selectionEditor->setScale(scaleX, scaleY);
        selectionEditor->setTransformAnchor(selectionAnchor);
        selectionEditor->calculateSelectionTransformation();
    }
}
