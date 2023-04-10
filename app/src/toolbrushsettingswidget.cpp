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
#include "mpbrushsettingcategories.h"

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
    mBrushSettingsLayout = new QVBoxLayout();

    mSpacer = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto toolMan = mEditor->tools();

    ToolType currentToolType = toolMan->currentTool()->type();

    setupSettings(currentToolType);

    containerWidget->setLayout(mMainVerticalLayout);
    mMainVerticalLayout->addLayout(mBrushSettingsLayout);

    mMainHorizontalLayout->addWidget(containerWidget);

    mBrushSettingsLayout->setContentsMargins(0,0,0,0);
    mMainVerticalLayout->setContentsMargins(0,0,0,0);
    mMainHorizontalLayout->setContentsMargins(0,0,0,0);
    mMainVerticalLayout->addItem(mSpacer);

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
    if (!mBrushSettingWidgets.isEmpty()) {
        clearSettings();
    }

    BaseTool* tool = mEditor->tools()->currentTool();
    ToolManager* toolMan = mEditor->tools();

    qDebug() << "setup settings for :" << tool->typeName();
    QSettings settings(PENCIL2D, PENCIL2D);

    QString configGroupForBrush = mEditor->brushes()->currentToolBrushIdentifier();

    MPBrushSettingCategories config;
    for (BrushSetting& setting : config.defaultBrushListForTool(tool->type())) {

        settings.beginGroup(configGroupForBrush);
            settings.beginGroup(getBrushSettingIdentifier(setting.type));
            settings.setValue("visible", true);
            settings.setValue("name", setting.name);
            settings.setValue("min", setting.min);
            settings.setValue("max", setting.max);
            settings.endGroup();
        settings.endGroup();

        BrushSettingWidget* settingWidget = new BrushSettingWidget(setting.name, setting.type, setting.min, setting.max, this);
        mBrushSettingsLayout->addWidget(settingWidget);
        settingWidget->setCore(mEditor);
        settingWidget->initUI();

        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, toolMan, &ToolManager::setMPBrushSetting);
        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);
        mBrushSettingWidgets.insert(static_cast<int>(settingWidget->setting()), settingWidget);
    }
}

void ToolBrushSettingsWidget::setupSettingsForTool(ToolType toolType)
{
    if (!mBrushSettingWidgets.isEmpty()) {
        clearSettings();
    }

    ToolManager* toolMan = mEditor->tools();
    QSettings pencilSettings(PENCIL2D, PENCIL2D);
    auto groups = pencilSettings.childGroups();

    QString toolGroup = mEditor->brushes()->currentToolBrushIdentifier();

    pencilSettings.beginGroup(toolGroup);

    MPBrushSettingCategories settingListConfig;

    for (const auto &brushCategory : settingListConfig.allBrushSettings() ) {
        for (const auto &brushSetting : brushCategory.settings) {

            QListIterator<QString> settingsIt(pencilSettings.childGroups());
            while(settingsIt.hasNext()) {
                QString key = settingsIt.next();
                pencilSettings.beginGroup(key);

                bool show = pencilSettings.value("visible").toBool();
                QString name = pencilSettings.value("name").toString();
                qreal min = pencilSettings.value("min").toReal();
                qreal max = pencilSettings.value("max").toReal();
                pencilSettings.endGroup();
                if (brushSetting.type == getBrushSetting(key)) {
                    if (show) {
                        BrushSettingWidget* settingWidget = new BrushSettingWidget(name, brushSetting.type, min, max, this);
                        mBrushSettingsLayout->addWidget(settingWidget);
                        settingWidget->setCore(mEditor);
                        settingWidget->initUI();

                        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, toolMan, &ToolManager::setMPBrushSetting);
                        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);
                        mBrushSettingWidgets.insert(static_cast<int>(settingWidget->setting()), settingWidget);
                    }
                    break;
                }
            }
        }
    }
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

