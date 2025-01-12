#include "keyframeeditor.h"

#include <QDebug>

KeyFrameEditor::KeyFrameEditor()
{
    qDebug() << "created empty KeyFrameEditor";
}

KeyFrameEditor::KeyFrameEditor(KeyFrameEditor &editor)
{
    qDebug() << "created copy of KeyFrameEditor";
    mFrame = editor.mFrame;
    mIsModified = editor.mIsModified;
    mIsSelected = editor.mIsSelected;
    mLength = editor.mLength;
    mAttachedFileName = editor.mAttachedFileName;
}

KeyFrameEditor::~KeyFrameEditor()
{
}
