#include "mpbrushmanager.h"

#include "pencildef.h"
#include "basetool.h"

#include "editor.h"
#include "mpbrushutils.h"
#include <QMessageBox>

MPBrushManager::MPBrushManager(Editor* editor) : BaseManager(editor, __FUNCTION__)
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

void MPBrushManager::brushPreferences(std::function<void(QSettings&)> callback)
{
    QSettings settings(PENCIL2D, PENCIL2D);
    settings.beginGroup(MPBRUSH_GROUP);
    callback(settings);
    settings.endGroup();
}

Status MPBrushManager::loadPresets()
{
    QFile fileOrder(getBrushConfigPath(BrushConfigFile));

    Status st = Status::OK;
    if (fileOrder.open(QIODevice::ReadOnly))
    {
        // TODO: will probably have to create a brush importer
        mBrushPresets = parseConfig(fileOrder, mBrushesPath);

        if (mBrushPresets.isEmpty() || mBrushPresets.first().allBrushes().isEmpty()) {
            st = Status::FAIL;
            DebugDetails dd;

            dd << "file path: " + fileOrder.fileName();
            st.setTitle(tr("Parse error!"));
            st.setDescription(tr("Not able to parse brush config"));
            st.setDetails(dd);
        }

        brushPreferences( [=] (QSettings& settings)
        {
            QString lastPreset = settings.value(SETTING_MPBRUSHPRESET).toString();
            if (lastPreset.isEmpty()) {
                mCurrentPresetName = mBrushPresets.constFirst().name;
                settings.setValue(SETTING_MPBRUSHPRESET, mCurrentPresetName);
            } else {
                mCurrentPresetName = lastPreset;
            }
        });
    }

    return st;
}

/// Resets all settings for the currently selected brush.
/// Does not replace brush icon
Status MPBrushManager::resetCurrentBrush()
{
    const QString& brushPath = MPCONF::getBrushesPath() + "/" + mCurrentPresetName + "/" + mCurrentBrushName + BRUSH_CONTENT_EXT;
    auto status = replaceBrushIfNeeded(brushPath);

    if (status.fail()) { return status; }

    return readBrushFromCurrentPreset(mCurrentBrushName);
}

Status MPBrushManager::readBrushFromCurrentPreset(const QString& brushName)
{
    auto status = readBrushFromFile(mCurrentPresetName, brushName);
    mCurrentBrushName = brushName;
    mCurrentToolName = mEditor->currentTool()->typeName();
    return status;
}

QString MPBrushManager::currentToolBrushIdentifier() const
{
    return QString(SETTING_MPBRUSHSETTING)
            .arg(mCurrentToolName)
            .arg(mCurrentPresetName)
            .arg(mCurrentBrushName).toLower();
}

