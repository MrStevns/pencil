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

#ifndef MPCONFIGFILEHANDLER_H
#define MPCONFIGFILEHANDLER_H

#include "mpbrushutils.h"

#include <QFile>

class MPConfigFileHandler: public QObject
{
    Q_OBJECT
public:
    MPConfigFileHandler(QObject* parent = nullptr);

    Status read();
    void deleteConfig();

    const QVector<MPBrushPreset> presets() const;

private:
    QVector<MPBrushPreset> parseConfig(QFile& file);

    QString resourcePath() const;
    QString configPath() const;

    QVector<MPBrushPreset> mPresets;
};

#endif // MPCONFIGFILEHANDLER_H
