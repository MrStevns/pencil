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
#include "pencilapp.h"

bool PencilApp::event(QEvent *event)
{
    if ( event->type() == QEvent::TabletEnterProximity ||
         event->type() == QEvent::TabletLeaveProximity ) {

        m_window->onTabletProximity(static_cast<QTabletEvent *>(event));

        return true;
    }

    return QApplication::event(event);
}

void PencilApp::setMainWindow(MainWindow2* window)
{
    m_window = window;
}
