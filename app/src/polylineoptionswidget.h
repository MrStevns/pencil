/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang
Copyright (C) 2025-2099 Oliver S. Larsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#ifndef POLYLINEOPTIONSWIDGET_H
#define POLYLINEOPTIONSWIDGET_H

#include "basewidget.h"

class Editor;
class PolylineTool;

namespace Ui {
class PolylineOptionsWidget;
}

class PolylineOptionsWidget : public BaseWidget
{
    Q_OBJECT
public:
    explicit PolylineOptionsWidget(Editor* editor, QWidget *parent = nullptr);
    ~PolylineOptionsWidget();

    void initUI() override;
    void updateUI() override;

private:
    void makeConnectionFromModelToUI();
    void makeConnectionFromUIToModel();

    void setWidthValue(qreal width);
    void setAntiAliasingEnabled(bool enabled);
    void setBezierPathEnabled(bool enabled);
    void setClosedPathEnabled(bool enabled);

    Ui::PolylineOptionsWidget *ui;

    PolylineTool* mPolylineTool = nullptr;
    Editor* mEditor = nullptr;
};

#endif // POLYLINEOPTIONSWIDGET_H