Status MPBrushManager::applyChangesToBrushFile(bool flush)
{
    Status status = readBrushFromFile(mCurrentPresetName, mCurrentBrushName);

    if (status != Status::OK) {
        return status;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(mCurrentBrushData, &error);

    QString toolBrushIdentifier = currentToolBrushIdentifier();
    auto modificationHash = mBrushModsForTools.find(toolBrushIdentifier);
    if (flush) {
        modificationHash = mOldBrushModsForTools.find(toolBrushIdentifier);
        Q_ASSERT(!modificationHash->isEmpty());
    }

    doc = writeModifications(doc, error, modificationHash.value());

    Status statusWrite = writeBrushToFile(mCurrentPresetName, mCurrentBrushName, doc.toJson());

    return statusWrite;
}

void MPBrushManager::discardBrushChanges()
{
    QHashIterator<BrushSettingType, BrushChanges> changesIt(mOldBrushModsForTools.find(currentToolBrushIdentifier()).value());
    while (changesIt.hasNext()) {
        changesIt.next();

        mEditor->setMPBrushSettingBaseValue(static_cast<BrushSettingType>(changesIt.key()),
                                   static_cast<float>(changesIt.value().baseValue));

        const auto brushChanges = changesIt.value();
        QHashIterator<BrushInputType, InputChanges> inputIt(brushChanges.listOfinputChanges);
        while (inputIt.hasNext()) {
            inputIt.next();

            const auto inputChanges = inputIt.value();
            mEditor->setBrushInputMapping(inputChanges.mappedPoints, brushChanges.settingsType, inputChanges.inputType);
        }
    }
}

void MPBrushManager::backupBrushInputChanges(BrushSettingType settingType, BrushInputType inputType, QVector<QPointF> mappingPoints)
{

    backupInitialBrushInputMappingChanges(settingType, inputType);
    mBrushModsForTools = mOldBrushModsForTools;

    auto toolHashIt = mBrushModsForTools.constFind(currentToolBrushIdentifier());
    auto toolHash = mBrushModsForTools;

    if (toolHashIt != toolHash.constEnd()) {

        auto settingHash = toolHashIt.value();
        auto settingHashIt = settingHash.constFind(settingType);

        if (settingHashIt != settingHash.constEnd()) {
            auto inputHash = settingHashIt.value().listOfinputChanges;
            auto inputHashIt = inputHash.constFind(inputType);

            if (inputHashIt != inputHash.constEnd()) {
                BrushChanges changes = settingHashIt.value();
                changes.listOfinputChanges.insert(inputType, InputChanges { mappingPoints, inputType });
                mBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType, changes) } );
            }
        }
    } else {

        // Setting already exists, make sure we're adding a new input
        QHashIterator<BrushSettingType, BrushChanges> oldIt(toolHashIt.value());

        while (oldIt.hasNext()) {
            oldIt.next();

            auto brushChanges = oldIt.value();
            auto listOfInputChanges = brushChanges.listOfinputChanges;
            QHashIterator<BrushInputType, InputChanges> inputIt(listOfInputChanges);
            while (inputIt.hasNext()) {
                inputIt.next();
                auto inputChanges = inputIt.value();

                if (inputChanges.inputType != inputType) {
                    auto points = mappingPoints;
                    brushChanges.listOfinputChanges.insert(inputType, InputChanges { points, inputType });
                    mBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType, brushChanges) } );
                }
            }
        }
    }
}

void MPBrushManager::backupInitialBrushInputMappingChanges(BrushSettingType settingType, BrushInputType inputType)
{
    auto toolHashIt = mOldBrushModsForTools.constFind(currentToolBrushIdentifier());
    auto toolHash = mOldBrushModsForTools;

    if (toolHashIt != toolHash.constEnd()) {

        auto settingHash = toolHashIt.value();
        auto settingHashIt = settingHash.constFind(settingType);

        if (settingHashIt != settingHash.constEnd()) {
            auto inputHash = settingHashIt.value().listOfinputChanges;
            auto inputHashIt = inputHash.constFind(inputType);

            if (inputHashIt != inputHash.constEnd() && inputHash.contains(inputType)) { return; }
        }
    }

    auto mappingInput = mEditor->getBrushInputMapping(settingType, inputType);

    // if no base value has been provided, use the default value from brush settings
    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(settingType));

    BrushChanges changes;
    changes.baseValue = baseValue;
    changes.settingsType = settingType;
    auto mappedInputs = mappingInput.controlPoints.points;

    changes.listOfinputChanges.insert(inputType, InputChanges { mappedInputs, inputType });

    mOldBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType,changes) } );
}

void MPBrushManager::backupInitialBrushSettingChanges(BrushSettingType settingType)
{
    // Adds old brush value and only save once, so we can discard it later if needed
    auto toolHashIt = mOldBrushModsForTools.constFind(currentToolBrushIdentifier());
    auto toolHash = mOldBrushModsForTools;
    if (toolHashIt != toolHash.constEnd() && toolHashIt->contains(settingType)) { return; }

    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(settingType));

    BrushChanges changes;
    changes.baseValue = baseValue;
    changes.settingsType = settingType;
    mOldBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType, changes) } );
}

