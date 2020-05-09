#ifndef BRUSHSETTING_H
#define BRUSHSETTING_H

#include <QVector>
#include <QPointF>
#include <QMap>

#include <QtGlobal>

/// This enum should be interchangeable with MyPaintBrushSetting
enum class BrushSettingType {
        BRUSH_SETTING_OPAQUE,
        BRUSH_SETTING_OPAQUE_MULTIPLY,
        BRUSH_SETTING_OPAQUE_LINEARIZE,
        BRUSH_SETTING_RADIUS_LOGARITHMIC,
        BRUSH_SETTING_HARDNESS,
        BRUSH_SETTING_SOFTNESS,
        BRUSH_SETTING_ANTI_ALIASING,
        BRUSH_SETTING_DABS_PER_BASIC_RADIUS,
        BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS,
        BRUSH_SETTING_DABS_PER_SECOND,
        BRUSH_SETTING_GRIDMAP_SCALE,
        BRUSH_SETTING_GRIDMAP_SCALE_X,
        BRUSH_SETTING_GRIDMAP_SCALE_Y,
        BRUSH_SETTING_RADIUS_BY_RANDOM,
        BRUSH_SETTING_SPEED1_SLOWNESS,
        BRUSH_SETTING_SPEED2_SLOWNESS,
        BRUSH_SETTING_SPEED1_GAMMA,
        BRUSH_SETTING_SPEED2_GAMMA,
        BRUSH_SETTING_OFFSET_BY_RANDOM,
        BRUSH_SETTING_OFFSET_Y,
        BRUSH_SETTING_OFFSET_X,
        BRUSH_SETTING_OFFSET_ANGLE,
        BRUSH_SETTING_OFFSET_ANGLE_ASC,
        BRUSH_SETTING_OFFSET_ANGLE_2,
        BRUSH_SETTING_OFFSET_ANGLE_2_ASC,
        BRUSH_SETTING_OFFSET_ANGLE_ADJ,
        BRUSH_SETTING_OFFSET_MULTIPLIER,
        BRUSH_SETTING_OFFSET_BY_SPEED,
        BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS,
        BRUSH_SETTING_SLOW_TRACKING,
        BRUSH_SETTING_SLOW_TRACKING_PER_DAB,
        BRUSH_SETTING_TRACKING_NOISE,
        BRUSH_SETTING_COLOR_H,
        BRUSH_SETTING_COLOR_S,
        BRUSH_SETTING_COLOR_V,
        BRUSH_SETTING_RESTORE_COLOR,
        BRUSH_SETTING_CHANGE_COLOR_H,
        BRUSH_SETTING_CHANGE_COLOR_L,
        BRUSH_SETTING_CHANGE_COLOR_HSL_S,
        BRUSH_SETTING_CHANGE_COLOR_V,
        BRUSH_SETTING_CHANGE_COLOR_HSV_S,
        BRUSH_SETTING_SMUDGE,
        BRUSH_SETTING_SMUDGE_LENGTH,
        BRUSH_SETTING_SMUDGE_RADIUS_LOG,
        BRUSH_SETTING_ERASER,
        BRUSH_SETTING_STROKE_THRESHOLD,
        BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC,
        BRUSH_SETTING_STROKE_HOLDTIME,
        BRUSH_SETTING_CUSTOM_INPUT,
        BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS,
        BRUSH_SETTING_ELLIPTICAL_DAB_RATIO,
        BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE,
        BRUSH_SETTING_DIRECTION_FILTER,
        BRUSH_SETTING_LOCK_ALPHA,
        BRUSH_SETTING_COLORIZE,
        BRUSH_SETTING_SNAP_TO_PIXEL,
        BRUSH_SETTING_PRESSURE_GAIN_LOG,
        BRUSH_SETTINGS_COUNT
};

inline uint qHash(BrushSettingType key, uint seed)
{
    return ::qHash(static_cast<uint>(key), seed);
}

struct BrushSetting {
    BrushSettingType type;
    qreal min;
    qreal max;
    QString name;

