#include "mpbrushmanager.h"

#include <QSettings>
#include "pencildef.h"

#include "editor.h"
#include "mpbrushutils.h"
#include <QMessageBox>

MPBrushManager::MPBrushManager(Editor* editor) : BaseManager(editor)
{
    mEditor = editor;
}

bool MPBrushManager::init()
{
    return true;
}

Status MPBrushManager::load(Object*)
{
    Status st = MPBrushParser::copyResourcesToAppData();
    mBrushesPath = MPCONF::getBrushesPath();

    if (st.fail()) {
        // TODO: emit failure to UI
//        emit()
    }
    return Status::OK;
}

Status MPBrushManager::save(Object *o)
{
    Q_UNUSED(o)
    return Status::OK;
}

Status MPBrushManager::loadPresets()
{
    QFile fileOrder(MPBrushParser::getBrushConfigPath(BrushConfigFile));

    Status st = Status::OK;
    if (fileOrder.open(QIODevice::ReadOnly))
    {
        // TODO: will probably have to create a brush importer
        mBrushPresets = mEditor->brushes()->parseConfig(fileOrder, mBrushesPath);

        if (mBrushPresets.isEmpty() || mBrushPresets.first().allBrushes().isEmpty()) {
            st = Status::FAIL;
            st.setTitle(tr("Parse error!"));
            st.setDescription(tr("Not able to parse brush config"));
        }

        QSettings settings(PENCIL2D,PENCIL2D);
        QString lastPreset = settings.value(SETTING_MPBRUSHPRESET).toString();

        if (lastPreset.isEmpty()) {
            mCurrentPresetName = mBrushPresets.first().name;
            settings.setValue(SETTING_MPBRUSHPRESET, mCurrentPresetName);
        } else {
            mCurrentPresetName = lastPreset;
        }
    }

    return st;
}

Status MPBrushManager::readBrushFromCurrentPreset(const QString& brushName)
{
    auto status = readBrushFromFile(mCurrentPresetName, brushName);
    mCurrentBrushName = brushName;
    return status;
}

