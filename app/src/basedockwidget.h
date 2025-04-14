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

#ifndef BASEDOCKWIDGET_H
#define BASEDOCKWIDGET_H

#include <QDockWidget>

class Editor;
class PencilDockTitleBarWidget;


class BaseDockWidget : public QDockWidget
{
    Q_OBJECT

protected:
    explicit BaseDockWidget(QWidget* pParent);
    virtual  ~BaseDockWidget();

public:
    virtual void initUI() = 0;
    virtual void updateUI() = 0;

    void lock(bool state);
    void setTitle(const QString& title);

    bool isLocked() const { return mLocked; }

    Editor* editor() const { return mEditor; }
    void setEditor( Editor* e ) { mEditor = e; }

protected:
    void resizeEvent(QResizeEvent* event) override;
    virtual int getMinHeightForWidth(int width) const;

private:
    Editor* mEditor = nullptr;

    QWidget* mNoTitleBarWidget = nullptr;
    PencilDockTitleBarWidget* mTitleBarWidget = nullptr;

    bool mLocked = false;
};

#endif // BASEDOCKWIDGET_H
