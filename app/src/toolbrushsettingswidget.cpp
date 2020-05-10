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
    QString toolGroupKey = QString(SETTING_MPBRUSHSETTING)
                           .arg(mEditor->tools()->currentTool()->typeName()).toLower();

    QSettings settings(PENCIL2D, PENCIL2D);
    auto groups = settings.childGroups();

    settings.beginGroup(toolGroupKey);

    setupSettings(currentToolType);

    containerWidget->setLayout(mMainVerticalLayout);

    mMainHorizontalLayout->addWidget(containerWidget);

    mMainVerticalLayout->setContentsMargins(0,0,0,0);
    mMainHorizontalLayout->setContentsMargins(0,0,0,0);

    setLayout(mMainHorizontalLayout);
}

void ToolBrushSettingsWidget::updateUI()
{
    setupSettingsForTool(mEditor->tools()->currentTool()->type());
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

    for (BrushSetting setting : tool->enabledBrushSettings()) {
        BrushSettingWidget* settingWidget = new BrushSettingWidget(setting.name, setting.type, setting.min, setting.max, this);
        mMainVerticalLayout->addWidget(settingWidget);
        settingWidget->setCore(mEditor);
        settingWidget->initUI();

        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, toolMan, &ToolManager::setMPBrushSetting);
        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);
        mBrushSettingWidgets.insert(static_cast<int>(settingWidget->setting()), settingWidget);

        QString toolSetting = QString(SETTING_MPBRUSHSETTING)
                              .arg(mEditor->tools()->currentTool()->typeName()).toLower();

        QSettings settings(PENCIL2D, PENCIL2D);

        settings.beginGroup(toolSetting);
            settings.beginGroup(getBrushSettingIdentifier(setting.type));
            settings.setValue("visible", true);
            settings.setValue("name", setting.name);
            settings.setValue("min", setting.min);
            settings.setValue("max", setting.max);
            settings.endGroup();
        settings.endGroup();
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

    QString toolGroup = QString(SETTING_MPBRUSHSETTING)
                        .arg(mEditor->tools()->currentTool()->typeName()).toLower();

    settings.beginGroup(toolGroup);

    for (QString key : settings.childGroups())
    {
        settings.beginGroup(key);

        bool show = settings.value("visible").toBool();
        QString name = settings.value("name").toString();
        qreal min = settings.value("min").toReal();
        qreal max = settings.value("max").toReal();
        settings.endGroup();

        if (show) {
            qDebug() << key;
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
    QString toolGroupKey = QString(SETTING_MPBRUSHSETTING)
                           .arg(mEditor->tools()->currentTool()->typeName()).toLower();

    QSettings settings(PENCIL2D, PENCIL2D);
    auto groups = settings.childGroups();

    if (!groups.contains(toolGroupKey)) {
        setupDefaultSettings();
    } else {
        setupSettingsForTool(toolType);
    }
}

void ToolBrushSettingsWidget::updateSetting(qreal value, BrushSettingType setting)
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

    QHash<int, BrushChanges> changes;
    changes.insert(static_cast<int>(setting), change);
    mEditor->brushes()->applyChangesToBrushFile(changes);

    emit notifyBrushSettingUpdated(value, setting);
}

void ToolBrushSettingsWidget::toggleSetting(QString name, BrushSettingType setting, qreal min, qreal max, bool visible)
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