    BrushSetting(QString name, BrushSettingType type, qreal min, qreal max) { this->type = type;
                                                                              this->min = min;
                                                                              this->max = max;
                                                                              this->name = name; }
};

static const BrushSetting Opacity = BrushSetting(QT_TR_NOOP("Opacity"), BrushSettingType::BRUSH_SETTING_OPAQUE, 0, 100);
static const BrushSetting OpacityMultiply = BrushSetting(QT_TR_NOOP("Opacity multiply"), BrushSettingType::BRUSH_SETTING_OPAQUE_MULTIPLY, 0, 100);
static const BrushSetting OpacityLinearize = BrushSetting(QT_TR_NOOP("Opacity linearize"), BrushSettingType::BRUSH_SETTING_OPAQUE_LINEARIZE, 0, 100);
static const BrushSetting RadiusLog = BrushSetting(QT_TR_NOOP("Radius"), BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC, 0, 100);
static const BrushSetting Hardness = BrushSetting(QT_TR_NOOP("Hardness"), BrushSettingType::BRUSH_SETTING_HARDNESS, 0, 100);
static const BrushSetting DabsPerBasicRadius = BrushSetting(QT_TR_NOOP("Dabs per basic radius"), BrushSettingType::BRUSH_SETTING_DABS_PER_BASIC_RADIUS, 0, 100);
static const BrushSetting DabsPerActualRadius = BrushSetting(QT_TR_NOOP("Dabs per actual radius"), BrushSettingType::BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS, 0, 100);
static const BrushSetting DabsPerSecond = BrushSetting(QT_TR_NOOP("Dabs per second"), BrushSettingType::BRUSH_SETTING_DABS_PER_SECOND, 0, 100);
static const BrushSetting DabScale = BrushSetting(QT_TR_NOOP("Dab scale"), BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE, 0, 100);
static const BrushSetting DabScaleX = BrushSetting(QT_TR_NOOP("Dab scale X"), BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_X, 0, 100);
static const BrushSetting DabScaleY = BrushSetting(QT_TR_NOOP("Dab scale Y"), BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_Y, 0, 100);
static const BrushSetting RadiusRandom = BrushSetting(QT_TR_NOOP("Radius by random"), BrushSettingType::BRUSH_SETTING_RADIUS_BY_RANDOM, 0, 100);
static const BrushSetting SpeedStart = BrushSetting(QT_TR_NOOP("Speed start"), BrushSettingType::BRUSH_SETTING_SPEED1_SLOWNESS, 0, 100);
static const BrushSetting SpeedEnd = BrushSetting(QT_TR_NOOP("Speed end"), BrushSettingType::BRUSH_SETTING_SPEED2_SLOWNESS, 0, 100);
static const BrushSetting SpeedGammaStart = BrushSetting(QT_TR_NOOP("Speed gamma start"), BrushSettingType::BRUSH_SETTING_SPEED1_GAMMA, 0, 100);
static const BrushSetting SpeedGammaEnd = BrushSetting(QT_TR_NOOP("Speed gamma end"), BrushSettingType::BRUSH_SETTING_SPEED2_GAMMA, 0, 100);
static const BrushSetting OffsetRandom = BrushSetting(QT_TR_NOOP("Offset by random"), BrushSettingType::BRUSH_SETTING_OFFSET_BY_RANDOM, 0, 100);
static const BrushSetting OffsetX = BrushSetting(QT_TR_NOOP("Offset X"), BrushSettingType::BRUSH_SETTING_OFFSET_X, -100, 100);
static const BrushSetting OffsetY = BrushSetting(QT_TR_NOOP("Offset Y"), BrushSettingType::BRUSH_SETTING_OFFSET_Y, -100, 100);
static const BrushSetting OffsetAngleLeft = BrushSetting(QT_TR_NOOP("Offset angle left"), BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE, -100, 100);
static const BrushSetting OffsetAngleLeftAscend = BrushSetting(QT_TR_NOOP("Offset angle left ascend"), BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ASC, -100, 100);
static const BrushSetting OffsetAngleRight = BrushSetting(QT_TR_NOOP("Offset angle right"), BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2, -100, 100);
static const BrushSetting OffsetAngleRightAscend = BrushSetting(QT_TR_NOOP("Offset angle right ascend"), BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2_ASC, -100, 100);
static const BrushSetting OffsetAngleAdjecent = BrushSetting(QT_TR_NOOP("Offset angle adjecent"), BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ADJ, -180, 180);
static const BrushSetting OffsetMultiplier = BrushSetting(QT_TR_NOOP("Offset multiplier"), BrushSettingType::BRUSH_SETTING_OFFSET_MULTIPLIER, 0, 100);
static const BrushSetting OffsetBySpeed = BrushSetting(QT_TR_NOOP("Offset by speed"), BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED, 0, 100);
static const BrushSetting OffsetSpeedSlowness = BrushSetting(QT_TR_NOOP("Offset by speed slowness"), BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS, 0, 100);
static const BrushSetting SlowTracking = BrushSetting(QT_TR_NOOP("Slow tracking"), BrushSettingType::BRUSH_SETTING_SLOW_TRACKING, 0, 100);
static const BrushSetting SlowTrackingPerDab = BrushSetting(QT_TR_NOOP("Slow tracking per. dab"), BrushSettingType::BRUSH_SETTING_SLOW_TRACKING_PER_DAB, 0, 100);
static const BrushSetting TrackingNoise = BrushSetting(QT_TR_NOOP("Tracking noise"), BrushSettingType::BRUSH_SETTING_TRACKING_NOISE, 0, 100);
static const BrushSetting ChangeColorHue = BrushSetting(QT_TR_NOOP("Change color: Hue"), BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_H, -100, 100);
static const BrushSetting ChangeColorLightness = BrushSetting(QT_TR_NOOP("Change color: Lightness"), BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_L, -100, 100);
static const BrushSetting ChangeColorHLSSaturation = BrushSetting(QT_TR_NOOP("Change color (HLS): Saturation"), BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSL_S, -100, 100);
static const BrushSetting ChangeColorValue = BrushSetting(QT_TR_NOOP("Change color: Value"), BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_V, -100, 100);
static const BrushSetting ChangeColorHSVSaturation = BrushSetting(QT_TR_NOOP("Change color (HSV): Saturation"), BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSV_S, -100, 100);
static const BrushSetting RestoreColor = BrushSetting(QT_TR_NOOP("Restore color"), BrushSettingType::BRUSH_SETTING_RESTORE_COLOR, 0, 100);
static const BrushSetting Smudge = BrushSetting(QT_TR_NOOP("Smudge"), BrushSettingType::BRUSH_SETTING_SMUDGE, 0, 100);
static const BrushSetting SmudgeLength = BrushSetting(QT_TR_NOOP("Smudge length"), BrushSettingType::BRUSH_SETTING_SMUDGE_LENGTH, 0, 100);
static const BrushSetting SmudgeRadius = BrushSetting(QT_TR_NOOP("Smudge radius"), BrushSettingType::BRUSH_SETTING_SMUDGE_RADIUS_LOG, 0, 100);
static const BrushSetting Eraser = BrushSetting(QT_TR_NOOP("Eraser"), BrushSettingType::BRUSH_SETTING_ERASER, 0, 100);
static const BrushSetting StrokeThreshold = BrushSetting(QT_TR_NOOP("Stroke threshold"), BrushSettingType::BRUSH_SETTING_STROKE_THRESHOLD, 0, 100);
static const BrushSetting StrokeDuration = BrushSetting(QT_TR_NOOP("Stroke duration"), BrushSettingType::BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC, 0, 100);
static const BrushSetting StrokeHoldTime = BrushSetting(QT_TR_NOOP("Stroke holdtime"), BrushSettingType::BRUSH_SETTING_STROKE_HOLDTIME, 0, 100);
static const BrushSetting CustomInput = BrushSetting(QT_TR_NOOP("Custom input"), BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT, 0, 100);
static const BrushSetting CustomInputSlowness = BrushSetting(QT_TR_NOOP("Custom input slowness"), BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS, 0, 100);
static const BrushSetting EllepticalDabRatio = BrushSetting(QT_TR_NOOP("Elleptical dab ratio"), BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_RATIO, 0, 100);
static const BrushSetting EllepticalDabAngle = BrushSetting(QT_TR_NOOP("Elleptical dab angle"), BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE, 0, 100);
static const BrushSetting AntiAliasing = BrushSetting(QT_TR_NOOP("Anti-aliasing"), BrushSettingType::BRUSH_SETTING_ANTI_ALIASING, 0, 100);
static const BrushSetting LockAlpha = BrushSetting(QT_TR_NOOP("Lock Alpha"), BrushSettingType::BRUSH_SETTING_LOCK_ALPHA, 0, 100);
static const BrushSetting Colorize = BrushSetting(QT_TR_NOOP("Colorize"), BrushSettingType::BRUSH_SETTING_COLORIZE, 0, 100);
static const BrushSetting SnapToPixel = BrushSetting(QT_TR_NOOP("Snap to pixel"), BrushSettingType::BRUSH_SETTING_SNAP_TO_PIXEL, 0, 100);
static const BrushSetting PressureGain = BrushSetting(QT_TR_NOOP("Pressure gain"), BrushSettingType::BRUSH_SETTING_PRESSURE_GAIN_LOG, 0, 100);

