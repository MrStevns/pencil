#include "mpbrushsettingcategories.h"

#include "QHash"
#include "QList"

MPBrushSettingCategories::MPBrushSettingCategories()
{

}

QList<BrushSetting> MPBrushSettingCategories::defaultBrushListForTool(ToolType type)
{
    switch(type)
    {
    case ToolType::BRUSH:
    case ToolType::ERASER:
        return { RadiusLog,
                 Hardness,
                 PressureGain };
    case ToolType::PEN:
        return { RadiusLog,
                 PressureGain,
                 AntiAliasing
        };
    case ToolType::PENCIL:
        return { RadiusLog,
                 PressureGain };
    case ToolType::SMUDGE:
        return { RadiusLog,
                 Hardness,
                 Smudge };
    case ToolType::POLYLINE:
        return { RadiusLog,
                 Hardness,
                 Opacity };
    default:
        return { };
    }
}

QList<BrushSettingCategory> MPBrushSettingCategories::basicBrushSettings() const
{
    return { BrushSettingCategory(BrushSettingCategoryType::Opacity, { Opacity }),
                BrushSettingCategory(BrushSettingCategoryType::Dab, { RadiusLog , Hardness }),
                BrushSettingCategory(BrushSettingCategoryType::Other, { PressureGain, AntiAliasing }) };
}

BrushSettingCategory MPBrushSettingCategories::opacityBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Opacity, { Opacity, OpacityMultiply, OpacityLinearize });
}

BrushSettingCategory MPBrushSettingCategories::dabBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Dab, { RadiusLog,
             Hardness,
             DabsPerBasicRadius,
             DabsPerActualRadius,
             DabsPerSecond,
             DabScale, DabScaleX, DabScaleY });
}

BrushSettingCategory MPBrushSettingCategories::randomBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Random, { RadiusRandom });
}

BrushSettingCategory MPBrushSettingCategories::speedBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Speed, { SpeedStart, SpeedEnd, SpeedGammaStart, SpeedGammaEnd });
}

BrushSettingCategory MPBrushSettingCategories::offsetBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Offset, {
                                    OffsetRandom,
                                    OffsetX,
                                    OffsetY,
                                    OffsetAngleLeft,
                                    OffsetAngleLeftAscend,
                                    OffsetAngleRight,
                                    OffsetAngleRightAscend,
                                    OffsetAngleAdjecent,
                                    OffsetMultiplier,
                                    OffsetBySpeed,
                                    OffsetSpeedSlowness });
}

BrushSettingCategory MPBrushSettingCategories::trackingBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Tracking, { SlowTracking, SlowTrackingPerDab, TrackingNoise });
}

BrushSettingCategory MPBrushSettingCategories::colorBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Color, { ChangeColorHue,
                ChangeColorLightness,
                ChangeColorHLSSaturation,
                ChangeColorValue,
                ChangeColorHSVSaturation,
                RestoreColor });
}

BrushSettingCategory MPBrushSettingCategories::smudgeBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Smudge, { Smudge, SmudgeLength, SmudgeRadius });
}

BrushSettingCategory MPBrushSettingCategories::eraserBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Eraser, { Eraser });
}

BrushSettingCategory MPBrushSettingCategories::strokeBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Stroke, { StrokeThreshold, StrokeDuration, StrokeHoldTime });
}

BrushSettingCategory MPBrushSettingCategories::customBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Custom, { CustomInput, CustomInputSlowness });
}

BrushSettingCategory MPBrushSettingCategories::ellipticalBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Elliptical, { EllepticalDabRatio, EllepticalDabAngle });
}

BrushSettingCategory MPBrushSettingCategories::otherBrushSettings() const
{
    return BrushSettingCategory(BrushSettingCategoryType::Other, { PressureGain, AntiAliasing, LockAlpha, Colorize, SnapToPixel });
}

BrushSettingCategory MPBrushSettingCategories::categoryForType(const BrushSettingCategoryType type) const
{
    for (auto category : allBrushSettings()) {
        if (category.categoryType == type) {
            return category;
        }
    }
    // if you're here, you've forgotten to implement a category type
    Q_ASSERT(false);
    return { };
}

BrushSettingCategory MPBrushSettingCategories::categoryForSetting(const BrushSettingType type) const
{
    for (auto category : allBrushSettings()) {
        for (auto setting : category.settings) {
            if (type == setting.type) {
                return category;
            }
        }
    }

    // if you're here, you've forgotten to implement a category type
    Q_ASSERT(false);
    return { };
}

BrushSettingType MPBrushSettingCategories::previousSettingInCategory(const BrushSettingType type) const
{
    for (auto category : allBrushSettings()) {
        for (int i = 0; i < category.settings.count(); i++) {
            auto setting = category.settings[i];
            if (type == setting.type) {
                if (i <= 0) { return setting.type; };
                return category.settings[i-1].type;
            }
        }
    }

    // if you're here, you've forgotten to implement a category type
    Q_ASSERT(false);
    return { };
}

BrushSettingType MPBrushSettingCategories::nextSettingInCategory(const BrushSettingType type) const
{
    for (auto category : allBrushSettings()) {
        for (int i = 0; i < category.settings.count(); i++) {
            auto setting = category.settings[i];
            if (type == setting.type) {
                if (i+1 > category.settings.count()) { return setting.type; };
                return category.settings[i+1].type;
            }
        }
    }

    // if you're here, you've forgotten to implement a category type
    Q_ASSERT(false);
    return { };
}

int MPBrushSettingCategories::indexOfSetting(const BrushSettingType type, const BrushSettingCategory category) const
{
    for (int i = 0; i < category.settings.count(); i++) {
        auto setting = category.settings[i];
        if (type == setting.type) {
            return i;
        }
    }

    return -1;
}

QList<BrushSettingCategory> MPBrushSettingCategories::allBrushSettings() const
{
    return { opacityBrushSettings(),
                dabBrushSettings(),
                randomBrushSettings(),
                speedBrushSettings(),
                offsetBrushSettings(),
                trackingBrushSettings(),
                colorBrushSettings(),
                smudgeBrushSettings(),
                eraserBrushSettings(),
                strokeBrushSettings(),
                customBrushSettings(),
                ellipticalBrushSettings(),
                otherBrushSettings()};
}
