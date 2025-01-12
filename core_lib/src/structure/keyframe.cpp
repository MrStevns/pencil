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

#include "keyframe.h"

KeyFrame::KeyFrame()
{
    mKeyEditor = new KeyFrameEditor();
}

KeyFrame::KeyFrame(const KeyFrame& k2)
{
    mKeyEditor = new KeyFrameEditor(*k2.mKeyEditor);
    // intentionally not copying event listeners
}

KeyFrame::~KeyFrame()
{
    for (KeyFrameEventListener* listener : mEventListeners)
    {
        listener->onKeyFrameDestroy(this);
    }
    // if (eventCallback) {
    //     eventCallback(KeyFrameEvent::DESTROY, this);
    // }
}

KeyFrame& KeyFrame::operator=(const KeyFrame& k2)
{
	if (this == &k2)
	{
		return *this; // a self-assignment
	}

    mKeyEditor = new KeyFrameEditor(*k2.mKeyEditor);
    // intentionally not copying event listeners
    return *this;
}

void KeyFrame::modification() {
    mKeyEditor->modification();

    if (eventCallback) {
        eventCallback(KeyFrameEvent::MODIFY, this);
    }
}

void KeyFrame::setModified(bool b)
{
    mKeyEditor->setModified(b);

    if (b && eventCallback) {
        eventCallback(KeyFrameEvent::MODIFY, this);
    }
}

void KeyFrame::setupEventCallback(KeyFrameEventCallback eventCallback)
{
    this->eventCallback = eventCallback;
    eventCallback(KeyFrameEvent::CREATE, this);
}

void KeyFrame::addEventListener(KeyFrameEventListener* listener)
{
    auto it = std::find(mEventListeners.begin(), mEventListeners.end(), listener);
    if (it == mEventListeners.end())
    {
        mEventListeners.push_back(listener);
    }
}

void KeyFrame::removeEventListner(KeyFrameEventListener* listener)
{
    auto it = std::find(mEventListeners.begin(), mEventListeners.end(), listener);
    if (it != mEventListeners.end())
    {
        mEventListeners.erase(it);
    }
}
