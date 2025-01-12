#ifndef KEYFRAMEEDITOR_H
#define KEYFRAMEEDITOR_H


class KeyFrameEditor
{

public:
    explicit KeyFrameEditor();
    explicit KeyFrameEditor(KeyFrameEditor& editor);

    int pos() const { return mFrame; }
    void setPos(int position) { mFrame = position; }
    int length() const { return mLength; }
    void setLength(int length) { mLength = length; }

    void modification() { mIsModified = true; }
    void setModified(bool modified) { mIsModified = modified; }
    bool isModified() { return mIsModified; }

    void setSelected(bool b) { mIsSelected = b; }
    bool isSelected() const { return mIsSelected; }

    QString fileName() const { return mAttachedFileName; }
    void    setFileName(QString strFileName) { mAttachedFileName = strFileName; }

private:
    int mFrame = -1;
    int mLength = 1;
    bool mIsModified = true;
    bool mIsSelected = false;
    QString mAttachedFileName;
};

#endif // KEYFRAMEEDITOR_H