void ToolBrushSettingsWidget::setVisibleState(BrushSettingCategoryType settingCategoryType, QString name, BrushSettingType settingType, qreal min, qreal max, bool visible)
{
    Q_ASSERT(!mBrushSettingWidgets.isEmpty());
    if (visible && !mBrushSettingWidgets.contains(static_cast<int>(settingType))) {

        MPBrushSettingCategories mpSettingCategories;
        auto listOfCategories = mpSettingCategories.allBrushSettings();

        BrushSettingWidget* settingWidget = new BrushSettingWidget(name, settingType, min, max, this);

        bool hasCategory = false;
        for (auto setting : mpSettingCategories.categoryForSetting(settingType).settings) {
            if (!mBrushSettingWidgets.contains(static_cast<int>(setting.type))) { continue; }

            hasCategory = true;
            break;
        }

        if (hasCategory) {
            addSettingToCategory(settingCategoryType, settingWidget);
        } else {
            insertSettingAfter(settingCategoryType, settingWidget);
        }

        mBrushSettingWidgets.insert(static_cast<int>(settingType), settingWidget);

        settingWidget->setCore(mEditor);
        settingWidget->initUI();

        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, mEditor->tools(), &ToolManager::setMPBrushSetting);
        connect(settingWidget, &BrushSettingWidget::brushSettingChanged, this, &ToolBrushSettingsWidget::didUpdateSetting);

    } else {
        auto settingWidget = mBrushSettingWidgets.take(static_cast<int>(settingType));
        mBrushSettingsLayout->removeWidget(settingWidget);
        settingWidget->deleteLater();
    }
}

void ToolBrushSettingsWidget::insertSettingAfter(BrushSettingCategoryType categoryType, BrushSettingWidget* settingWidget)
{
    MPBrushSettingCategories mpSettingCategories;

    int newCategoryTypeIndex = static_cast<int>(categoryType);
    int newIndexAfter = 0;
    for (BrushSettingWidget* widgetInList : findChildren<BrushSettingWidget*>()) {
        auto categoryForWidget = mpSettingCategories.categoryForSetting(widgetInList->setting());

        int categoryIndexOfWidget = static_cast<int>(categoryForWidget.categoryType);

        if (categoryIndexOfWidget < newCategoryTypeIndex && widgetInList != settingWidget) {
            newIndexAfter++;
        }
    }
    mBrushSettingsLayout->insertWidget(newIndexAfter, settingWidget);
}

void ToolBrushSettingsWidget::addSettingToCategory(BrushSettingCategoryType settingCategoryType, BrushSettingWidget* settingWidget)
{
    int categoryTypeIndex = static_cast<int>(settingCategoryType);
    int newIndex = -1;
    MPBrushSettingCategories mpSettingCategories;
    for (BrushSettingWidget* widgetInList : findChildren<BrushSettingWidget*>()) {

        auto categoryForWidget = mpSettingCategories.categoryForSetting(widgetInList->setting());

        int categoryIndexOfWidget = static_cast<int>(categoryForWidget.categoryType);

        if (categoryIndexOfWidget == categoryTypeIndex && widgetInList != settingWidget) {
            if (newIndex == -1) {
                // Locate first index in category
                newIndex = mBrushSettingsLayout->indexOf(widgetInList);
            }

            int indexOfNewSetting = mpSettingCategories.indexOfSetting(settingWidget->setting(), categoryForWidget);

            // Add indexes until we find the position of the new setting
            if (mpSettingCategories.indexOfSetting(widgetInList->setting(), categoryForWidget) < indexOfNewSetting) {
                newIndex++;
            }
        }
    }

    mBrushSettingsLayout->insertWidget(newIndex, settingWidget);
}

void ToolBrushSettingsWidget::clearSettings()
{
    QMapIterator<int, BrushSettingWidget*> it(mBrushSettingWidgets);

    while (it.hasNext()) {
        it.next();

        auto widget = mBrushSettingWidgets.take(it.key());
        mBrushSettingsLayout->removeWidget(widget);
        widget->deleteLater();
    }
}
