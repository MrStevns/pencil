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

Status MPFile::updateBrushIcon(const QPixmap& iconPixmap)
{
    QString iconPath = getPresetPath() + "/" + getBrushName() + BRUSH_PREVIEW_EXT;
    if (iconPixmap.save(iconPath) == false) {
        Status status = Status::FAIL;
        status.setTitle(QObject::tr("Error saving image"));
        status.setDescription(QObject::tr("Failed to save: ") + iconPath);
        return status;
    }
    return Status::OK;
}

Status MPFile::renameBrush(const QString &renamePath)
{
    QString absolutePresetPath = QFileInfo(renamePath).absolutePath();

    Status status = createFolder(absolutePresetPath);
    if (status.fail()) {
        return status;
    }

    Status st = renameBrushFile(renamePath, BRUSH_CONTENT_EXT);

    if (st.fail()) {
        return st;
    }

    st = renameBrushFile(renamePath, BRUSH_PREVIEW_EXT);

    if (st.fail()) {
        return st;
    }

    mBrushPath = renamePath;

    return status;
}

Status MPFile::copyBrush(const QString &copyToPath)
{
    QDir dir(copyToPath);
    Status status = createFolder(copyToPath);
    if (status.fail()) {
        return status;
    }

    Status st = copyBrushFile(copyToPath, BRUSH_CONTENT_EXT);

    if (st.fail()) { return st; }

    st = copyBrushFile(copyToPath, BRUSH_PREVIEW_EXT);

    if (st.fail()) { return st; }

    return Status::OK;
}

Status MPFile::copyBrushFile(const QString &copyToPath, const QString& extension)
{
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

Status MPFile::renameBrushFile(const QString& renamePath, const QString& extension)
{
    QFileInfo info(renamePath);

    QString baseFileName = info.baseName();
    QString folderPath = info.absolutePath();
    QString filePath = folderPath + "/" + baseFileName + extension;

    QFile file(filePath);

    if (file.exists()) {

        Q_STATIC_ASSERT_X(true, "overwrite existing file is not implemented yet");
        Status status = Status::FAIL;
        status.setTitle(tr("Existing file"));
        status.setDescription(tr("Failed to rename: ") + file.fileName() + tr(" a file with same name already exists.")
                              + tr("\nOverwriting file has not been implemented yet!"));
        return Status::FAIL;
    }

    info = QFileInfo(getBrushPath());

    baseFileName = info.baseName();
    folderPath = info.absolutePath();
    QString currentAbsoluteFilePath = folderPath + "/" + baseFileName + extension;

    QFile moveFile(currentAbsoluteFilePath);
    moveFile.rename(filePath);

    if (moveFile.error() != QFile::NoError) {
        Status status = Status::FAIL;

        status.setTitle(QObject::tr("Something went wrong"));
        status.setDescription(QObject::tr("Failed to rename or move: ") + moveFile.fileName() + QObject::tr(" verify that the folder is writable")
                              + QObject::tr("The following error was given: ") + moveFile.errorString());
        return status;
    }

    return Status::OK;
}

Status MPFile::createFolder(const QString& path)
{
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