enum class BrushState {
    BRUSH_STATE_ACTUAL_RADIUS
};

enum class BrushInputType {
        BRUSH_INPUT_PRESSURE,
        BRUSH_INPUT_SPEED1,
        BRUSH_INPUT_SPEED2,
        BRUSH_INPUT_RANDOM,
        BRUSH_INPUT_STROKE,
        BRUSH_INPUT_DIRECTION,
        BRUSH_INPUT_DIRECTION_ANGLE,
        BRUSH_INPUT_ATTACK_ANGLE,
        BRUSH_INPUT_TILT_DECLINATION,
        BRUSH_INPUT_TILT_ASCENSION,
        BRUSH_INPUT_GRIDMAP_X,
        BRUSH_INPUT_GRIDMAP_Y,
        BRUSH_INPUT_BRUSH_RADIUS,
        BRUSH_INPUT_CUSTOM,
        BRUSH_INPUTS_COUNT
};

struct MappingControlPoints {
  // a set of control points (stepwise linear)
  QVector<QPointF> points;
  int numberOfPoints;
};

struct BrushInputMapping {
    int inputsUsed;
    MappingControlPoints controlPoints;
    float baseValue;
};

struct BrushSettingInfo {
    QString cname;
    QString name;
    bool isConstant;
    float min;
    float defaultValue;
    float max;
    QString tooltip;
};

