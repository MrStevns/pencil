#ifndef BACKUPMANAGER_H
#define BACKUPMANAGER_H

#include <QUndoStack>
#include <QUndoView>
#include "basemanager.h"
#include "preferencemanager.h"
#include "layer.h"
#include "direction.h"
#include "movemode.h"
#include "vectorselection.h"

#include <QAction>

class BitmapImage;
class VectorImage;
class Camera;
class SoundClip;
class KeyFrame;
class LegacyBackupElement;
class BackupElement;

class BackupManager : public BaseManager
{
    Q_OBJECT

    friend class RemoveKeyFrameElement;
    friend class AddKeyFrameElement;
    friend class DeleteLayerElement;

public:
    explicit BackupManager(Editor* editor);
    ~BackupManager();

    bool init() override;
    Status load(Object*) override;
    Status save(Object*) override;

    bool hasUnsavedChanges() const;

    void keyAdded(const int& keySpacing, const bool& keyExisted, const QString& description);
    void keyAdded(const QString& description = "");
    void keyRemoved();
    void bitmap(const QString& description);
    void vector(const QString& description);
    void cameraMotion(const QString& description = "");
    void layerAdded();
    void layerDeleted(const std::map<int, KeyFrame*, std::greater<int>>& oldKeys);
    void layerRenamed();
    void layerMoved(const int& backupNewLayerIndex);

    void importBitmap(const std::map<int, KeyFrame*, std::greater<int>>& canvasKeys,
                      const std::map<int, KeyFrame*, std::less<int>>& importedKeys);
    void selection();
    void deselect();
    void transform(const QString& description);
    void cameraProperties(const QRect& backupViewRect);
    void frameMoved(const int offset);
    void framesMoved(const int offset,
                     const int scrubberFrameIndex);

    void frameSelected(const QList<int> newSelectedIndexes, const int frameIndex, const bool isSelected);
    void frameDeselected(const QList<int> newDeselectedIndexes, const int frameIndex);
    void frameDeselected(const int frameIndex);
    void flipView(const bool& backupIsFlipped, const Direction& backupFlipDirection);
    void toggleSetting(bool backupToggleState, const SETTING& backupType);
    void saveStates();

    void restoreKey(const int& layerId, const int& frame, KeyFrame* keyFrame);

    static int getActiveFrameIndex(Layer* layer, const int frameIndex, const DrawOnEmptyFrameAction& frameAction);

    // const BackupElement* currentBackup();

    QUndoStack* undoStack() { return mUndoStack; }

    QAction* createUndoAction(QObject* parent, const QString& description, const QIcon& icon);
    QAction* createRedoAction(QObject* parent, const QString& description, const QIcon& icon);

    void updateUndoAction(QAction* undoAction);
    void updateRedoAction(QAction* redoAction);

    void clearStack();

    // Legacy System

    void backup(const QString& undoText);
    bool backup(int backupLayer, int backupFrame, const QString& undoText);
    void sanitizeLegacyBackupElementsAfterLayerDeletion(int layerIndex);
    void restoreLegacyKey();

    void rememberLastModifiedFrame(int layerNumber, int frameNumber);

Q_SIGNALS:
    void updateBackup();

private:

    void legacyUndo();
    void legacyRedo();

    bool newUndoRedoSystemEnabled() const;

    void restoreKey(const BackupElement* element);
    void restoreLayerKeys(const BackupElement* element);

    QUndoStack* mUndoStack;

    int mLayerId = 0;
    int mFrameIndex = 0;
    int mLayerIndex = 0;

    qreal mViewRotation = 0;
    qreal mViewScale = 0;
    QPointF mViewTranslation;

    QString mLayerName;

    Layer* mLayer = nullptr;
    BitmapImage* mBitmap = nullptr;
    VectorImage* mVector = nullptr;
    SoundClip* mClip = nullptr;
    Camera* mCamera = nullptr;
    KeyFrame* mKeyframe = nullptr;

    QList<int> mFrameIndexes;

    bool mIsSelected = false;

    VectorSelection mVectorSelection;

    QRectF mSelectionRect = QRectF();
    qreal mSelectionRotationAngle = 0.0;
    qreal mSelectionScaleX = 0.0;
    qreal mSelectionScaleY = 0.0;
    QPointF mSelectionTranslation;
    QPointF mSelectionAnchor;

    MoveMode mMoveMode;

    Layer::LAYER_TYPE mLayerType;

    DrawOnEmptyFrameAction mEmptyFrameSettingVal;
    const BackupElement* mBackupAtSave = nullptr;

    // Legacy system
    int mLegacyBackupIndex;
    LegacyBackupElement* mLegacyBackupAtSave = nullptr;
    QList<LegacyBackupElement*> mLegacyBackupList;

    int mLegacyLastModifiedLayer = -1;
    int mLegacyLastModifiedFrame = -1;

    bool mNewBackupSystemEnabled = false;

};

#endif // BACKUPMANAGER_H
