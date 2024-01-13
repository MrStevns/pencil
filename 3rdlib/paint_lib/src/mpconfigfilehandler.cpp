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

#include "mpconfigfilehandler.h"

MPConfigFileHandler::MPConfigFileHandler(QObject* parent) : QObject(parent)
{

}

Status MPConfigFileHandler::read()
{
    QFile fileOrder(configPath());

    Status st = Status::OK;
    if (fileOrder.open(QIODevice::ReadOnly))
    {
        QVector<MPBrushPreset> presets = parseConfig(fileOrder);

        if (presets.isEmpty() || presets.first().allBrushes().isEmpty()) {
            st = Status::FAIL;
            DebugDetails dd;

            dd << "file path: " + fileOrder.fileName();
            st.setTitle(tr("Parse error!"));
            st.setDescription(tr("Not able to parse brush config"));
            st.setDetails(dd);
            return st;
        }

        mPresets = presets;
    }

    return st;
}

void MPConfigFileHandler::deleteConfig()
{
    QFile configFile(configPath());
    configFile.remove();
}

const QVector<MPBrushPreset> MPConfigFileHandler::presets() const
{
    return mPresets;
}

/// Parses the mypaint brush config ".conf" format and returns a map of the brush groups
QVector<MPBrushPreset> MPConfigFileHandler::parseConfig(QFile& file)
{
    MPBrushPreset brushesForPreset;
    QString currentTool;
    QString currentPreset;
    QStringList brushList;

    QVector<MPBrushPreset> brushPresets;

    int presetIndex = 0;
    while (!file.atEnd())
    {
        QString line ( file.readLine().trimmed() );
        if (line.isEmpty() || line.startsWith("#")) continue;

        if (MPCONF::isPresetToken(line))
        {
            if (!brushesForPreset.isEmpty() && !currentTool.isEmpty()) {

                brushesForPreset.insert(currentTool, brushList);

                brushPresets[presetIndex] = brushesForPreset;
                presetIndex++;
            }
            currentPreset = MPCONF::getValue(line);
            brushesForPreset.name = currentPreset;

            brushesForPreset.clear();
            brushList.clear();

            brushPresets.append(brushesForPreset);
            continue;
        }

        if (MPCONF::isToolToken(line))
        {

            if (!currentTool.isEmpty()) {
                brushesForPreset.insert(currentTool, brushList);
            }

            currentTool = MPCONF::getValue(line);
            brushList.clear();
            continue;
        }

        if (MPCONF::isBrushToken(line)) {

            QString brush = MPCONF::getValue(line);
            QString relativePath = currentPreset + "/" + brush;
            if (QFileInfo(resourcePath() + "/" + relativePath + BRUSH_CONTENT_EXT).isReadable()) {
                brushList << brush;
            }
            continue;
        }

        if (!currentTool.isEmpty() && !brushPresets.isEmpty()) {
            brushPresets.append(brushesForPreset);
        }
    }

    if (!brushesForPreset.isEmpty() && !currentTool.isEmpty()) {

        brushesForPreset.insert(currentTool, brushList);

        Q_ASSERT(presetIndex <= brushPresets.size());

        brushPresets[presetIndex] = brushesForPreset;
        presetIndex++;
    }

    return brushPresets;
}

QString MPConfigFileHandler::resourcePath() const
{
    QStringList pathList = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    return pathList.first() + "/" + "brushes";
}

QString MPConfigFileHandler::configPath() const {
    return resourcePath() + "/" + "brushes.conf";
}