struct BrushInputInfo {
    QString cname;
    qreal hard_min;
    qreal soft_min; // Recommended min
    qreal normal;
    qreal soft_max; // Recommended max
    qreal hard_max;
    QString name;
    QString tooltip;
};

BrushSettingType inline getBrushSetting(const QString identifier)
{
    static QMap<QString, BrushSettingType> map
    {
        { "opaque", BrushSettingType::BRUSH_SETTING_OPAQUE },
        { "opaque_multiply", BrushSettingType::BRUSH_SETTING_OPAQUE_MULTIPLY },
        { "opaque_linearize", BrushSettingType::BRUSH_SETTING_OPAQUE_LINEARIZE },
        { "radius_logarithmic", BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC },
        { "hardness", BrushSettingType::BRUSH_SETTING_HARDNESS },
        { "softness", BrushSettingType::BRUSH_SETTING_SOFTNESS },
        { "anti_aliasing", BrushSettingType::BRUSH_SETTING_ANTI_ALIASING },
        { "dabs_per_basic_radius", BrushSettingType::BRUSH_SETTING_DABS_PER_BASIC_RADIUS },
        { "dabs_per_actual_radius", BrushSettingType::BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS },
        { "dabs_per_second", BrushSettingType::BRUSH_SETTING_DABS_PER_SECOND },
        { "gridmap_scale", BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE },
        { "gridmap_scale_x", BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_X },
        { "gridmap_scale_y", BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_Y },
        { "radius_by_random", BrushSettingType::BRUSH_SETTING_RADIUS_BY_RANDOM },
        { "speed1_slowness", BrushSettingType::BRUSH_SETTING_SPEED1_SLOWNESS },
        { "speed2_slowness", BrushSettingType::BRUSH_SETTING_SPEED2_SLOWNESS },
        { "speed1_gamma", BrushSettingType::BRUSH_SETTING_SPEED1_GAMMA },
        { "speed2_gamma", BrushSettingType::BRUSH_SETTING_SPEED2_GAMMA },
        { "offset_by_random", BrushSettingType::BRUSH_SETTING_OFFSET_BY_RANDOM },
        { "offset_y", BrushSettingType::BRUSH_SETTING_OFFSET_Y },
        { "offset_x", BrushSettingType::BRUSH_SETTING_OFFSET_X },
        { "offset_angle", BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE },
        { "offset_angle_asc", BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ASC },
        { "offset_angle_2", BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2 },
        { "offset_angle_2_asc", BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2_ASC },
        { "offset_angle_adj", BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ADJ },
        { "offset_multiplier", BrushSettingType::BRUSH_SETTING_OFFSET_MULTIPLIER },
        { "offset_by_speed", BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED },
        { "offset_by_speed_slowness", BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS },
        { "slow_tracking", BrushSettingType::BRUSH_SETTING_SLOW_TRACKING },
        { "slow_tracking_per_dab", BrushSettingType::BRUSH_SETTING_SLOW_TRACKING_PER_DAB },
        { "tracking_noise", BrushSettingType::BRUSH_SETTING_TRACKING_NOISE },
        { "color_h", BrushSettingType::BRUSH_SETTING_COLOR_H },
        { "color_s", BrushSettingType::BRUSH_SETTING_COLOR_S },
        { "color_v", BrushSettingType::BRUSH_SETTING_COLOR_V },
        { "restore_color", BrushSettingType::BRUSH_SETTING_RESTORE_COLOR },
        { "change_color_h", BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_H },
        { "change_color_l", BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_L },
        { "change_color_hsl_s", BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSL_S },
        { "change_color_v", BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_V },
        { "change_color_hsv_s", BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSV_S },
        { "smudge", BrushSettingType::BRUSH_SETTING_SMUDGE },
        { "smudge_length", BrushSettingType::BRUSH_SETTING_SMUDGE_LENGTH },
        { "smudge_radius_log", BrushSettingType::BRUSH_SETTING_SMUDGE_RADIUS_LOG },
        { "eraser", BrushSettingType::BRUSH_SETTING_ERASER },
        { "stroke_threshold", BrushSettingType::BRUSH_SETTING_STROKE_THRESHOLD },
        { "stroke_duration_logarithmic", BrushSettingType::BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC },
        { "stroke_holdtime", BrushSettingType::BRUSH_SETTING_STROKE_HOLDTIME },
        { "custom_input", BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT },
        { "custom_input_slowness", BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS },
        { "elliptical_dab_ratio", BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_RATIO },
        { "elliptical_dab_angle", BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE },
        { "direction_filter", BrushSettingType::BRUSH_SETTING_DIRECTION_FILTER },
        { "lock_alpha", BrushSettingType::BRUSH_SETTING_LOCK_ALPHA },
        { "colorize", BrushSettingType::BRUSH_SETTING_COLORIZE },
        { "snap_to_pixel", BrushSettingType::BRUSH_SETTING_SNAP_TO_PIXEL },
        { "pressure_gain_log", BrushSettingType::BRUSH_SETTING_PRESSURE_GAIN_LOG },
    };

    auto value = map.find(identifier);
    if (value != map.end()) {
        return value.value();
    }
    return BrushSettingType::BRUSH_SETTINGS_COUNT;
}

