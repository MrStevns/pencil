#include "toolbrushsettingswidget.h"

#include <QSettings>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>

#include "editor.h"
#include "basetool.h"

#include "mpbrushmanager.h"
#include "layermanager.h"
#include "toolmanager.h"
#include "brushsettingwidget.h"
#include "brushsetting.h"

ToolBrushSettingsWidget::ToolBrushSettingsWidget(QWidget* parent) : QWidget(parent)
{
    setWindowTitle(tr("Brush settings", "Window title of brush settings panel"));
    setObjectName("brushSettingsWidget");
}

ToolBrushSettingsWidget::~ToolBrushSettingsWidget()
{
    clearSettings();
}

void ToolBrushSettingsWidget::initUI()
{
    QWidget* containerWidget = new QWidget(this);

    mMainVerticalLayout = new QVBoxLayout();
    mMainHorizontalLayout = new QHBoxLayout();

    mSpacer = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto toolMan = mEditor->tools();

    ToolType currentToolType = toolMan->currentTool()->type();

    setupSettings(currentToolType);

    containerWidget->setLayout(mMainVerticalLayout);

    mMainHorizontalLayout->addWidget(containerWidget);

    mMainVerticalLayout->setContentsMargins(0,0,0,0);
    mMainHorizontalLayout->setContentsMargins(0,0,0,0);

    setLayout(mMainHorizontalLayout);
}

void ToolBrushSettingsWidget::updateUI()
{
    setupSettings(mEditor->currentTool()->type());
}

void ToolBrushSettingsWidget::resetSettings()
{
    QSettings settings(PENCIL2D, PENCIL2D);

    QString toolSetting = mEditor->brushes()->currentToolBrushIdentifier();

    settings.beginGroup(toolSetting);
    settings.remove("");
    settings.endGroup();

    setupDefaultSettings();
}

void ToolBrushSettingsWidget::setupDefaultSettings()
{
    mMainVerticalLayout->removeItem(mSpacer);
    if (!mBrushSettingWidgets.isEmpty()) {
        clearSettings();
    }

    BaseTool* tool = mEditor->tools()->currentTool();
    ToolManager* toolMan = mEditor->tools();

    qDebug() << "setup settings for :" << tool->typeName();
    QSettings settings(PENCIL2D, PENCIL2D);

    QString configGroupForBrush = mEditor->brushes()->currentToolBrushIdentifier();

    for (int toolSettingNum = 0; toolSettingNum < static_cast<int>(BrushSettingType::BRUSH_SETTINGS_COUNT); toolSettingNum++)
    {
        BrushSettingType settingType = static_cast<BrushSettingType>(toolSettingNum);

        settings.beginGroup(configGroupForBrush);
            settings.beginGroup(getBrushSettingIdentifier(settingType));
            settings.setValue("visible", false);
            settings.endGroup();
        settings.endGroup();
    }

    for (BrushSetting setting : tool->enabledBrushSettings()) {

        settings.beginGroup(configGroupForBrush);
            settings.beginGroup(getBrushSettingIdentifier(setting.type));
            settings.setValue("visible", true);
            settings.setValue("name", setting.name);
            settings.setValue("min", setting.min);
            settings.setValue("max", setting.max);
            settings.endGroup();
        settings.endGroup();

        BrushSettingWidget* settingWidget = new BrushSettingWidget(setting.name, setting.type, setting.min, setting.max, this);
        mMainVerticalLayout->addWidget(settingWidget);
        settingWidget->setCore(mEditor);
        settingWidget->initUI();

        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, toolMan, &ToolManager::setMPBrushSetting);
        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);
        mBrushSettingWidgets.insert(static_cast<int>(settingWidget->setting()), settingWidget);
    }

    mMainVerticalLayout->addItem(mSpacer);
}

