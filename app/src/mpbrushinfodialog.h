#ifndef MPBRUSHCOSMETICS_H
#define MPBRUSHCOSMETICS_H

#include <QDialog>
#include <QJsonObject>
#include <QRegularExpression>

#include "pencildef.h"
#include <mpbrushutils.h>

class QLabel;
class QPlainTextEdit;
class ComboBox;
class QPushButton;
class Editor;

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

    bool processStatus(Status status);
    void didPressSetImage();
    void didPressSetImageFromClipBoard();
    void didSelectToolOption(int index, QString itemName, int value);
    void didPressCancel();
    void didPressSave();
    void didUpdateName();
    void didUpdatePreset(int index, QString name, int data);
    void didUpdateComment();
    void didUpdateVersion();

    QLabel* mImageLabel = nullptr;
    QPlainTextEdit* mNameTextEdit = nullptr;
    ComboBox* mPresetComboBox = nullptr;
    QPlainTextEdit* mCommentTextEdit = nullptr;
    QPlainTextEdit* mVersionTextEdit = nullptr;
    ComboBox* mToolComboBox = nullptr;
    QPushButton* mSetImageButton = nullptr;
    QPushButton* mSetImageFromClipBoard = nullptr;

    ToolType mToolType;

    Editor* mEditor = nullptr;

    QJsonObject mBrushInfoObject;

    QString mOriginalName;
    QString mOriginalPreset;
    QString mBrushName;
    QString mBrushPreset;
    QString mBrushComment;
    QString mBrushVersion;
    QString mToolName;

    QString mOldToolName;
    QString mOldPresetName;

    bool mIconModified = false;

    QRegularExpression regExp;

    MPBrushInfo mBrushInfo;
    const DialogContext mDialogContext;
};

#endif // MPBRUSHCOSMETICS_H
