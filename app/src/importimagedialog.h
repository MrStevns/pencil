#ifndef IMPORTIMAGEDIALOG_H
#define IMPORTIMAGEDIALOG_H

#include <importexportdialog.h>
#include "filedialogex.h"

#include <QWidget>
#include <QDialog>
#include <QImage>

class FileDialog;
class BitmapImage;
class QGraphicsScene;
class Editor;

namespace Ui {
class ImportImageDialog;
}


class ImportImageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImportImageDialog(QDialog* parent, enum FileType fileType);
    ~ImportImageDialog();

    void browse();
    void loadImageFromPath(QString path);
    void updateThreshold(int value);

    void applyOptions();
    void sizeToFit();
    void updateZoom();
    void resetPreview();

    void mapThreshold(int value);

    void setEditor(Editor* editor) {mEditor = editor;}

signals:

public slots:

private:
    Ui::ImportImageDialog* ui = nullptr;
    FileDialog* mFileDialog = nullptr;

    QStringList mFilePaths;
    const enum FileType mFileType;

    BitmapImage* mInputImage = nullptr;
    BitmapImage* mPreviewImage = nullptr;
    BitmapImage* mConvertedPreview = nullptr;
    QImage image;

    Editor* mEditor = nullptr;

    int mThresholdValue;

};

#endif // IMPORTIMAGEDIALOG_H
