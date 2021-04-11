#ifndef MPFILE_H
#define MPFILE_H

#include "pencilerror.h"

#include <QPixmap>

class MPFile : public QObject
{
    Q_OBJECT
public:
    explicit MPFile(const QString& brushPath);

    Status createFolder(const QString& path, const Status* preStatus = nullptr);
    Status updateBrushIcon(const QPixmap& iconPixmap, const Status* preStatus = nullptr);

    Status copyBrush(const QString& copyToPath, const Status* preStatus = nullptr);
    Status renameBrush(const QString& renamePath, const Status* preStatus = nullptr);

    QString getBrushPath() const;
    QString getBrushName() const;
    QString getPresetName() const;
    QString getPresetPath() const;

private:

    Status copyBrushFile(const QString &copyToPath, const QString& extension, const Status* preStatus = nullptr);
    Status renameBrushFile(const QString &renamedPath, const QString& extension, const Status* preStatus = nullptr);

    QString mBrushPath;
};

#endif // MPFILE_H