void MPBrushManager::backupBrushSettingChanges(BrushSettingType settingType, qreal baseValue)
{
    backupInitialBrushSettingChanges(settingType);

    auto toolHashIt = mBrushModsForTools.constFind(currentToolBrushIdentifier());
    auto toolHash = mBrushModsForTools;
    if (toolHashIt == toolHash.constEnd() || (toolHashIt != toolHash.constEnd() && !toolHashIt->contains(settingType))) {
        BrushChanges changes;
        changes.baseValue = baseValue;
        changes.settingsType = settingType;

        mBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType, changes) } );
    } else {
        QHashIterator<BrushSettingType, BrushChanges> changesHash(toolHashIt.value());
        while (changesHash.hasNext()) {
            changesHash.next();

            BrushChanges innerChanges = changesHash.value();
            if (settingType == innerChanges.settingsType) {
                innerChanges.baseValue = baseValue;
                innerChanges.settingsType = settingType;
                mBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType, innerChanges) } );
                break;
            }
        }
    }
}

void MPBrushManager::removeBrushInputMapping(BrushSettingType settingType, BrushInputType inputType)
{
    auto toolHashIt = mBrushModsForTools.constFind(currentToolBrushIdentifier());
    auto toolHash = mBrushModsForTools;
    Q_ASSERT(toolHashIt != toolHash.constEnd());
    Q_ASSERT(toolHashIt->contains(settingType));

    auto settingHashIt = toolHashIt->find(settingType).value();
    settingHashIt.listOfinputChanges.insert(inputType, InputChanges { {}, inputType, false });

    mBrushModsForTools.insert(currentToolBrushIdentifier(), { std::make_pair(settingType, settingHashIt) });
}

void MPBrushManager::clearCurrentBrushModifications()
{
    mBrushModsForTools.remove(currentToolBrushIdentifier());
    mOldBrushModsForTools.remove(currentToolBrushIdentifier());
}

bool MPBrushManager::brushModificationsForTool() const
{
    return mBrushModsForTools.contains(currentToolBrushIdentifier());
}