QString inline getBrushSettingIdentifier(const BrushSettingType type)
{
    switch(type)
    {
    case BrushSettingType::BRUSH_SETTING_OPAQUE: return "opaque";
    case BrushSettingType::BRUSH_SETTING_OPAQUE_MULTIPLY: return "opaque_multiply";
    case BrushSettingType::BRUSH_SETTING_OPAQUE_LINEARIZE: return "opaque_linearize";
    case BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC: return "radius_logarithmic";
    case BrushSettingType::BRUSH_SETTING_HARDNESS: return "hardness";
    case BrushSettingType::BRUSH_SETTING_SOFTNESS: return "softness";
    case BrushSettingType::BRUSH_SETTING_ANTI_ALIASING: return "anti_aliasing";
    case BrushSettingType::BRUSH_SETTING_DABS_PER_BASIC_RADIUS: return "dabs_per_basic_radius";
    case BrushSettingType::BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS: return "dabs_per_actual_radius";
    case BrushSettingType::BRUSH_SETTING_DABS_PER_SECOND: return "dabs_per_second";
    case BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE: return "gridmap_scale";
    case BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_X: return "gridmap_scale_x";
    case BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_Y: return "gridmap_scale_y";
    case BrushSettingType::BRUSH_SETTING_RADIUS_BY_RANDOM: return "radius_by_random";
    case BrushSettingType::BRUSH_SETTING_SPEED1_SLOWNESS: return "speed1_slowness";
    case BrushSettingType::BRUSH_SETTING_SPEED2_SLOWNESS: return "speed2_slowness";
    case BrushSettingType::BRUSH_SETTING_SPEED1_GAMMA: return "speed1_gamma";
    case BrushSettingType::BRUSH_SETTING_SPEED2_GAMMA: return "speed2_gamma";
    case BrushSettingType::BRUSH_SETTING_OFFSET_BY_RANDOM: return "offset_by_random";
    case BrushSettingType::BRUSH_SETTING_OFFSET_Y: return "offset_y";
    case BrushSettingType::BRUSH_SETTING_OFFSET_X: return "offset_x";
    case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE: return "offset_angle";
    case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ASC: return "offset_angle_asc";
    case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2: return "offset_angle_2";
    case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2_ASC: return "offset_angle_2_asc";
    case BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ADJ: return "offset_angle_adj";
    case BrushSettingType::BRUSH_SETTING_OFFSET_MULTIPLIER: return "offset_multiplier";
    case BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED: return "offset_by_speed";
    case BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS: return "offset_by_speed_slowness";
    case BrushSettingType::BRUSH_SETTING_SLOW_TRACKING: return "slow_tracking";
    case BrushSettingType::BRUSH_SETTING_SLOW_TRACKING_PER_DAB: return "slow_tracking_per_dab";
    case BrushSettingType::BRUSH_SETTING_TRACKING_NOISE: return "tracking_noise";
    case BrushSettingType::BRUSH_SETTING_COLOR_H: return "color_h";
    case BrushSettingType::BRUSH_SETTING_COLOR_S: return "color_s";
    case BrushSettingType::BRUSH_SETTING_COLOR_V: return "color_v";
    case BrushSettingType::BRUSH_SETTING_RESTORE_COLOR: return "restore_color";
    case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_H: return "change_color_h";
    case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_L: return "change_color_l";
    case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSL_S: return "change_color_hsl_s";
    case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_V: return "change_color_v";
    case BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSV_S: return "change_color_hsv_s";
    case BrushSettingType::BRUSH_SETTING_SMUDGE: return "smudge";
    case BrushSettingType::BRUSH_SETTING_SMUDGE_LENGTH: return "smudge_length";
    case BrushSettingType::BRUSH_SETTING_SMUDGE_RADIUS_LOG: return "smudge_radius_log";
    case BrushSettingType::BRUSH_SETTING_ERASER: return "eraser";
    case BrushSettingType::BRUSH_SETTING_STROKE_THRESHOLD: return "stroke_threshold";
    case BrushSettingType::BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC: return "stroke_duration_logarithmic";
    case BrushSettingType::BRUSH_SETTING_STROKE_HOLDTIME: return "stroke_holdtime";
    case BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT: return "custom_input";
    case BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS: return "custom_input_slowness";
    case BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_RATIO: return "elliptical_dab_ratio";
    case BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE: return "elliptical_dab_angle";
    case BrushSettingType::BRUSH_SETTING_DIRECTION_FILTER: return "direction_filter";
    case BrushSettingType::BRUSH_SETTING_LOCK_ALPHA: return "lock_alpha";
    case BrushSettingType::BRUSH_SETTING_COLORIZE: return "colorize";
    case BrushSettingType::BRUSH_SETTING_SNAP_TO_PIXEL: return "snap_to_pixel";
    case BrushSettingType::BRUSH_SETTING_PRESSURE_GAIN_LOG: return "pressure_gain_log";
    default: return "";
    }
}

