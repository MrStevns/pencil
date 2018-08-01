/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2008-2009 Mj Mendoza IV
Copyright (C) 2011-2015 Matt Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation;

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#ifndef PENCILAPP_H
#define PENCILAPP_H


#include <QApplication>
#include <QTabletEvent>
#include "mainwindow2.h"

class PencilApp : public QApplication
{
public:
    PencilApp(int &argc, char **argv) : QApplication(argc, argv) {}

    bool event(QEvent* event) Q_DECL_OVERRIDE;

    void setMainWindow(MainWindow2* m_window);

private:
    MainWindow2* m_window;
};

#endif // PENCILAPP_H
