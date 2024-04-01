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

#ifndef BACKUPELEMENT_H
#define BACKUPELEMENT_H

#include <QObject>
#include <QUndoCommand>
#include <QRectF>
#include <QTransform>

#include "direction.h"
#include "movemode.h"
#include "pencildef.h"
#include "layer.h"
#include "vectorselection.h"
#include "preferencemanager.h"

class Editor;
class BackupManager;
class PreferenceManager;
class BitmapImage;
class VectorImage;
class SoundClip;
class Camera;
class Layer;
class KeyFrame;

enum types { UNDEFINED,
             ADD_KEY_MODIF,
             REMOVE_KEY_MODIF
           };

class BackupElement : public QUndoCommand
{
public:
    explicit BackupElement(Editor* editor, QUndoCommand* parent = nullptr);
    virtual ~BackupElement();

    Editor* editor() { return mEditor; }

    virtual int type() const { return UNDEFINED; }
    virtual void undo() { Q_ASSUME(true); } // should never end here
    virtual void redo() { Q_ASSUME(true); } // should never end here
private:
    Editor* mEditor = nullptr;
};

class TransformElement;

class AddBitmapElement : public BackupElement
{
public:
    AddBitmapElement(const BitmapImage* backupBitmap,
                     const int& backupLayerId,
                     const DrawOnEmptyFrameAction& frameAction,
                     QString description,
                     Editor* editor,
                     QUndoCommand* parent = nullptr);

    int oldLayerIndex = 0;
    int newLayerIndex = 0;

    int oldFrameIndex = 0;
    int newFrameIndex = 0;
    int previousFrameIndex = 0;

    int otherFrameIndex = 0;

    int oldLayerId = 0;
    int newLayerId = 0;

    int emptyFrameSettingVal = -1;

    BitmapImage* oldBitmap = nullptr;
    BitmapImage* newBitmap = nullptr;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;

    void redoTransform(const TransformElement* childElem);
    void undoTransform(const TransformElement* childElem);
};

class AddVectorElement : public BackupElement
{
public:
    AddVectorElement(const VectorImage* backupVector,
                     const int& backupLayerId,
                     const DrawOnEmptyFrameAction& backupFrameAction,
                     QString description,
                     Editor* editor,
                     QUndoCommand* parent = nullptr);

    int newLayerIndex = 0;
    int oldFrameIndex = 0;
    int newFrameIndex = 0;

    int newLayerId = 0;
    int oldLayerId = 0;
    int emptyFrameSettingVal = -1;

    VectorImage* oldVector = nullptr;
    VectorImage* newVector = nullptr;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;

};

class AddKeyFrameElement : public BackupElement
{
public:
    enum { Id = 5 };
    AddKeyFrameElement(const int backupFrameIndex,
                       const int backupLayerId,
                       const DrawOnEmptyFrameAction& backupFrameAction,
                       const int backupKeySpacing,
                       const bool backupKeyExisted,
                       QString description,
                       Editor* editor,
                       QUndoCommand* parent = nullptr);

    void undoSequence();
    void redoSequence();


    int newLayerIndex = 0;
    int newFrameIndex = 0;

    int oldFrameIndex;
    int oldLayerId;
    int oldKeySpacing;
    bool oldKeyExisted;

    int newLayerId;

    std::map<int, KeyFrame*>oldKeyFrames;
    std::map<int, KeyFrame*>newKeyFrames;

    KeyFrame* newKey = nullptr;
    int emptyFrameSettingVal = -1;

    bool isFirstRedo = true;

    int type() const override { return ADD_KEY_MODIF; }
    void undo() override;
    void redo() override;
    int id() const override { return Id; }
    bool mergeWith(const QUndoCommand *other) override;
};

class RemoveKeyFrameElement : public BackupElement
{
public:
    enum { Id = 4 };
    RemoveKeyFrameElement(const KeyFrame* backupBitmap,
                          const int& backupLayerId,
                          Editor* editor,
                          QUndoCommand* parent = nullptr);

    int oldLayerIndex = 0;
    int oldFrameIndex = 0;
    int oldLayerId = 0;

    KeyFrame* oldKey = nullptr;

    BitmapImage* oldBitmap = nullptr;
    VectorImage* oldVector = nullptr;
    SoundClip* oldClip = nullptr;
    Camera* oldCamera = nullptr;

    bool isFirstRedo = true;

    int type() const override { return REMOVE_KEY_MODIF; }
    void undo() override;
    void redo() override;
    int id() const override { return Id; }

};

class SelectionElement : public BackupElement
{
public:

    enum { Id = 1 };

