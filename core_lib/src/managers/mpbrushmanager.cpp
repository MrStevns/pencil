#include "mpbrushmanager.h"

#include <QSettings>
#include "pencildef.h"

#include "editor.h"
#include "mpbrushutils.h"
#include "mpfile.h"

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
    Status st = copyResourcesToAppData();
    mBrushesPath = MPCONF::getBrushesPath();
    return st;
}

Status MPBrushManager::save(Object *o)
{
    Q_UNUSED(o)
    return Status::OK;
}

Status MPBrushManager::loadPresets()
{
    QFile fileOrder(getBrushConfigPath());

    Status st = Status::OK;
    if (fileOrder.open(QIODevice::ReadOnly))
    {
        // TODO: will probably have to create a brush importer
        mBrushPresets = mEditor->brushes()->parseConfig(fileOrder, mBrushesPath);

        if (mBrushPresets.isEmpty() || mBrushPresets.first().allBrushes().isEmpty()) {
            st = Status::FAIL;
            DebugDetails dd;

            dd << "file path: " + fileOrder.fileName();
            st.setTitle(tr("Parse error!"));
            st.setDescription(tr("Not able to parse brush config"));
            st.setDetails(dd);
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
    const QString& brushPath = MPCONF::getBrushesPath() + "/" + brushPreset + "/" + brushName;

    QFile file(brushPath + BRUSH_CONTENT_EXT);

    // satefy measure:
    // if no local brush file exists, look at internal resources
    // local brush files will overwrite the existence of the internal one
    if (!file.exists()) {
        file.setFileName(BRUSH_QRC + "/" + brushPreset + "/" + brushName + BRUSH_CONTENT_EXT);
    }

    // TODO: mypaint v2 uses a new format... either we should update to that or simply mention that the brush is not supported

    Status status = Status::OK;
    if (file.open( QIODevice::ReadOnly ))
    {
        QTextStream stream(&file);

        while (!stream.atEnd()) {
            QString line = stream.readLine();

            // Only libmypaint v1 brushes supported currently.
            if (line.at(0) != "{") {

                DebugDetails details;
                status = Status::FAIL;
                status.setTitle("Mypaint v2 brush format detected");
                status.setDescription("This brush is not compatible with the current brush engine");
                status.setDetails(details);
                return status;
            }
            break;
        }
        // reset position
        stream.seek(0);

        mCurrentBrushData = stream.readAll().toUtf8();
    } else {
        status = Status::FAIL;

        DebugDetails details;

        details << "\n\ntried to get brushes from: " + brushPath;
        status.setDetails(details);
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
            QString relativePath = currentPreset + "/" + brush;
            if (QFileInfo(brushesPath + "/" + relativePath + BRUSH_CONTENT_EXT).isReadable()) {
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
    const QString& groupPath = brushesPath + "/" + brushPreset + "/";
    const QString& brushPath = brushesPath + "/" + brushPreset + "/" + brushName;

    QFile file(brushPath + BRUSH_CONTENT_EXT);

    QDir dir(groupPath);
    if (!dir.exists()) {
        dir.mkpath(groupPath);
    }

    file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);

    if (file.error() == QFile::NoError) {
        file.write(data);
        file.close();

        mCurrentBrushData = data;
        status = Status::OK;
    } else {
        status.setTitle(QObject::tr("Write error:"));
        status.setDescription(file.errorString());
        status = Status::FAIL;

    }
    return status;
}

/// Copy internal brush resources to app data folder
/// This is where brushes will be loaded from in the future.
Status MPBrushManager::copyResourcesToAppData()
{
    QString appDataBrushesPath = MPCONF::getBrushesPath();
    QDir dir(appDataBrushesPath);

    // Brush folder exists, no need to copy resources again
    Status st = Status::OK;
    if (dir.exists()) {
        return Status::OK;
    } else {
        bool success = dir.mkpath(appDataBrushesPath);

        if (!success) {
            st = Status::FAIL;
            st.setTitle(QObject::tr("Folder creation failed"));
            st.setDescription(QObject::tr("Creating brushes folder failed"));
            return st;
        }
    }

    dir.setPath(BRUSH_QRC);

    QString internalBrushConfigPath = BRUSH_QRC + "/" + BrushConfigFile;

    QFile appDataFile(appDataBrushesPath+"/"+BrushConfigFile);
    if (!appDataFile.exists()) {
        QFile resFile(internalBrushConfigPath);
        bool success = resFile.copy(appDataFile.fileName());

        if (!success) {
            st = Status::FAIL;
            st.setTitle(QObject::tr("Copy failure"));
            st.setDescription(QObject::tr("The file: ") + resFile.fileName() +
                              QObject::tr(" couldn't be copied to:" ) + appDataFile.fileName() +
                              QObject::tr("\nThe following error was given:") + resFile.errorString());
            return st;
        }

        // make sure file has read and write access
        appDataFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    QStringList content = dir.entryList();

    for (QString entry : content) {

        // Ignore config file, we've already copied it
        if (entry.contains(".conf")) {
            continue;
        }

        QDir internalBrushDir(BRUSH_QRC + "/" + entry);
        QDir externalBrushDir(appDataBrushesPath + "/" + entry);
        if (!externalBrushDir.exists()) {
            bool success = externalBrushDir.mkpath(appDataBrushesPath + "/" + entry);

            if (!success) {
                st = Status::FAIL;
                st.setTitle(QObject::tr("Folder creation failed"));
                st.setDescription(QObject::tr("Creating folder for: ") + entry + QObject::tr("failed"));
                return st;
            }
        }

        QStringList dirContent = internalBrushDir.entryList();
        for (QString entryDown : dirContent) {
            QFile internalBrushFile(internalBrushDir.path() + "/" +entryDown);
            QFile brushFile(externalBrushDir.path() + "/" + entryDown);

            if (!brushFile.exists()) {
                bool success = internalBrushFile.copy(brushFile.fileName());

                if (!success) {
                    st = Status::FAIL;
                    st.setTitle(QObject::tr("Copy failure"));
                    st.setDescription(QObject::tr("The file: ") + brushFile.fileName()+
                                      QObject::tr(" couldn't be copied to:" ) + internalBrushFile.fileName() +
                                      QObject::tr("\nThe following error was given:") + internalBrushFile.errorString());
                    return st;
                }
                brushFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
            }
        }
    }

    return st;
}


Status MPBrushManager::renameMoveBrushFileIfNeeded(QString originalPreset, QString originalName, QString newPreset, QString newName)
{
    MPFile mpFile(getBrushPath(originalPreset,originalName,BRUSH_CONTENT_EXT));
    return mpFile.renameBrush(getBrushPath(newPreset, newName, BRUSH_CONTENT_EXT));
}

Status MPBrushManager::copyRenameBrushFileIfNeeded(const QString& originalPreset, const QString& originalName, const QString& newPreset, QString& newName)
{
    MPFile mpFile(getBrushPath(originalPreset,originalName,BRUSH_CONTENT_EXT));
    return mpFile.copyBrush(getBrushPath(newPreset, newName, BRUSH_CONTENT_EXT));
}

QString MPBrushManager::getBrushPreviewImagePath(const QString& brushPreset, const QString brushName)
{
    const QString& brushPath = MPCONF::getBrushesPath() + "/" + brushPreset + "/" + brushName;
    QFile file(brushPath+BRUSH_PREVIEW_EXT);

    if (file.exists()) {
        return file.fileName();
    }
    return "";
}

QString MPBrushManager::getBrushPath(const QString& brushPreset, const QString& brushName, const QString& extension)
{
    QString brushPath = MPCONF::getBrushesPath() + "/" + brushPreset + "/" + brushName;

    QFile file(brushPath+extension);
    // TODO: fix this.. we can't do this when the logic of mpfile expects a filepath.
    // maybe this shouldn't be done at all...
//    if (!file.exists()) {
//        brushPath = QString(BRUSH_QRC) + "/" + brushPreset + "/" + brushName;
//    }
    return brushPath + extension;
}

QString MPBrushManager::getBrushImagePath(const QString& brushPreset, const QString& brushName)
{
    return getBrushPath(brushPreset, brushName, BRUSH_PREVIEW_EXT);
}

QString MPBrushManager::getBrushConfigPath()
{
    QString brushPath = MPCONF::getBrushesPath() + "/" + BrushConfigFile;

    QFile file(brushPath);
    if (!file.exists()) {
        brushPath = BRUSH_QRC + "/" + BrushConfigFile;
    }
    return brushPath;
}

Status MPBrushManager::writeBrushIcon(const QPixmap& iconPix, const QString brushPreset, const QString brushName) 
{
    return MPFile(getBrushPath(brushPreset,brushName,BRUSH_CONTENT_EXT)).updateBrushIcon(iconPix);
}
