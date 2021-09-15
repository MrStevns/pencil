#ifndef MPBRUSHSETTINGCATEGORIES_H
#define MPBRUSHSETTINGCATEGORIES_H

#include "brushsetting.h"
#include "pencildef.h"

enum class BrushSettingCategoryType {
    Active = 0,
    Basic,
    Advanced,
    Opacity,
    Dab,
    Random,
    Speed,
    Offset,
    Tracking,
    Color,
    Smudge,
    Eraser,
    Stroke,
    Custom,
    Elliptical,
    Other
};

struct BrushSettingCategory {
    QList<BrushSetting> settings;
    BrushSettingCategoryType categoryType;

    BrushSettingCategory() { }

    BrushSettingCategory(BrushSettingCategoryType categoryType, QList<BrushSetting> settings)
    {
        this->settings = settings;
        this->categoryType = categoryType;
    }
};

class MPBrushSettingCategories
{
public:
    MPBrushSettingCategories();

    QList<BrushSetting> defaultBrushListForTool(ToolType type);

    QList<BrushSettingCategory> allBrushSettings() const;

    QList<BrushSettingCategory> basicBrushSettings() const;

    BrushSettingCategory opacityBrushSettings() const;
    BrushSettingCategory dabBrushSettings() const;
    BrushSettingCategory randomBrushSettings() const;
    BrushSettingCategory speedBrushSettings() const;
    BrushSettingCategory offsetBrushSettings() const;
    BrushSettingCategory trackingBrushSettings() const;
    BrushSettingCategory colorBrushSettings() const;
    BrushSettingCategory smudgeBrushSettings() const;
    BrushSettingCategory eraserBrushSettings() const;
    BrushSettingCategory strokeBrushSettings() const;
    BrushSettingCategory customBrushSettings() const;
    BrushSettingCategory ellipticalBrushSettings() const;
    BrushSettingCategory otherBrushSettings() const;

    BrushSettingCategory categoryForType(const BrushSettingCategoryType type) const;
    BrushSettingCategory categoryForSetting(const BrushSettingType type) const;

    BrushSettingType previousSettingInCategory(const BrushSettingType type) const;
    BrushSettingType nextSettingInCategory(const BrushSettingType type) const;
    int indexOfSetting(const BrushSettingType type, const BrushSettingCategory category) const;
};

inline uint qHash(BrushSettingCategoryType key, uint seed)
{
    return ::qHash(static_cast<uint>(key), seed);
}

#endif // MPBRUSHSETTINGCATEGORIES_H