void ToolBrushSettingsWidget::setupSettingsForTool(ToolType toolType)
{
    mMainVerticalLayout->removeItem(mSpacer);
    if (!mBrushSettingWidgets.isEmpty()) {
        clearSettings();
    }

    ToolManager* toolMan = mEditor->tools();
    QSettings settings(PENCIL2D, PENCIL2D);
    auto groups = settings.childGroups();

    QString toolGroup = mEditor->brushes()->currentToolBrushIdentifier();

    settings.beginGroup(toolGroup);

    QListIterator<QString> settingsIt(settings.childGroups());
    settingsIt.toBack();
    while(settingsIt.hasPrevious())
    {
        QString key = settingsIt.previous();
        settings.beginGroup(key);

        bool show = settings.value("visible").toBool();
        QString name = settings.value("name").toString();
        qreal min = settings.value("min").toReal();
        qreal max = settings.value("max").toReal();
        settings.endGroup();

        if (show) {
            BrushSettingWidget* settingWidget = new BrushSettingWidget(name, getBrushSetting(key), min, max, this);
            mMainVerticalLayout->addWidget(settingWidget);
            settingWidget->setCore(mEditor);
            settingWidget->initUI();

            connect(settingWidget, &BrushSettingWidget::brushSettingChanged, toolMan, &ToolManager::setMPBrushSetting);
            connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);
            mBrushSettingWidgets.insert(static_cast<int>(settingWidget->setting()), settingWidget);
        }
    }

    mMainVerticalLayout->addItem(mSpacer);
}

void ToolBrushSettingsWidget::setupSettings(ToolType toolType)
{
    if (!mEditor->brushes()->brushLoaded()) { return; }

    QString toolGroupKey = mEditor->brushes()->currentToolBrushIdentifier();

    QSettings settings(PENCIL2D, PENCIL2D);
    auto groups = settings.childGroups();

    if (!groups.contains(toolGroupKey)) {
        setupDefaultSettings();
    } else {
        setupSettingsForTool(toolType);
    }
}

void ToolBrushSettingsWidget::updateFromUnmappedSetting(qreal value, BrushSettingType setting)
{
    qDebug() << "notify setting was updated";
    auto settingWidget = mBrushSettingWidgets.find(static_cast<int>(setting)).value();

    if (mBrushSettingWidgets.contains(static_cast<int>(setting))) {
        settingWidget->setValueFromUnmapped(value);
    }
}

void ToolBrushSettingsWidget::setValue(qreal value, BrushSettingType setting)
{
    qDebug() << "notify setting was updated";
    auto settingWidget = mBrushSettingWidgets.find(static_cast<int>(setting)).value();

    if (mBrushSettingWidgets.contains(static_cast<int>(setting))) {
        settingWidget->setValue(value);
    }
}

void ToolBrushSettingsWidget::didUpdateSetting(qreal value, BrushSettingType setting)
{
    BrushChanges change;
    change.baseValue = value;
    change.settingsType = setting;

    QHash<BrushSettingType, BrushChanges> changes;
    changes.insert(setting, change);
    mEditor->brushes()->backupBrushSettingChanges(setting, value);
    mEditor->brushes()->applyChangesToBrushFile(false);

    emit brushSettingChanged(value, setting);
}

void ToolBrushSettingsWidget::setVisibleState(QString name, BrushSettingType setting, qreal min, qreal max, bool visible)
{
    if (visible && !mBrushSettingWidgets.contains(static_cast<int>(setting))) {
        BrushSettingWidget* settingWidget = new BrushSettingWidget(name, setting, min, max, this);
        mBrushSettingWidgets.insert(static_cast<int>(setting), settingWidget);
        mMainVerticalLayout->addWidget(settingWidget);

        settingWidget->setCore(mEditor);
        settingWidget->initUI();

        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, mEditor->tools(), &ToolManager::setMPBrushSetting);
        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);

    } else if (!visible && mBrushSettingWidgets.contains(static_cast<int>(setting))) {
        auto settingWidget = mBrushSettingWidgets.take(static_cast<int>(setting));
        mMainVerticalLayout->removeWidget(settingWidget);
        settingWidget->deleteLater();
    }

    mMainVerticalLayout->removeItem(mSpacer);
    mMainVerticalLayout->addItem(mSpacer);
}

void ToolBrushSettingsWidget::clearSettings()
{
    QHashIterator<int, BrushSettingWidget*> it(mBrushSettingWidgets);

    while (it.hasNext()) {
        it.next();

        auto widget = mBrushSettingWidgets.take(it.key());
        mMainVerticalLayout->removeWidget(widget);
        widget->deleteLater();
    }

    mMainVerticalLayout->removeItem(mSpacer);
}