QString inline getBrushInputIdentifier(const BrushInputType& type)
{
    switch(type)
    {
    case BrushInputType::BRUSH_INPUT_PRESSURE: return "pressure";
    case BrushInputType::BRUSH_INPUT_SPEED1: return "speed1";
    case BrushInputType::BRUSH_INPUT_SPEED2: return "speed2";
    case BrushInputType::BRUSH_INPUT_RANDOM: return "random";
    case BrushInputType::BRUSH_INPUT_STROKE: return "stroke";
    case BrushInputType::BRUSH_INPUT_DIRECTION: return "direction";
    case BrushInputType::BRUSH_INPUT_DIRECTION_ANGLE: return "direction_angle";
    case BrushInputType::BRUSH_INPUT_ATTACK_ANGLE: return "attack_angle";
    case BrushInputType::BRUSH_INPUT_TILT_DECLINATION: return "tilt_declination";
    case BrushInputType::BRUSH_INPUT_TILT_ASCENSION: return "tilt_ascension";
    case BrushInputType::BRUSH_INPUT_GRIDMAP_X: return "gridmap_x";
    case BrushInputType::BRUSH_INPUT_GRIDMAP_Y: return "gridmap_y";
    case BrushInputType::BRUSH_INPUT_BRUSH_RADIUS: return "brush_radius";
    case BrushInputType::BRUSH_INPUT_CUSTOM: return "custom";
    default: return "";
    }
}


