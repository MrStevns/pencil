#ifndef MPFILE_H
#define MPFILE_H

#include "pencilerror.h"

#include <QPixmap>

class MPFile : public QObject
{
    Q_OBJECT
public:
    explicit MPFile(const QString& brushPath);

    Status createFolder(const QString& path);
    Status updateBrushIcon(const QPixmap& iconPixmap);

    Status copyBrush(const QString& copyToPath);
    Status renameBrush(const QString& renamePath);

    QString getBrushPath() const;
    QString getBrushName() const;
    QString getPresetName() const;
    QString getPresetPath() const;

private:

    Status copyBrushFile(const QString &copyToPath, const QString& extension);
    Status renameBrushFile(const QString &renamedPath, const QString& extension);

    QString mBrushPath;
};

#endif // MPFILE_H
