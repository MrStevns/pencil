#ifndef MPBRUSHCOSMETICS_H
#define MPBRUSHCOSMETICS_H

#include <QDialog>
#include <QJsonObject>

#include "pencildef.h"
#include <mpbrushutils.h>

class QLabel;
class QPlainTextEdit;
class QLineEdit;
class ComboBox;
class QPushButton;
class Editor;
class MPFile;

enum DialogContext {
    Edit,
    Clone
};

class MPBrushInfoDialog : public QDialog
{
    Q_OBJECT
public:

    MPBrushInfoDialog(DialogContext dialogContext, QWidget* parent = nullptr);
    void setCore(Editor* editor) { mEditor = editor; }

    void initUI();

    void setBrushInfo(QString brushName, QString brushPreset, ToolType tool, QJsonDocument brushJsonDoc);

signals:
    void updatedBrushInfo(QString newName, QString brushPreset);

private:

    void didPressSetImage();
    void didPressSetImageFromClipBoard();
    void didSelectToolOption(int index, QString itemName, int value);
    void didPressSave();
    void didUpdateName();
    void didUpdatePreset(int index, QString name, int data);
    void didUpdateComment();
    void didUpdateVersion();

    Status didPressSaveAs(MPFile* mpFile, const QString& newName);

    QLabel* mImageLabel = nullptr;
    QLineEdit* mNameTextEdit = nullptr;
    QLineEdit* mVersionTextEdit = nullptr;
    ComboBox* mPresetComboBox = nullptr;
    QPlainTextEdit* mCommentTextEdit = nullptr;
    ComboBox* mToolComboBox = nullptr;
    QPushButton* mSetImageButton = nullptr;
    QPushButton* mSetImageFromClipBoard = nullptr;

    ToolType mToolType;

    Editor* mEditor = nullptr;

    QJsonObject mBrushInfoObject;

    QString mOriginalBrushName;
    QString mOriginalPresetName;
    QString mOriginalToolName;
    QString mBrushName;
    QString mPresetName;
    QString mToolName;
    QString mBrushComment;
    QString mBrushVersion;

    bool mIconModified = false;

    MPBrushInfo mBrushInfo;
    const DialogContext mDialogContext;
};

#endif // MPBRUSHCOSMETICS_H
