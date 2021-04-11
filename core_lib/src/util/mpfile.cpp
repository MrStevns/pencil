#include "mpfile.h"

#include "mpbrushutils.h"

#include <QDir>
#include <QFileInfo>

MPFile::MPFile(const QString& brushFilePath) : mBrushPath(brushFilePath)
{

}

QString MPFile::getPresetName() const
{
    return QDir(mBrushPath).dirName();
}

QString MPFile::getBrushName() const
{
    return QFileInfo(mBrushPath).baseName();
}

QString MPFile::getPresetPath() const
{
    return QFileInfo(mBrushPath).absolutePath();
}

QString MPFile::getBrushPath() const
{
    return mBrushPath;
}

Status MPFile::updateBrushIcon(const QPixmap& iconPixmap, const Status* preStatus)
{
    if (preStatus && preStatus->fail()) { return *preStatus; }

    QString iconPath = getPresetPath() + "/" + getBrushName() + BRUSH_PREVIEW_EXT;
    if (iconPixmap.save(iconPath) == false) {
        Status status = Status::FAIL;
        status.setTitle(QObject::tr("Error saving image"));
        status.setDescription(QObject::tr("Failed to save: ") + iconPath);
        return status;
    }
    return Status::OK;
}

Status MPFile::renameBrush(const QString &renamePath, const Status* preStatus)
{
    if (preStatus && preStatus->fail()) { return *preStatus; }

    QString absolutePresetPath = QFileInfo(renamePath).absolutePath();

    Status status = createFolder(absolutePresetPath, preStatus);

    status = renameBrushFile(renamePath, BRUSH_CONTENT_EXT, &status);
    status = renameBrushFile(renamePath, BRUSH_PREVIEW_EXT, &status);

    // Return explicitly to avoid setting member variable.
    if (status.fail()) {
        return status;
    }
    mBrushPath = renamePath;

    return status;
}

Status MPFile::copyBrush(const QString &copyToPath, const Status* preStatus)
{
    if (preStatus && preStatus->fail()) { return *preStatus; }

    QDir dir(copyToPath);
    Status status = createFolder(copyToPath, preStatus);

    status = copyBrushFile(copyToPath, BRUSH_CONTENT_EXT, &status);
    status = copyBrushFile(copyToPath, BRUSH_PREVIEW_EXT, &status);

    return status;
}

Status MPFile::copyBrushFile(const QString &copyToPath, const QString& extension, const Status* preStatus)
{
    if (preStatus && preStatus->fail()) { return *preStatus; }

    QFileInfo info(copyToPath);

    QString baseFileName = info.baseName();
    QString folderPath = info.absolutePath();
    QString filePath = folderPath + "/" + baseFileName + extension;
    QFile file(filePath);

    if (file.error() != QFile::NoError) {
        Status status = Status::FAIL;
        status.setTitle(QObject::tr("File error"));
        status.setDescription(QObject::tr("Failed to read files: ") + file.errorString());
        return status;
    }

    if (file.exists()) {

        QDir dirPath(folderPath);

        QString newFileName = info.baseName();
        auto entries = dirPath.entryList();

        int clones = 0;
        for (int i = 0; i < entries.count(); i++) {
            const QString& entry = entries[i];

            if (entry.compare(newFileName + extension) == 0) {
                clones++;
            }
        }

        if (newFileName.compare(getBrushName()) == 0) {
            newFileName = newFileName.append(BRUSH_COPY_POSTFIX+QString::number(clones));
        }

        filePath = info.absolutePath() + "/" + newFileName + extension;
    }

    QString filePathToCopy = getPresetPath() + "/" + getBrushName() + extension;
    QFile fileToCopy(filePathToCopy);
    fileToCopy.copy(filePath);

    if (fileToCopy.error() != QFile::NoError) {
        Status status = Status::FAIL;
        status.setTitle("Error: Copy file");
        status.setDescription(QObject::tr("Failed to copy: ") + fileToCopy.fileName() + ", the following error was given: "
                              + fileToCopy.errorString());

        return status;
    }

    return Status::OK;
}

Status MPFile::renameBrushFile(const QString& renamePath, const QString& extension, const Status* preStatus)
{
    if (preStatus && preStatus->fail()) { return *preStatus; }

    QFileInfo info(renamePath);

    QString baseFileName = info.baseName();
    QString folderPath = info.absolutePath();
    QString filePath = folderPath + "/" + baseFileName + extension;

    QFile file(filePath);

    if (file.exists()) {
        Q_STATIC_ASSERT_X(true, "overwrite existing file is not implemented yet");
    }

    info = QFileInfo(getBrushPath());

    baseFileName = info.baseName();
    folderPath = info.absolutePath();
    QString currentAbsoluteFilePath = folderPath + "/" + baseFileName + extension;

    // Rename not neccesary, return safely
    if (currentAbsoluteFilePath.compare(filePath, Qt::CaseInsensitive) == 0) {
        return Status::SAFE;
    }

    QFile moveFile(currentAbsoluteFilePath);
    moveFile.rename(filePath);

    if (moveFile.error() != QFile::NoError) {
        Status status = Status::FAIL;

        status.setTitle(QObject::tr("Something went wrong"));
        status.setDescription(QObject::tr("Failed to rename or move: ") + moveFile.fileName() +
                              QObject::tr(" The following error was given: ") + moveFile.errorString());
        return status;
    }

    return Status::OK;
}

Status MPFile::createFolder(const QString& path, const Status* preStatus)
{
    if (preStatus && preStatus->fail()) { return *preStatus; }

    QFileInfo info(path);
    QString folderPath = info.absolutePath();
    auto dirRef = QDir(folderPath);

    if (dirRef.exists()) {
        return Status::SAFE;
    }

    if (!dirRef.mkpath(folderPath)) {
        Status status = Status::FAIL;

        status.setTitle(QObject::tr("Something went wrong"));
        status.setDescription(QObject::tr("Couldn't create dir: ") + folderPath + QObject::tr("verify that the folder is writable"));
        return status;
    }

    return Status::OK;
}