QJsonDocument MPBrushManager::writeModifications(const QJsonDocument& doc, QJsonParseError& error, QHash<BrushSettingType, BrushChanges> modifications)
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
    QHashIterator<BrushSettingType, BrushChanges> settingIt(modifications);
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

    Status status = Status::OK;

    bool brushReplaced = false;
    if (file.open(QIODevice::ReadOnly))
    {
        QTextStream stream(&file);

        while (!stream.atEnd()) {
            QString line = stream.readLine();

            // Only libmypaint v1 brushes supported currently.
            if (QString(line.at(0)).compare("{", Qt::CaseInsensitive) != 0) {

                Status status = replaceBrushIfNeeded(file.fileName());
                if (status.fail()) {
                    DebugDetails details;
                    details << "Mypaint v2 brush format detected";
                    details << "Brush is not compatible with the current brush engine";
                    status.setDetails(details);
                    return status;
                } else {
                    brushReplaced = true;
                }
            }
            break;
        }
        // reset position
        stream.seek(0);
        if (!brushReplaced) {
            mCurrentBrushData = stream.readAll().toUtf8();
        }
        file.close();
    }

    if (brushReplaced) {
        // Brush has been replaced, make sure to read the stream again
        if (file.open(QIODevice::ReadOnly))
        {
            QTextStream stream(&file);
            mCurrentBrushData = stream.readAll().toUtf8();
            file.close();
        }
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

        Q_ASSERT(presetIndex <= brushPresets.size());

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
        status = Status::FAIL;
        status.setTitle(QObject::tr("Write error:"));
        status.setDescription(file.errorString());
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
    if (dir.exists() && !dir.isEmpty()) {
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

/// Convenient method to avoid having users to reload their brush library because
/// x brush might be of wrong version
Status MPBrushManager::replaceBrushIfNeeded(QString brushPath)
{
    QString appDataBrushesPath = MPCONF::getBrushesPath();

    QDir presetDir(brushPath);
    presetDir.cdUp();
    QFileInfo brushFileInfo(brushPath);
    QString presetName = presetDir.dirName();
    QString brushFileName = brushFileInfo.fileName();

    QDir internalBrushDir(BRUSH_QRC + "/" + presetName);
    QDir externalBrushDir(appDataBrushesPath + "/" + presetName);
    QFile internalBrushFile(internalBrushDir.path() + "/" + brushFileInfo.fileName());

    QFile externalBrushFile(brushPath);
    bool success = externalBrushFile.remove();
    success = internalBrushFile.copy(brushPath);

    if (!success) {
        Status st = Status::FAIL;
        st.setTitle(QObject::tr("Replace failure"));
        st.setDescription(QObject::tr("Tried to replace: ") + brushPath +
                          QObject::tr(" with:" ) + internalBrushFile.fileName() +
                          QObject::tr("\nbecause the previous brush was incompatible. The following error was given:") + internalBrushFile.errorString());
        return st;
    }
    // Reset file since we've replaced it, otherwise permissions won't be set
    externalBrushFile.reset();
    externalBrushFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    return Status::OK;
}


Status MPBrushManager::renameMoveBrushFileIfNeeded(QString originalPreset, QString originalName, QString newPreset, QString newName)
{
    Status status = Status::OK;
    QString brushesPath = MPCONF::getBrushesPath();
    const QString presetPath = brushesPath + "/" + newPreset;
    const QString brushPath = brushesPath + "/" + newPreset + "/" + newName;
    const QString oldBrushPath = brushesPath + "/" + originalPreset + "/" + originalName;

    QDir presetDir(presetPath);

    QString absoluteOldBrushPath = oldBrushPath + BRUSH_CONTENT_EXT;
    QString absoluteOldBrushPreviewPath = oldBrushPath + BRUSH_PREVIEW_EXT;

    QString absoluteBrushPath = brushPath + BRUSH_CONTENT_EXT;
    QString absoluteBrushPreviewPath = brushPath+ BRUSH_PREVIEW_EXT;
    if (!presetDir.exists()) {
        bool pathCreated = presetDir.mkpath(presetPath);

        if (!pathCreated) {
            status = Status::FAIL;

            status.setTitle(QObject::tr("Something went wrong"));
            status.setDescription(QObject::tr("Couldn't create preset dir, verify that the folder is writable"));
            return status;
        }
    }

    QFile brushFile(absoluteBrushPath);
    if (!brushFile.exists()) {

        QFile moveFile(oldBrushPath);
        moveFile.rename(absoluteOldBrushPath, absoluteBrushPath);

        if (moveFile.error() != QFile::NoError) {
            status = Status::FAIL;

            status.setTitle(QObject::tr("Something went wrong"));
            status.setDescription(QObject::tr("Failed to rename or move: ") + moveFile.fileName() + QObject::tr(" verify that the folder is writable")
                                  + QObject::tr("The following error was given: ") + moveFile.errorString());
            return status;
        }
    }

    QFile brushImageFile(absoluteBrushPreviewPath);
    if (!brushImageFile.exists()) {
        QFile moveImageFile(absoluteOldBrushPreviewPath);
        moveImageFile.rename(absoluteOldBrushPreviewPath, absoluteBrushPreviewPath);

        if (moveImageFile.error() != QFile::NoError) {
            status = Status::FAIL;

            status.setTitle(QObject::tr("Something went wrong"));
            status.setDescription(QObject::tr("Failed to rename or move: ") + moveImageFile.fileName() + QObject::tr(" verify that the folder is writable")
                                  + QObject::tr("The following error was given: ") + moveImageFile.errorString());
            return status;
        }

    }

    return status;
}

Status MPBrushManager::copyRenameBrushFileIfNeeded(const QString& originalPreset, const QString& originalName, const QString& newPreset, QString& newName)
{
    QString brushesPath = MPCONF::getBrushesPath();
    QString presetPath = brushesPath + "/" + newPreset;
    QString brushPath = brushesPath + "/" + newPreset + "/" + newName;

    QString oldPresetPath = brushesPath + "/" + originalPreset;
    QString oldBrushPath = brushesPath + "/" + originalPreset + "/" + originalName;

    Status status = Status::OK;
    QFile file(brushPath + BRUSH_CONTENT_EXT);
    QFile fileImage(brushPath + BRUSH_PREVIEW_EXT);

    QDir dir(presetPath);
    if (!dir.exists()) {
        bool pathCreated = dir.mkpath(presetPath);

        if (!pathCreated) {
            status = Status::FAIL;

            status.setTitle(QObject::tr("Something went wrong"));
            status.setDescription(QObject::tr("Couldn't create dir: ") + dir.path() + QObject::tr("verify that the folder is writable"));
            return status;
        }
    }

    if (file.error() != QFile::NoError || fileImage.error() != QFile::NoError) {
        status = Status::FAIL;
        status.setTitle(QObject::tr("File error"));
        status.setDescription(QObject::tr("Failed to read files: ") + file.errorString() + " " + fileImage.errorString());
    } else {

        if (!file.exists() && !fileImage.exists()) {
            QFile fileToCopy(oldBrushPath+BRUSH_CONTENT_EXT);
            fileToCopy.copy(oldBrushPath+BRUSH_CONTENT_EXT, brushPath+BRUSH_CONTENT_EXT);

            if (fileToCopy.error() != QFile::NoError) {
                status = Status::FAIL;
                status.setTitle("Error: Copy file");
                status.setDescription(QObject::tr("Failed to copy: ") + fileToCopy.fileName() + ", the folder error was given: "
                                      + fileToCopy.errorString());

                return status;
            }

            fileToCopy.setFileName(oldBrushPath+BRUSH_PREVIEW_EXT);
            fileToCopy.copy(oldBrushPath+BRUSH_PREVIEW_EXT, brushPath+BRUSH_PREVIEW_EXT);

            if (fileToCopy.error() != QFile::NoError) {
                status = Status::FAIL;
                status.setTitle(QObject::tr("Error: Copy file"));
                status.setDescription(QObject::tr("Failed to copy: ") + fileToCopy.fileName() + QObject::tr(", the folder error was given: ")
                                      + fileToCopy.errorString());
                return status;
            }

        } else {

            QString clonePostFix = BRUSH_COPY_POSTFIX;

            int countClones = 0;
            for (int i = 0; i < dir.entryList().count(); i++) {
                QString x = dir.entryList()[i];

                if (x.compare(newName+BRUSH_CONTENT_EXT) == 0) {
                    countClones++;
                }
            }

            QString clonedName = newName;

            if (newName.compare(originalName) == 0) {
                clonedName = clonedName.append(clonePostFix+QString::number(countClones));
            }

            QString clonedPath = oldPresetPath + "/" + clonedName;
            QString newFileName = clonedPath+BRUSH_CONTENT_EXT;
            QString newImageName = clonedPath+BRUSH_PREVIEW_EXT;
            file.copy(newFileName);
            fileImage.copy(newImageName);

            if (file.error() != QFile::NoError || fileImage.error() != QFile::NoError) {
                status = Status::FAIL;
                status.setTitle(QObject::tr("Error: Copy file(s)"));
                status.setDescription(QObject::tr("Failed to copy: ") +
                                      file.fileName() + "\n " + fileImage.fileName() +
                                      QObject::tr(", the folder error was given: ") + file.errorString() + "\n" + fileImage.errorString());
                return status;
            }

            newName = clonedName;
        }
    }

    return status;
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
    if (!file.exists()) {
        brushPath = QString(BRUSH_QRC) + "/" + brushPreset + "/" + brushName;
    }
    return brushPath + extension;
}

QString MPBrushManager::getBrushImagePath(const QString& brushPreset, const QString& brushName)
{
    return getBrushPath(brushPreset, brushName, BRUSH_PREVIEW_EXT);
}

QString MPBrushManager::getBrushConfigPath(const QString extension)
{
    QString brushPath = MPCONF::getBrushesPath() + "/" + extension;

    QFile file(brushPath);
    if (!file.exists()) {
        brushPath = BRUSH_QRC + "/" + extension;
    }
    return brushPath;
}

Status MPBrushManager::writeBrushIcon(const QPixmap iconPix, const QString brushPreset, const QString brushName) {
    Status status = Status::OK;

    const QString brushPath = MPCONF::getBrushesPath() + "/" + brushPreset + "/" + brushName;

    const QString brushFileName = brushPath+BRUSH_PREVIEW_EXT;
    if (iconPix.save(brushPath+BRUSH_PREVIEW_EXT) == false) {
        status = Status::FAIL;
        status.setTitle(QObject::tr("Error saving brushImage"));
        status.setDescription(QObject::tr("Failed to save: ") + brushFileName);
    }
    return status;
}
