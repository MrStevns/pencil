#ifndef TOOLBRUSHSETTINGSWIDGET_H
#define TOOLBRUSHSETTINGSWIDGET_H

#include "basewidget.h"

#include "brushsetting.h"
#include "pencildef.h"
#include "mpbrushsettingcategories.h"

#include <QHash>

class QVBoxLayout;
class QHBoxLayout;
class BrushSettingWidget;
class QSpacerItem;
class QScrollArea;
class BaseTool;
class Editor;

class ToolBrushSettingsWidget : public BaseWidget
{
    Q_OBJECT
public:
    ToolBrushSettingsWidget(Editor* editor, QWidget* parent = nullptr);
    ~ToolBrushSettingsWidget() override;

    void initUI() override;
    void updateUI() override;

    void setVisibleState(BrushSettingCategoryType settingCategoryType, QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
    void setValue(qreal value, BrushSettingType setting);

    void setupSettings(ToolType toolType);
    void resetSettings();

    void updateFromUnmappedSetting(qreal value, BrushSettingType setting);

Q_SIGNALS:
    void brushSettingChanged(qreal value, BrushSettingType setting);

private:

    void addBrushSetting(QString settingName, BrushSettingType type, qreal min, qreal max);

    void setupSettingsForTool(ToolType toolType);
    void setupDefaultSettings();
    void clearSettings();
    void didUpdateSetting(qreal value, BrushSettingType setting);

    void addSettingToCategory(BrushSettingCategoryType settingCategoryType, BrushSettingWidget* settingWidget);
    void insertSettingAfter(BrushSettingCategoryType categoryType, BrushSettingWidget* settingWidget);

    QVBoxLayout* mMainVerticalLayout = nullptr;
    QHBoxLayout* mMainHorizontalLayout = nullptr;
    QVBoxLayout* mBrushSettingsLayout = nullptr;
    QScrollArea* mScrollArea = nullptr;
    Editor* mEditor = nullptr;

    QMap<int, BrushSettingWidget*> mBrushSettingWidgets;

    QSpacerItem* mSpacer = nullptr;
};

#endif // TOOLBRUSHSETTINGSWIDGET_H