QString inline getBrushInputName(const BrushInputType& type)
{
    switch(type)
    {
    case BrushInputType::BRUSH_INPUT_PRESSURE: return "Pressure";
    case BrushInputType::BRUSH_INPUT_SPEED1: return "Fine speed";
    case BrushInputType::BRUSH_INPUT_SPEED2: return "Gross speed";
    case BrushInputType::BRUSH_INPUT_RANDOM: return "Noise";
    case BrushInputType::BRUSH_INPUT_STROKE: return "Stroke";
    case BrushInputType::BRUSH_INPUT_DIRECTION: return "Direction";
    case BrushInputType::BRUSH_INPUT_DIRECTION_ANGLE: return "Direction 360";
    case BrushInputType::BRUSH_INPUT_ATTACK_ANGLE: return "Attack Angle";
    case BrushInputType::BRUSH_INPUT_TILT_DECLINATION: return "Declination";
    case BrushInputType::BRUSH_INPUT_TILT_ASCENSION: return "Ascension";
    case BrushInputType::BRUSH_INPUT_GRIDMAP_X: return "GridMap X";
    case BrushInputType::BRUSH_INPUT_GRIDMAP_Y: return "GridMap Y";
    case BrushInputType::BRUSH_INPUT_BRUSH_RADIUS: return "Base Brush Radius";
    case BrushInputType::BRUSH_INPUT_CUSTOM: return "Custom";
    default: return "";
    }
}

