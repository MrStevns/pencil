/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef KeyFrame_H
#define KeyFrame_H

#include <cstdint>
#include <vector>
#include <memory>
#include <QString>
#include "pencilerror.h"
#include "pencildef.h"
#include "keyframeeditor.h"

class KeyFrameEventListener;

class KeyFrame
{
    typedef std::function<void(KeyFrameEvent event, KeyFrame*)> KeyFrameEventCallback;
public:
    explicit KeyFrame();

    explicit KeyFrame(const KeyFrame& k2);
    virtual ~KeyFrame();

    KeyFrame& operator=(const KeyFrame& k2);

    int  pos() const { return mKeyEditor->pos(); }
    void setPos(int position) { mKeyEditor->setPos(position); }

    int length() const { return mKeyEditor->length(); }
    void setLength(int len) { mKeyEditor->setLength(len); }

    void modification();
    void setModified(bool b);
    bool isModified() const { return mKeyEditor->isModified(); }

    void setSelected(bool b) { mKeyEditor->setSelected(b); }
    bool isSelected() const { return mKeyEditor->isSelected(); }

    QString fileName() const { return mKeyEditor->fileName(); }
    void    setFileName(QString strFileName) { mKeyEditor->setFileName(strFileName); }

    void setupEventCallback(KeyFrameEventCallback eventCallback);

    void addEventListener(KeyFrameEventListener*);
    void removeEventListner(KeyFrameEventListener*);

    virtual KeyFrame* clone() const { return nullptr; }
    virtual void loadFile() {}
    virtual void unloadFile() {}
    virtual bool isLoaded() const { return true; }

    virtual quint64 memoryUsage() { return 0; }

protected:
    KeyFrameEditor* mKeyEditor = nullptr;

private:
    // Contrary to KeyFrameEventListener, this callback is meant to be
    // setup and triggered for all keyframe types
    KeyFrameEventCallback eventCallback;

    std::vector<KeyFrameEventListener*> mEventListeners;
};

class KeyFrameEventListener
{
public:
    virtual void onKeyFrameDestroy(KeyFrame*) = 0;
};

#endif // KeyFrame_H