    SelectionElement(const int backupLayerId,
                     const int backupFrameIndex,
                     const VectorSelection& backupVectorSelection,
                     const SelectionType& backupSelectionType,
                     const QRectF& backupSelectionRect,
                     const qreal& backupRotationAngle,
                     const bool& backupIsSelected,
                     Editor* editor,
                     QUndoCommand* parent = nullptr);

    const int layerId;
    const int frameIndex;

    VectorSelection oldVectorSelection;
    VectorSelection newVectorSelection;

    QRectF oldSelectionRect = QRectF();
    QRectF newSelectionRect = QRectF();

    qreal oldRotationAngle = 0.0;
    qreal newRotationAngle = 0.0;

    bool oldIsSelected = false;
    bool newIsSelected = false;

    SelectionType selectionType;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
    bool mergeWith(const QUndoCommand *other) override;
    int id() const override { return Id; }

    void redoDeselection();
    void redoSelection();
    void undoDeselection();
    void undoSelection();
};

class TransformElement : public BackupElement

{
public:

    enum { Id = 2 };
    TransformElement(const KeyFrame* backupKeyFrame,
                     const int backupLayerId,
                     const DrawOnEmptyFrameAction& backupFrameAction,
                     const QRectF& backupSelectionRect,
                     const QPointF backupTranslation,
                     const qreal backupRotationAngle,
                     const qreal backupScaleX,
                     const qreal backupScaleY,
                     const QPointF backupTransformAnchor,
                     const QString& description,
                     Editor* editor,
                     QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    void apply(const BitmapImage* bitmapImage,
               const VectorImage* vectorImage,
               const QRectF& selectionRect,
               const QPointF translation,
               const qreal rotationAngle,
               const qreal scaleX,
               const qreal scaleY,
               const QPointF selectionAnchor,
               const int layerId);

    int id() const override { return Id; }

    int oldFrameIndex = 0;
    int newFrameIndex = 0;

    QRectF oldSelectionRect;
    QRectF newSelectionRect;

    QPointF oldAnchor;
    QPointF newAnchor;

    QPointF oldTranslation;
    QPointF newTranslation;

    qreal oldScaleX;
    qreal oldScaleY;

    qreal newScaleX;
    qreal newScaleY;

    qreal oldRotationAngle;
    qreal newRotationAngle;


    BitmapImage* oldBitmap = nullptr;
    BitmapImage* newBitmap = nullptr;

    VectorImage* oldVector = nullptr;
    VectorImage* newVector = nullptr;

    int oldLayerId = 0;
    int newLayerId = 0;

    bool isFirstRedo = true;
};

class QProgressDialog;
class ImportBitmapElement : public BackupElement
{

public:
    enum { Id = 7 };

    ImportBitmapElement(const std::map<int, KeyFrame*, std::greater<int> >& backupCanvasKeyFrames,
                        const std::map<int, KeyFrame*, std::less<int> >& backupImportedKeyFrames,
                        const int& backupLayerId,
                        Editor* editor,
                        QUndoCommand* parent = nullptr);

    std::map<int, KeyFrame*, std::greater<int>> oldKeyFrames;
    std::map<int, KeyFrame*,std::less<int>> importedKeyFrames;

    int oldLayerId = 0;
    int newLayerId = 0;

    bool isFirstRedo = true;

    QProgressDialog* progress = nullptr;

    void undo() override;
    void redo() override;
    bool mergeWith(const QUndoCommand *other) override;
    int id() const override { return Id; }
};

class CameraMotionElement : public BackupElement
{
public:

    enum { Id = 3 };
    CameraMotionElement(const int backupFrameIndex,
                        const int backupLayerId,
                        const QPointF& backupTranslation,
                        const float backupRotation,
                        const float backupScale,
                        const QString& description,
                        Editor* editor,
                        QUndoCommand* parent = nullptr);


    QPointF oldTranslation = QPointF(0,0);
    float  oldRotation = 0.0f;
    float oldScale = 0.0f;

    QPointF newTranslation = QPointF(0,0);
    float  newRotation = 0.0f;
    float newScale = 0.0f;

    int frameIndex = 0;
    int layerId = 0;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
    bool mergeWith(const QUndoCommand *other) override;
    int id() const override { return Id; }
};

class AddLayerElement : public BackupElement
{
public:
    AddLayerElement(Layer* backupLayer,
                    Editor* editor,
                    QUndoCommand* parent = nullptr);

    Layer* oldLayer;
    Layer* newLayer;

    QString newLayerName;

    Layer::LAYER_TYPE newLayerType = Layer::UNDEFINED;

