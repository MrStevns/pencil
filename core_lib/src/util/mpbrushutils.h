#ifndef MPBRUSHUTILS_H
#define MPBRUSHUTILS_H

#include <QString>
#include <QMap>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QDirIterator>
#include <QJsonObject>

#include <QPixmap>

#include <QDebug>

#include "pencilerror.h"
#include "brushsetting.h"

static const QString BRUSH_CONTENT_EXT = ".myb";
static const QString BRUSH_PREVIEW_EXT = "_prev.png";
static const QString BRUSH_QRC = ":brushes";
static const QString BrushesFolderName = "brushes";
static const QString BrushConfigExtension = ".conf";
static const QString BrushConfigFile = BrushesFolderName+BrushConfigExtension;
static const int ICON_SZ = 64;
static const QString BRUSH_COPY_POSTFIX = "_clone";
static const QString DefaultPreset = "deevad";

static const QString CommentToken= "#";
static const QString PresetToken = "Preset:";
static const QString BrushToken = "-";
static const QString ToolToken = "Tool:";
struct MPCONF {

    static Status renamePreset(const QString& oldName, const QString& newName)
    {
        Status st = Status::OK;
        QString brushConfigPath = getBrushesPath() + QDir::separator() + BrushConfigFile;

        QFile file(brushConfigPath);
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream stream(&file);

        QStringList newFilesList;
        bool presetRenamed = false;

        while (!stream.atEnd()) {
            QString line = stream.readLine();

            if (!presetRenamed) {
                if (MPCONF::getValue(line).compare(oldName, Qt::CaseInsensitive) == 0) {
                    line = PresetToken + " " + newName;
                    presetRenamed = true;
                }
            }

            newFilesList << line;
        }
        file.close();

        if (newFilesList.isEmpty()) { return st; }

        QFile editConfigFile(brushConfigPath);
        editConfigFile.resize(0);
        editConfigFile.open(QFile::ReadWrite);

        if (editConfigFile.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream editStream(&editConfigFile);

        for (QString line : newFilesList) {
            editStream << line + "\n";
        }

        editConfigFile.close();

        return st;
    }

    static Status addPreset(const QString& presetName)
    {
        Status st = Status::OK;
        QString brushConfigPath = getBrushesPath() + QDir::separator() + BrushConfigFile;

        QFile file(brushConfigPath);
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream stream(&file);

        QStringList readLineList;
        while (!stream.atEnd()) {
            readLineList << stream.readLine();
        }

        QStringList newFilesList;
        bool presetTokenFound = false;
        bool presetAdded = false;
        for (const QString& line : readLineList) {

            // Find preset first
            if (!presetAdded) {
                if (isPresetToken(line)) {

                    presetTokenFound = true;
                }

                if (presetTokenFound) {
                    newFilesList.append(PresetToken + " " + presetName);
                    newFilesList.append("\tTool: pencil");
                    newFilesList.append("\tTool: eraser");
                    newFilesList.append("\tTool: pen");
                    newFilesList.append("\tTool: smudge");
                    newFilesList.append("\tTool: brush");
                    presetAdded = true;
                }
            }

            newFilesList << line;
        }
        file.close();

        QFile editConfigFile(brushConfigPath);
        editConfigFile.open(QFile::ReadWrite);

        if (editConfigFile.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream editStream(&editConfigFile);

        for (QString line : newFilesList) {
            editStream << line + "\n";
        }

        editConfigFile.close();

        return st;
    }

    static Status removePreset(const QString presetName)
    {
        Status st = Status::OK;
        QString brushConfigPath = getBrushesPath() + QDir::separator() + BrushConfigFile;

        QFile file(brushConfigPath);
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream stream(&file);
        bool searchingPreset = false;
        QStringList newFilesList;
        while (!stream.atEnd()) {

            const QString& line = stream.readLine();
                if (isPresetToken(line)) {

                    // find preset of interest otherwise skip
                    if (MPCONF::getValue(line).compare(presetName, Qt::CaseInsensitive) == 0) {
                        searchingPreset = true;
                    } else {
                        searchingPreset = false;
                    }
                }

            if (searchingPreset) {
                continue;
            }
            newFilesList << line;
        }
        file.close();

        if (newFilesList.isEmpty()) { return Status::FAIL; }

        QFile editConfigFile(brushConfigPath);
        editConfigFile.resize(0);
        editConfigFile.open(QFile::ReadWrite);

        if (editConfigFile.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream editStream(&editConfigFile);
        editStream.reset();

        for (QString line : newFilesList) {
            editStream << line + "\n";
            qDebug() << line;
        }

        editConfigFile.close();

        return st;
    }

    // TODO: handle case where no conf file exists ...
    static Status addBrushEntry(const QString toolName, const QString& brushPreset, const QString& brushName)
    {
        Status st = Status::OK;
        QString brushConfigPath = getBrushesPath() + QDir::separator() + BrushConfigFile;

        QFile file(brushConfigPath);
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream stream(&file);

        bool searchingPreset = false;
        bool searchingTool = false;
        bool brushAdded = false;
        QStringList newFilesList;
        while (!stream.atEnd()) {

            QString line = stream.readLine();
            // Find preset first
            if (!brushAdded) {

                qDebug() << MPCONF::getValue(line);
                if (isPresetToken(line)) {

                    // find preset of interest otherwise skip
                    if (MPCONF::getValue(line).compare(brushPreset, Qt::CaseInsensitive) == 0) {
                        searchingPreset = true;
                    } else {
                        searchingPreset = false;
                    }
                }

                if (isToolToken(line)) {
                    if (MPCONF::getValue(line).compare(toolName, Qt::CaseInsensitive) == 0) {
                        searchingTool = true;
                    } else {
                        searchingTool = false;
                    }
                }
            }

            if (searchingTool && searchingPreset) {
                if (!brushAdded) {

                    if (isToolToken(line)) {
                        newFilesList << line;
                        newFilesList << "\t\t- " + brushName;
                    } else {
                        newFilesList << "\t\t- " + brushName;
                    }
                    brushAdded = true;
                    continue;
                }
            }
            newFilesList << line;
        }
        file.close();

        QFile editConfigFile(brushConfigPath);
        editConfigFile.open(QFile::ReadWrite);

        if (editConfigFile.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream editStream(&editConfigFile);

        for (QString line : newFilesList) {
            editStream << line + "\n";
        }

        editConfigFile.close();

        return st;
    }

    static Status removeBrush(const QString& brushPreset, const QString& toolName, const QString& brushName)
    {
        Status st = Status::OK;
        QString brushConfigPath = getBrushesPath() + QDir::separator() + BrushConfigFile;

        QFile file(brushConfigPath);

        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream stream(&file);

        QStringList newList;

        bool searchingPreset = false;
        bool searchingTool = false;
        while(!stream.atEnd()) {
            QString line = stream.readLine();

            if (isPresetToken(line)) {

                if (MPCONF::getValue(line).compare(brushPreset, Qt::CaseInsensitive) == 0) {
                    searchingPreset = true;
                } else {
                    searchingPreset = false;
                }
            }

            if (isToolToken(line)) {

                if (MPCONF::getValue(line).compare(toolName, Qt::CaseInsensitive) == 0) {
                    searchingTool = true;
                } else {
                    searchingTool = false;
                }
            }

            if ((searchingPreset && searchingTool)) {
                if (MPCONF::getValue(line).compare(brushName, Qt::CaseInsensitive) == 0)
                {
                    continue;
                }
            }
            newList << line;
        }
        file.close();

        QFile editConfigFile(brushConfigPath);

        editConfigFile.resize(0);
        editConfigFile.open(QFile::ReadWrite);
        if (editConfigFile.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream editStream(&editConfigFile);

        for (QString line : newList) {
            editStream << line + "\n";
        }

        editConfigFile.close();

        return Status::OK;
    }

    // MAYBE: There's no need to blacklist files anymore since it's all been moved to disk
    // simply deleting the brush now and removing it from the config file should be enough.
    // might be better for performance if the user owns a lot of brushes...
    static Status blackListBrushFile(const QString& brushPreset, const QString& brushName)
    {
        Status st = Status::OK;
        QString brushConfigPath = getBrushesPath() + QDir::separator() + BrushConfigFile;

        QFile file(brushConfigPath);

        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream stream(&file);

        QStringList newList;

        bool searchingPreset = false;
        while(!stream.atEnd()) {
            QString line = stream.readLine();

            if (isPresetToken(line)) {

                if (MPCONF::getValue(line).compare(brushPreset, Qt::CaseInsensitive) == 0) {
                    searchingPreset = true;
                } else {
                    searchingPreset = false;
                }
            }

            if (searchingPreset) {
                if (MPCONF::getValue(line).compare(brushName, Qt::CaseInsensitive) == 0)
                {
                    line = line.prepend("#");
                }
            }
            newList << line;
        }
        file.close();

        QFile editConfigFile(brushConfigPath);

        editConfigFile.open(QFile::ReadWrite);
        if (editConfigFile.error() != QFile::NoError) {
            st = Status::FAIL;

            st.setTitle(QObject::tr("Failed to open file"));
            st.setDescription(QObject::tr("The following error was given: \n") + file.errorString());
            return st;
        }

        QTextStream editStream(&editConfigFile);

        for (QString line : newList) {
            editStream << line + "\n";
        }

        editConfigFile.close();

        return Status::OK;
    }

    static QString getBrushesPath()
    {
        QStringList pathList = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
        return pathList.first() + QDir::separator() + "brushes";
    }

    static QString getValue(const QString& text) {
        if (isCommentToken(text)) {
            return text.section('#',1).trimmed();
        } else if (isPresetToken(text) || isToolToken(text)) {
            return text.section(':',1).trimmed();
        } else if (isBrushToken(text)) {
            return text.section('-',1).trimmed();
        }
        return "";
    }

    static bool isCommentToken(const QString& text) {
        if (text.isEmpty() || text.startsWith(CommentToken)) { return true; }
        return false;
    }

    static bool isPresetToken(const QString& text) {
        if (text.trimmed().startsWith(PresetToken, Qt::CaseInsensitive)) { return true; }
        return false;
    }

    static bool isToolToken(const QString& text) {
        if (text.trimmed().startsWith(ToolToken, Qt::CaseInsensitive)) { return true; }
        return false;
    }

    static bool isBrushToken(const QString& text) {
        if (text.trimmed().startsWith(BrushToken, Qt::CaseInsensitive)) { return true; }
        return false;
    }
};

struct MPBrushInfo {
    QString comment = "";
    qreal version = 0.0;

    void write(QJsonObject& object) const
    {
        QJsonObject::iterator commentObjIt = object.find("comment");

        QString commentId = "comment";
        if (commentObjIt->isUndefined()) {
            object.insert(commentId, comment);
        } else {
            object.remove(commentId);
            object.insert(commentId, comment);
        }

        QString brushVersionKey = "brush-version";
        QJsonObject::iterator versionObjIt = object.find("brush-version");
        if (versionObjIt->isUndefined()) {
            object.insert(brushVersionKey, version);
        } else {
            object.remove(brushVersionKey);
            object.insert(brushVersionKey, version);
        }
    }

    MPBrushInfo read(QJsonObject& object) {
        QJsonObject::iterator commentObjIt = object.find("comment");

        MPBrushInfo info;
        QString commentId = "comment";
        if (!commentObjIt->isUndefined()) {
            info.comment = object.value(commentId).toString();
        }

        QString brushVersionKey = "brush-version";
        QJsonObject::iterator versionObjIt = object.find(brushVersionKey);
        if (!versionObjIt->isUndefined()) {
            info.version = object.value(brushVersionKey).toDouble();
        }

        return info;
    }
};

struct MPBrushPreset {
    QString name;

    void clear() {
        brushForToolMap.clear();
    }

    QList<QStringList> allBrushes() const {
        return brushForToolMap.values();
    }

    QStringList brushesForTool(QString toolKey) const {
        return brushForToolMap.value(toolKey);
    }

    void insert(QString toolName, QStringList brushNames)
    {
        brushForToolMap.insert(toolName, brushNames);
    }

    bool isEmpty() const {
        return brushForToolMap.isEmpty();
    }

    QList<QString> toolNames() {
        return brushForToolMap.keys();
    }

private:
    QMap<QString, QStringList> brushForToolMap;

};

struct MPBrushParser {

    static QString getBrushSettingIdentifier(const BrushSettingType& type)
    {
        switch(type)
        {
        case BrushSettingType::BRUSH_SETTING_OPAQUE:                      return "opaque";
        case BrushSettingType::BRUSH_SETTING_OPAQUE_MULTIPLY:             return "opaque_multiply";
        case BrushSettingType::BRUSH_SETTING_OPAQUE_LINEARIZE:            return "opaque_linearize";
        case BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC:          return "radius_logarithmic";
        case BrushSettingType::BRUSH_SETTING_HARDNESS:                    return "hardness";
        case BrushSettingType::BRUSH_SETTING_SOFTNESS:                    return "softness";
        case BrushSettingType::BRUSH_SETTING_ANTI_ALIASING:               return "anti_aliasing";
        case BrushSettingType::BRUSH_SETTING_DABS_PER_BASIC_RADIUS:       return "dabs_per_basic_radius";
        case BrushSettingType::BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS:      return "dabs_per_actual_radius";
        case BrushSettingType::BRUSH_SETTING_DABS_PER_SECOND:             return "dabs_per_second";
        case BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE:               return "gridmap_scale";
        case BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_X:             return "gridmap_scale_x";
        case BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_Y:             return "gridmap_scale_y";
        case BrushSettingType::BRUSH_SETTING_RADIUS_BY_RANDOM:            return "radius_by_random";
        case BrushSettingType::BRUSH_SETTING_SPEED1_SLOWNESS:             return "speed1_slowness";
        case BrushSettingType::BRUSH_SETTING_SPEED2_SLOWNESS:             return "speed2_slowness";
        case BrushSettingType::BRUSH_SETTING_SPEED1_GAMMA:                return "speed1_gamma";
        case BrushSettingType::BRUSH_SETTING_SPEED2_GAMMA:                return "speed2_gamma";
        case BrushSettingType::BRUSH_SETTING_OFFSET_BY_RANDOM:            return "offset_by_random";
        case BrushSettingType::BRUSH_SETTING_OFFSET_Y:                    return "offset_y";
        case BrushSettingType::BRUSH_SETTING_OFFSET_X:                    return "offset_x";
        case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE:                return "offset_angle";
        case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ASC:            return "offset_angle_asc";
        case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2:              return "offset_angle_2";
        case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2_ASC:          return "offset_angle_2_asc";
        case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ADJ:            return "offset_angle_adj";
        case BrushSettingType::BRUSH_SETTING_OFFSET_MULTIPLIER:           return "offset_multiplier";
        case BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED:             return "offset_by_speed";
        case BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS:    return "offset_by_speed_slowness";
        case BrushSettingType::BRUSH_SETTING_SLOW_TRACKING:               return "slow_tracking";
        case BrushSettingType::BRUSH_SETTING_SLOW_TRACKING_PER_DAB:       return "slow_tracking_per_dab";
        case BrushSettingType::BRUSH_SETTING_TRACKING_NOISE:              return "tracking_noise";
        case BrushSettingType::BRUSH_SETTING_COLOR_H:                     return "color_h";
        case BrushSettingType::BRUSH_SETTING_COLOR_S:                     return "color_s";
        case BrushSettingType::BRUSH_SETTING_COLOR_V:                     return "color_v";
        case BrushSettingType::BRUSH_SETTING_RESTORE_COLOR:               return "restore_color";
        case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_H:              return "change_color_h";
        case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_L:              return "change_color_l";
        case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSL_S:          return "change_color_hsl_s";
        case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_V:              return "change_color_v";
        case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSV_S:          return "change_color_hsv_s";
        case BrushSettingType::BRUSH_SETTING_SMUDGE:                      return "smudge";
        case BrushSettingType::BRUSH_SETTING_SMUDGE_LENGTH:               return "smudge_length";
        case BrushSettingType::BRUSH_SETTING_SMUDGE_RADIUS_LOG:           return "smudge_radius_log";
        case BrushSettingType::BRUSH_SETTING_ERASER:                      return "eraser";
        case BrushSettingType::BRUSH_SETTING_STROKE_THRESHOLD:            return "stroke_threshold";
        case BrushSettingType::BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC: return "stroke_duration_logarithmic";
        case BrushSettingType::BRUSH_SETTING_STROKE_HOLDTIME:             return "stroke_holdtime";
        case BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT:                return "custom_input";
        case BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS:       return "custom_input_slowness";
        case BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_RATIO:        return "elliptical_dab_ratio";
        case BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE:        return "elliptical_dab_angle";
        case BrushSettingType::BRUSH_SETTING_DIRECTION_FILTER:            return "direction_filter";
        case BrushSettingType::BRUSH_SETTING_LOCK_ALPHA:                  return "lock_alpha";
        case BrushSettingType::BRUSH_SETTING_COLORIZE:                    return "colorize";
        case BrushSettingType::BRUSH_SETTING_SNAP_TO_PIXEL:               return "snap_to_pixel";
        case BrushSettingType::BRUSH_SETTING_PRESSURE_GAIN_LOG:           return "pressure_gain_log";
        default: return "";
        }
    }
};

#endif // MPBRUSHUTILS_H
