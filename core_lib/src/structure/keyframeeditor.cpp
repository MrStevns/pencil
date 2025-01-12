#include "keyframeeditor.h"

KeyFrameEditor::KeyFrameEditor()
{
}

KeyFrameEditor::KeyFrameEditor(KeyFrameEditor &editor)
{
    mFrame = editor.mFrame;
    mIsModified = editor.mIsModified;
    mIsSelected = editor.mIsSelected;
    mLength = editor.mLength;
    mAttachedFileName = editor.mAttachedFileName;
}