Status MPBrushManager::applyChangesToBrushFile(QHash<int, BrushChanges> changes)
{
    Status status = readBrushFromFile(mCurrentPresetName, mCurrentBrushName);

    if (status != Status::OK) {
        return status;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(mCurrentBrushData, &error);

    doc = writeModifications(doc, error, changes);

    Status statusWrite = writeBrushToFile(mCurrentPresetName, mCurrentBrushName, doc.toJson());

    return statusWrite;
}

QJsonDocument MPBrushManager::writeModifications(const QJsonDocument& doc, QJsonParseError& error, QHash<int, BrushChanges> modifications)
{
    QJsonDocument document = doc;
    QJsonObject rootObject = document.object();

    if (error.error != QJsonParseError::NoError) {

//        QMessageBox::information(this, tr("Parse error!"), tr("Could not save brush file\n the following error was given: ") + error.errorString());
//        emit errorFromTitleMessage(tr("Parse error!"), tr("Could not save brush file\n the following error was given: ") + error.errorString());
        return document;
    }

    QJsonObject::iterator settingsContainerObjIt = rootObject.find("settings");

    if (settingsContainerObjIt->isUndefined()) {
//        emit errorFromTitleMessage(tr("Parse error!"), tr("Looks like you are missign a 'settings' field in your brush file, this shouldn't happen...") + error.errorString());
        return document;
    }
    QJsonValueRef settingsContainerRef = settingsContainerObjIt.value();

    QJsonObject settingsContainerObj = settingsContainerRef.toObject();
    QHashIterator<int, BrushChanges> settingIt(modifications);
    while (settingIt.hasNext()) {
        settingIt.next();

        BrushChanges brushChanges = settingIt.value();
        QString settingId = MPBrushParser::getBrushSettingIdentifier(brushChanges.settingsType);

        QJsonObject::iterator settingObjIt = settingsContainerObj.find(settingId);

        if (settingObjIt->isUndefined()) {
            QJsonObject settingObj;
            brushChanges.write(settingObj);
            settingsContainerObj.insert(settingId, settingObj);
        } else {
            QJsonValueRef settingRef = settingObjIt.value();
            QJsonObject settingObj = settingRef.toObject();
            brushChanges.write(settingObj);

            settingsContainerObj.remove(settingId);
            settingsContainerObj.insert(settingId, settingObj);
        }
    }
    settingsContainerRef = settingsContainerObj;
    document.setObject(rootObject);

    return document;
}

Status MPBrushManager::readBrushFromFile(const QString& brushPreset, const QString& brushName)
{
    const QString& brushPath = MPCONF::getBrushesPath() + QDir::separator() + brushPreset + QDir::separator() + brushName;

    QFile file(brushPath + BRUSH_CONTENT_EXT);

    // satefy measure:
    // if no local brush file exists, look at internal resources
    // local brush files will overwrite the existence of the internal one
    if (!file.exists()) {
        file.setFileName(BRUSH_QRC + QDir::separator() + brushPreset + QDir::separator() + brushName + BRUSH_CONTENT_EXT);
    }

    Status status = Status::OK;
    if (file.open( QIODevice::ReadOnly ))
    {
        mCurrentBrushData = file.readAll();
    } else {
        status = Status::FAIL;
    }
    return status;
}

/// Parses the mypaint brush config ".conf" format and returns a map of the brush groups
QVector<MPBrushPreset> MPBrushManager::parseConfig(QFile& file, QString brushesPath)
{
    MPBrushPreset brushesForPreset;
    QString currentTool;
    QString currentPreset;
    QStringList brushList;

    QVector<MPBrushPreset> brushPresets;

    int presetIndex = 0;
    while (!file.atEnd())
    {
        QString line ( file.readLine().trimmed() );
        if (line.isEmpty() || line.startsWith("#")) continue;

        if (MPCONF::isPresetToken(line))
        {
            if (!brushesForPreset.isEmpty() && !currentTool.isEmpty()) {

                brushesForPreset.insert(currentTool, brushList);

                brushPresets[presetIndex] = brushesForPreset;
                presetIndex++;
            }
            currentPreset = MPCONF::getValue(line);
            brushesForPreset.name = currentPreset;

            brushesForPreset.clear();
            brushList.clear();

            brushPresets.append(brushesForPreset);
            continue;
        }

        if (MPCONF::isToolToken(line))
        {

            if (!currentTool.isEmpty()) {
                brushesForPreset.insert(currentTool, brushList);
            }

            currentTool = MPCONF::getValue(line);
            brushList.clear();
            continue;
        }

        if (MPCONF::isBrushToken(line)) {

            QString brush = MPCONF::getValue(line);
            QString relativePath = currentPreset + QDir::separator() + brush;
            if (QFileInfo(brushesPath + QDir::separator() + relativePath + BRUSH_CONTENT_EXT).isReadable()) {
                brushList << brush;
            }
            continue;
        }

        if (!currentTool.isEmpty() && !brushPresets.isEmpty()) {
            brushPresets.append(brushesForPreset);
        }
    }

    if (!brushesForPreset.isEmpty() && !currentTool.isEmpty()) {

        brushesForPreset.insert(currentTool, brushList);

        Q_ASSUME(presetIndex <= brushPresets.size());

        brushPresets[presetIndex] = brushesForPreset;
        presetIndex++;
    }

    return brushPresets;
}

Status MPBrushManager::writeBrushToFile(const QString& brushPreset, const QString& brushName, const QByteArray& data)
{
    Status status = Status::OK;
    QString brushesPath = MPCONF::getBrushesPath();
    const QString& groupPath = brushesPath + QDir::separator() + brushPreset + QDir::separator();
    const QString& brushPath = brushesPath + QDir::separator() + brushPreset + QDir::separator() + brushName;

    QFile file(brushPath + BRUSH_CONTENT_EXT);

    QDir dir(groupPath);
    if (!dir.exists()) {
        dir.mkpath(groupPath);
    }

    file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);

    if (file.error() == QFile::NoError) {
        file.write(data);
        file.close();

        status = Status::OK;
    } else {
        status.setTitle(QObject::tr("Write error:"));
        status.setDescription(file.errorString());
        status = Status::FAIL;

    }
    return status;
}
