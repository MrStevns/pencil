#ifndef MPBRUSHMANAGER_H
#define MPBRUSHMANAGER_H

#include "basemanager.h"

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

    Status applyChangesToBrushFile(QHash<int, BrushChanges> changes);
    Status loadPresets();

    QVector<MPBrushPreset> presets() { return mBrushPresets; }

    QString brushesPath() { return mBrushesPath; }

    QString currentPresetName() { return mCurrentPresetName; }
    QString currentBrushName() { return mCurrentBrushName; }
    QByteArray currentBrushData() { return mCurrentBrushData; }

    void setCurrentPresetName(const QString& name) { mCurrentPresetName = name; }

    Status readBrushFromCurrentPreset(const QString& brushName);
    Status readBrushFromFile(const QString& brushPreset, const QString& brushName);
    Status writeBrushToFile(const QString& brushPreset, const QString& brushName, const QByteArray& data);

    QVector<MPBrushPreset> parseConfig(QFile& file, QString brushesPath);

Q_SIGNALS:
    void errorFromStatus(Status& status);
    void errorFromTitleMessage(QString title, QString description);

private:

    QJsonDocument writeModifications(const QJsonDocument& doc, QJsonParseError& error, QHash<int, BrushChanges> modifications);

    QVector<MPBrushPreset> mBrushPresets;
    QString mBrushesPath;

    QString mCurrentPresetName;
    QString mCurrentBrushName;
    QString mCurrentToolName;

    QByteArray mCurrentBrushData;

    Editor* mEditor = nullptr;
};

#endif // MPBRUSHMANAGER_H
