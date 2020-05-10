#ifndef TOOLBRUSHSETTINGSWIDGET_H
#define TOOLBRUSHSETTINGSWIDGET_H

#include "basedockwidget.h"

#include "brushsetting.h"
#include "pencildef.h"

#include <QHash>

class QVBoxLayout;
class QHBoxLayout;
class BrushSettingWidget;
class QSpacerItem;
class QScrollArea;
class BaseTool;

class ToolBrushSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    ToolBrushSettingsWidget(QWidget* parent = nullptr);
    ~ToolBrushSettingsWidget() override;

    void initUI();
    void updateUI();

    void setCore(Editor* editor) { mEditor = editor; }

    void toggleSetting(QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
    void updateSetting(qreal value, BrushSettingType setting);

    void setupSettings(ToolType toolType);

Q_SIGNALS:
    void notifySettingsUpdated();
    void notifyBrushSettingUpdated(qreal value, BrushSettingType setting);

    void settingUpdated(BrushSettingType setting, qreal value);

private:
    void setupSettingsForTool(ToolType toolType);
    void setupDefaultSettings();
    void clearSettings();
    void didUpdateSetting(qreal value, BrushSettingType setting);

    QVBoxLayout* mMainVerticalLayout = nullptr;
    QHBoxLayout* mMainHorizontalLayout = nullptr;
    QScrollArea* mScrollArea = nullptr;
    Editor* mEditor = nullptr;

    QHash<int, BrushSettingWidget*> mBrushSettingWidgets;

    QSpacerItem* mSpacer = nullptr;
};

#endif // TOOLBRUSHSETTINGSWIDGET_H
