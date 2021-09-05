#ifndef MPBRUSHMANAGER_H
#define MPBRUSHMANAGER_H

#include "basemanager.h"
#include "pencildef.h"

#include "QJsonDocument"
#include <QJsonParseError>
#include <QJsonParseError>

#include "brushchanges.h"
#include "mpbrushutils.h"

class MPBrushManager : public BaseManager
{
    Q_OBJECT
public:
    MPBrushManager(Editor* editor);

    bool init() override;

    Status load(Object *o) override;
    Status save(Object *o) override;

    Status applyChangesToBrushFile(bool flush);
    Status loadPresets();

    QVector<MPBrushPreset> presets() { return mBrushPresets; }

    QString brushesPath() { return mBrushesPath; }

    QString currentPresetName() { return mCurrentPresetName; }
    QString currentBrushName() { return mCurrentBrushName; }
    QByteArray currentBrushData() { return mCurrentBrushData; }
    Status resetCurrentBrush();

    void setCurrentPresetName(const QString& name) { mCurrentPresetName = name; }

    Status readBrushFromCurrentPreset(const QString& brushName);
    Status readBrushFromFile(const QString& brushPreset, const QString& brushName);
    Status writeBrushToFile(const QString& brushPreset, const QString& brushName, const QByteArray& data);
    Status renameMoveBrushFileIfNeeded(QString originalPreset, QString originalName, QString newPreset, QString newName);
    Status copyRenameBrushFileIfNeeded(const QString& originalPreset, const QString& originalName, const QString& newPreset, QString& newName);
    Status writeBrushIcon(const QPixmap& iconPix, const QString brushPreset, const QString brushName);

    QString getBrushPath(const QString& brushPreset, const QString& brushName, const QString& extension);
    QString getBrushConfigPath(const QString extension = "");
    QString getBrushImagePath(const QString& brushPreset, const QString& brushName);
    QString getBrushPreviewImagePath(const QString& brushPreset, const QString brushName);

    QVector<MPBrushPreset> parseConfig(QFile& file, QString brushesPath);

    void removeBrushInputMapping(BrushSettingType settingType, BrushInputType inputType);
    void backupBrushInputChanges(BrushSettingType settingType, BrushInputType inputType, QVector<QPointF> mappingPoints);
    void backupInitialBrushSettingChanges(BrushSettingType settingType);
    void backupInitialBrushInputMappingChanges(BrushSettingType settingType, BrushInputType inputType);
    void backupBrushSettingChanges(BrushSettingType settingType, qreal baseValue);
    void discardBrushChanges();
    bool brushModificationsForTool();

    void clearCurrentBrushModifications();

    QString currentToolBrushIdentifier();

    bool brushLoaded() const { return !mCurrentBrushName.isEmpty() && !mCurrentBrushData.isEmpty(); }

Q_SIGNALS:
    void errorFromStatus(Status& status);
    void errorFromTitleMessage(QString title, QString description);

private:
    Status replaceBrushIfNeeded(QString brushPath);
    Status copyResourcesToAppData();
    QJsonDocument writeModifications(const QJsonDocument& doc, QJsonParseError& error, QHash<BrushSettingType, BrushChanges> modifications);

    QVector<MPBrushPreset> mBrushPresets;
    QString mBrushesPath;

    QString mCurrentPresetName = "";
    QString mCurrentBrushName = "";
    QString mCurrentToolName = "";

    QByteArray mCurrentBrushData;

    QHash<QString, QHash<BrushSettingType, BrushChanges>> mBrushModsForTools;
    QHash<QString, QHash<BrushSettingType, BrushChanges>> mOldBrushModsForTools;

    Editor* mEditor = nullptr;
};

#endif // MPBRUSHMANAGER_H
