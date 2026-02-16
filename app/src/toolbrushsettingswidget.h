#ifndef TOOLBRUSHSETTINGSWIDGET_H
#define TOOLBRUSHSETTINGSWIDGET_H

#include "basewidget.h"

#include "brushsetting.h"
#include "pencildef.h"
#include "mpbrushsettingcategories.h"

#include <QHash>

class QVBoxLayout;
class QHBoxLayout;
class DefaultBrushSettingWidget;
class QSpacerItem;
class QScrollArea;
class BaseTool;
class Editor;
class StrokeTool;

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

Q_SIGNALS:
    void brushSettingChanged(qreal value, BrushSettingType setting);

private:

    void updateToolConnections(StrokeTool* tool);
    void addBrushSetting(QString settingName, BrushSettingType type, qreal min, qreal max);

    void setupSettingsForTool(ToolType toolType);
    void setupDefaultSettings();
    void clearSettings();
    void didUpdateSetting(qreal value, BrushSettingType setting);

    void addSettingToCategory(BrushSettingCategoryType settingCategoryType, DefaultBrushSettingWidget* settingWidget);
    void insertSettingAfter(BrushSettingCategoryType categoryType, DefaultBrushSettingWidget* settingWidget);

    QVBoxLayout* mMainVerticalLayout = nullptr;
    QHBoxLayout* mMainHorizontalLayout = nullptr;
    QVBoxLayout* mBrushSettingsLayout = nullptr;
    QScrollArea* mScrollArea = nullptr;
    Editor* mEditor = nullptr;

    QMap<int, DefaultBrushSettingWidget*> mBrushSettingWidgets;

    QSpacerItem* mSpacer = nullptr;
};

#endif // TOOLBRUSHSETTINGSWIDGET_H