    int newLayerId = 0;
    int oldLayerId = 0;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
};

class DeleteLayerElement : public BackupElement
{
public:
    DeleteLayerElement(const QString& backupLayerName,
                       const Layer::LAYER_TYPE& backupType,
                       const std::map<int, KeyFrame*, std::greater<int> >&,
                       const int& backupFrameIndex,
                       const int& backupLayerIndex,
                       const int& backupLayerId,
                       Editor* editor,
                       QUndoCommand* parent = nullptr);

    std::map<int, KeyFrame*, std::greater<int> >oldLayerKeys;

    QString oldLayerName;

    Layer::LAYER_TYPE oldLayerType;

    int oldLayerIndex = 0;
    int oldFrameIndex = 0;
    int oldLayerId = 0;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
};

class RenameLayerElement : public BackupElement
{
public:
    RenameLayerElement(const QString& backupLayerName,
                       const int& backupLayerId,
                       Editor* editor,
                       QUndoCommand* parent = nullptr);

    QString oldLayerName;
    QString newLayerName;

    int oldLayerIndex = 0;
    int newLayerIndex = 0;

    int oldLayerId = 0;
    int newLayerId = 0;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
};

class CameraPropertiesElement : public BackupElement
{
public:
    CameraPropertiesElement(const QString& backupLayerName,
                            const QRect& backupViewRect,
                            const int& backupLayerId,
                            Editor* editor,
                            QUndoCommand* parent = nullptr);

    QString oldLayerName;
    QString newLayerName;

    int oldLayerIndex = 0;
    int newLayerIndex = 0;

    QRect oldViewRect;
    QRect newViewRect;

    int oldLayerId = 0;
    int newLayerId = 0;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
};

class MoveFramesElement : public BackupElement
{
public:

    MoveFramesElement(const int backupLayerId,
                      const int backupScrubberFrameIndex,
                      const int backupOffset,
                      const bool wasSelected,
                      const QList<int> selectedFrameIndexes,
                      Editor* editor,
                      QUndoCommand* parent = nullptr);

    int layerId = 0;

    const int offset;

    QList<int> oldSelectedFrameIndexes;
    QList<int> newSelectedFrameIndexes;

    int scrubberIndex = 0;

    bool isFirstRedo = true;
    bool framesSelected = false;

    void undo() override;
    void redo() override;
    void applyToSingle(Layer* layer, const int oldFrameIndex, const int newFrameIndex);
    void applyToMulti(Layer* layer, const int offset, const QList<int> selectedFrameIndexes);
};

//class SelectFramesElement : public BackupElement
//{
//public:
//    enum { Id = 8 };
//    SelectFramesElement(const SelectionType selectionType,
//                        const int backupLayerId,
//                        const int backupFrameIndex,
//                        const QList<int> backupFrameIndexes,
//                        const QList<int> backupChangedSelectedIndexes,
//                        const bool backupIsFrameSelected,
//                        Editor* editor,
//                        QUndoCommand* parent = nullptr);

//    int oldLayerId;
//    int newLayerId;

//    int frameIndex;
//    bool oldIsSelected;
//    bool isFirstRedo = true;

//    QList<int> oldFrameIndexes;
//    QList<int> newFrameIndexes;

//    QList<int> oldChangedIndexes;
//    QList<int> newChangedIndexes;
//    SelectionType selectionType;

//    bool moreFramesSelected = false;

//    void undo() override;
//    void redo() override;
//    bool mergeWith(const QUndoCommand *other) override;
//    int id() const override { return Id; };

//private:
//    QList<int> getUniqueFrames(const QList<int> frameIndexes, const QList<int> compareIndxes);
//    void apply(const bool moreFramesSelected,
//          const int layerId,
//          const QList<int> additionalFramesIndexes,
//          const QList<int> oldFrameIndexes,
//          const QList<int> newFrameIndexes,
//          const SelectionType& selectionType);
//};

class FlipViewElement : public BackupElement
{
public:
    FlipViewElement(const bool& backupFlipEnabled,
                    const Direction& backupFlipDirection,
                    Editor* editor,
                    QUndoCommand* parent = nullptr);

    bool isFlipped = false;

    Direction direction;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
};

class MoveLayerElement : public BackupElement
{

public:
    MoveLayerElement(const int& backupOldLayerIndex,
                     const int& backupNewLayerIndex,
                     Editor* editor,
                     QUndoCommand* parent = nullptr);

    int oldLayerIndex = 0;
    int newLayerIndex = 0;

    bool isFirstRedo = true;

    void undo() override;
    void redo() override;
};

#endif // BACKUPELEMENT_H