const QVector<BrushSettingType> allSettings = { BrushSettingType::BRUSH_SETTING_OPAQUE,
                                              BrushSettingType::BRUSH_SETTING_OPAQUE_MULTIPLY,
                                              BrushSettingType::BRUSH_SETTING_OPAQUE_LINEARIZE,
                                              BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC,
                                              BrushSettingType::BRUSH_SETTING_HARDNESS,
                                              BrushSettingType::BRUSH_SETTING_SOFTNESS,
                                              BrushSettingType::BRUSH_SETTING_ANTI_ALIASING,
                                              BrushSettingType::BRUSH_SETTING_DABS_PER_BASIC_RADIUS,
                                              BrushSettingType::BRUSH_SETTING_DABS_PER_ACTUAL_RADIUS,
                                              BrushSettingType::BRUSH_SETTING_DABS_PER_SECOND,
                                              BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE,
                                              BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_X,
                                              BrushSettingType::BRUSH_SETTING_GRIDMAP_SCALE_Y,
                                              BrushSettingType::BRUSH_SETTING_RADIUS_BY_RANDOM,
                                              BrushSettingType::BRUSH_SETTING_SPEED1_SLOWNESS,
                                              BrushSettingType::BRUSH_SETTING_SPEED2_SLOWNESS,
                                              BrushSettingType::BRUSH_SETTING_SPEED1_GAMMA,
                                              BrushSettingType::BRUSH_SETTING_SPEED2_GAMMA,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_BY_RANDOM,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_Y,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_X,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ASC,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_2_ASC,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_ANGLE_ADJ,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_MULTIPLIER,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED,
                                              BrushSettingType::BRUSH_SETTING_OFFSET_BY_SPEED_SLOWNESS,
                                              BrushSettingType::BRUSH_SETTING_SLOW_TRACKING,
                                              BrushSettingType::BRUSH_SETTING_SLOW_TRACKING_PER_DAB,
                                              BrushSettingType::BRUSH_SETTING_TRACKING_NOISE,
                                              BrushSettingType::BRUSH_SETTING_COLOR_H,
                                              BrushSettingType::BRUSH_SETTING_COLOR_S,
                                              BrushSettingType::BRUSH_SETTING_COLOR_V,
                                              BrushSettingType::BRUSH_SETTING_RESTORE_COLOR,
                                              BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_H,
                                              BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_L,
                                              BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSL_S,
                                              BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_V,
                                              BrushSettingType::BRUSH_SETTING_CHANGE_COLOR_HSV_S,
                                              BrushSettingType::BRUSH_SETTING_SMUDGE,
                                              BrushSettingType::BRUSH_SETTING_SMUDGE_LENGTH,
                                              BrushSettingType::BRUSH_SETTING_SMUDGE_RADIUS_LOG,
                                              BrushSettingType::BRUSH_SETTING_ERASER,
                                              BrushSettingType::BRUSH_SETTING_STROKE_THRESHOLD,
                                              BrushSettingType::BRUSH_SETTING_STROKE_DURATION_LOGARITHMIC,
                                              BrushSettingType::BRUSH_SETTING_STROKE_HOLDTIME,
                                              BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT,
                                              BrushSettingType::BRUSH_SETTING_CUSTOM_INPUT_SLOWNESS,
                                              BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_RATIO,
                                              BrushSettingType::BRUSH_SETTING_ELLIPTICAL_DAB_ANGLE,
                                              BrushSettingType::BRUSH_SETTING_DIRECTION_FILTER,
                                              BrushSettingType::BRUSH_SETTING_LOCK_ALPHA,
                                              BrushSettingType::BRUSH_SETTING_COLORIZE,
                                              BrushSettingType::BRUSH_SETTING_SNAP_TO_PIXEL,
                                              BrushSettingType::BRUSH_SETTING_PRESSURE_GAIN_LOG
                                            };
#endif // BRUSHSETTING_H
