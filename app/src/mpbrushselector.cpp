/* brushlib - The MyPaint Brush Library (demonstration project)
 * Copyright (C) 2013 POINTCARRE SARL / Sebastien Leon email: sleon at pointcarre.com
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "mpbrushselector.h"

#include <QDir>
#include <QListWidget>
#include <QTabWidget>
#include <QLayout>
#include <QToolButton>
#include <QMessageBox>

#include <QJsonParseError>
#include <QJsonDocument>

#include <QSettings>
#include <QDebug>

#include "brushsetting.h"
#include "pencilerror.h"
#include "editor.h"
#include "toolmanager.h"
#include "combobox.h"
#include <errordialog.h>

#include "preferencemanager.h"
#include "mpbrushmanager.h"

#include "mpbrushutils.h"
#include "mpbrushconfigurator.h"
#include "mpbrushpresetswidget.h"


MPBrushSelector::MPBrushSelector(QWidget *parent)
    : BaseDockWidget(parent)
{
    setWindowTitle(tr("Brush Selector", "Window title of mypaint brush selector"));
    setObjectName("MPBrushSelector");

    QWidget* containerWidget = new QWidget(parent);
    mVLayout = new QVBoxLayout();

    mVLayout->setContentsMargins(2,2,2,2);
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(2,2,2,2);

    mPresetComboBox = new ComboBox();

    QToolButton* configuratorButton = new QToolButton(this);

    configuratorButton->setText(tr("Config"));
    configuratorButton->setToolTip(tr("Open brush configurator window"));

    mBrushListWidget = new QListWidget(this);
    mBrushListWidget->setUniformItemSizes(true);
    mBrushListWidget->setViewMode        (QListView::IconMode);
    mBrushListWidget->setResizeMode      (QListView::Adjust);
    mBrushListWidget->setMovement        (QListView::Static);
    mBrushListWidget->setFlow            (QListView::LeftToRight);
    mBrushListWidget->setSelectionMode   (QAbstractItemView::SingleSelection);
    mBrushListWidget->setIconSize        (QSize(ICON_SZ,ICON_SZ));

    connect(mBrushListWidget, &QListWidget::itemClicked, this, &MPBrushSelector::itemClicked);

    mTopAreaWidget = new QWidget(this);
    mVLayout->addWidget(mTopAreaWidget);
    mVLayout->addWidget(mBrushListWidget);
    mTopAreaWidget->setLayout(hLayout);

    hLayout->addWidget(configuratorButton);
    hLayout->addWidget(mPresetComboBox);

    mVLayout->setContentsMargins(8,2,8,8);

    containerWidget->setLayout(mVLayout);
    setWidget(containerWidget);

    connect(configuratorButton, &QToolButton::pressed, this, [=](void) { this->showBrushConfigurator(true); } );
    connect(mPresetComboBox, &ComboBox::activated, this, &MPBrushSelector::changeBrushPreset);
}

void MPBrushSelector::initUI()
{
    // First, we parse the "brushes.conf" file to fill m_brushLib
    loadPresets();
    loadToolBrushes(mEditor->tools()->currentTool()->type());
}

void MPBrushSelector::updateUI()
{

}

void MPBrushSelector::loadPresets()
{
    auto st = mEditor->brushes()->loadPresets();

    if (st.fail())
    {
        QMessageBox::information(this, st.title(), st.description() + "\n\n" + st.details().str());
        return;
    }

    QVector<MPBrushPreset> presets = mEditor->brushes()->presets();
    if (mPresetComboBox->count() > 0) {
        mPresetComboBox->clear();
    }

    for (const MPBrushPreset &preset : qAsConst(presets)) {
        mPresetComboBox->addItem(preset.name);
    }

    mEditor->brushes()->brushPreferences( [=] (QSettings& settings) {
        QString lastPreset = settings.value(SETTING_MPBRUSHPRESET).toString();

        if (!lastPreset.isEmpty()) {
            mPresetComboBox->setCurrentItemFrom(lastPreset);
        }

        loadBrushesIntoList();
    });
}

void MPBrushSelector::updateBrushList(QString brushName, QString brushPreset)
{
    mEditor->brushes()->setCurrentPresetName(brushPreset);

    loadPresets();
    setBrushAsSelected(brushName);
}

void MPBrushSelector::reloadBrushes()
{
    loadPresets();
}

void MPBrushSelector::loadBrushesIntoList()
{
    auto presets = mEditor->brushes()->presets();
    auto currentPresetName = mEditor->brushes()->currentPresetName();

    // TODO: should turn this into a map instead of iterating the list...
    for (const MPBrushPreset& pre : qAsConst(presets)) {
        if (pre.name == currentPresetName) {
            currentBrushPreset = pre;
            break;
        }
    }
    MPBrushPreset preset = currentBrushPreset;

    // clear existing widgets before re-adding
    mBrushListWidget->clear();

    int brushIndex = 0;
    const QString& presetPath = mEditor->brushes()->brushesPath() + "/" + currentPresetName;
    for (const QString& brush : preset.brushesForTool(mEditor->tools()->currentTool()->typeName().toLower())) {

        QIcon preview(presetPath + "/" + brush + BRUSH_PREVIEW_EXT);
        QListWidgetItem* p_item = new QListWidgetItem(preview, nullptr, mBrushListWidget, brushIndex);
        p_item->setToolTip(QString("%1").arg(brush));
        p_item->setData(Qt::UserRole, QVariant::fromValue(brush));
        brushIndex++;
    }
}

void MPBrushSelector::itemClicked(QListWidgetItem *itemWidget)
{
    QListWidget* listWidget = itemWidget->listWidget();
    if (listWidget)
    {
        // fine, let's read this one and emit the content to any receiver:
        const MPBrushPreset preset = currentBrushPreset;

        QString brushName = "";
        QString toolName = mEditor->tools()->currentTool()->typeName().toLower();
        int brushIndex = 0;
        for (const QString& brush : preset.brushesForTool(toolName)) {
            if (itemWidget->data(Qt::UserRole).toString().compare(brush, Qt::CaseInsensitive) == 0) {
                brushName = brush;
                break;
            }
            brushIndex++;
        }

        if (loadBrushFromFile(brushName)) {
            setLastUsedBrush(mEditor->tools()->currentTool()->type(), preset, brushName);
        }
    }
}

bool MPBrushSelector::loadBrushFromFile(const QString& brushName)
{
    auto brushMan = mEditor->brushes();
    Status status = brushMan->readBrushFromCurrentPreset(brushName);
    if (status == Status::OK)
    {
        emit brushSelected(); // Read the whole file and broadcast is as a char* buffer
        return true;
    } else {
        ErrorDialog errorDialog(status.title(), status.description(), status.details().str());
        errorDialog.exec();
        return false;
    }
}

void MPBrushSelector::onDidReloadBrushSettings()
{
    mEditor->loadBrush();
    emit didReloadBrush();
}

void MPBrushSelector::loadToolBrushes(ToolType toolType)
{
    const MPBrushPreset preset = currentBrushPreset;

    if (preset.isEmpty()) { return; }

    mEditor->brushes()->brushPreferences( [=] (QSettings& settings)
    {
        const QString& toolName = mEditor->getTool(toolType)->typeName().toLower();
        QString lastBrushForTool = settings.value(QString(SETTING_MPBRUSHFORTOOL+toolName+preset.name).toLower()).toString();

        QString brushName = lastBrushForTool;
        if (lastBrushForTool.isEmpty() || !preset.brushesForTool(toolName).contains(lastBrushForTool, Qt::CaseInsensitive)) {
            // No brush has been selected before, select the first

            if (!preset.brushesForTool(toolName).isEmpty()) {
                brushName = preset.brushesForTool(toolName).constFirst();
            } else {
                qDebug() << "NO BRUSH EXISTS FOR TOOL";
            }
        }

        if (!brushName.isEmpty()) {
            setBrushAsSelected(brushName);
            setLastUsedBrush(toolType, preset, brushName);
        }
    });
}

void MPBrushSelector::setLastUsedBrush(ToolType toolType, MPBrushPreset preset, QString brushName)
{
    QString toolName = mEditor->getTool(toolType)->typeName().toLower();
    mEditor->brushes()->brushPreferences( [=] (QSettings& settings) {
        settings.setValue((QString(SETTING_MPBRUSHFORTOOL+toolName+preset.name).toLower()), brushName.toLower());
    });
}

void MPBrushSelector::typeChanged(ToolType eToolMode)
{
    if (oldToolType == eToolMode) { return; }

    currentToolType = eToolMode;
    loadBrushesIntoList();
    loadToolBrushes(eToolMode);

    oldToolType = eToolMode;
}

void MPBrushSelector::setBrushAsSelected(QString brushName)
{
    if (mEditor->brushes()->presets().isEmpty()) return;

    int listWidgetIdx = 0;
    QListWidgetItem* itemWidget = nullptr;
    for (int i = 0; i < mBrushListWidget->count(); i++)
    {
        QListWidgetItem* item = mBrushListWidget->item(i);
        if (item->data(Qt::UserRole).toString().compare(brushName, Qt::CaseInsensitive) == 0) {
            itemWidget = mBrushListWidget->item(listWidgetIdx);
            break;
        }
        listWidgetIdx++;
    }

    // default one : we use the first tab page & the first item available:
    if (!itemWidget)
    {
        itemWidget = mBrushListWidget->item(0);
    }

    // Update GUI + load the brush (if any)
    if (itemWidget)
    {
        mBrushListWidget->setCurrentItem(itemWidget);
        itemClicked(itemWidget);
    }
}

void MPBrushSelector::showBrushConfigurator(bool show)
{
    if (mBrushConfiguratorWidget == nullptr) {
        mBrushConfiguratorWidget = new MPBrushConfigurator(this);
        mBrushConfiguratorWidget->setCore(mEditor);
        mBrushConfiguratorWidget->initUI();
        mBrushConfiguratorWidget->setAttribute(Qt::WA_DeleteOnClose);

        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::finished, [=]
        {
            mBrushConfiguratorWidget->hideUI();
            mBrushConfiguratorWidget = nullptr;
        });

        connect(this, &MPBrushSelector::brushSelected, mBrushConfiguratorWidget, &MPBrushConfigurator::updateConfig);
        connect(this, &MPBrushSelector::notifyBrushSettingChanged, mBrushConfiguratorWidget, &MPBrushConfigurator::setBrushSettingValue);
        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::brushRemoved, this, &MPBrushSelector::reloadBrushes);
        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::notifyBrushInfoUpdated, this, &MPBrushSelector::updateBrushList);
        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::notifyBrushSettingToggled, this, &MPBrushSelector::notifyBrushSettingToggled);
        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::brushSettingChanged, this, &MPBrushSelector::notifyBrushSettingChanged);
        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::reloadBrushSettings, this, &MPBrushSelector::onDidReloadBrushSettings);
    }

    if (show) {
        if (!mBrushConfiguratorWidget->isVisible()) {
            mBrushConfiguratorWidget->show();
        }
        mBrushConfiguratorWidget->updateUI();
    } else {
        mBrushConfiguratorWidget->hideUI();
    }
}

void MPBrushSelector::changeBrushPreset(int index, QString name, int data)
{
    Q_UNUSED(index)
    Q_UNUSED(data)

    mEditor->brushes()->brushPreferences( [=] (QSettings& settings) {
        settings.setValue(SETTING_MPBRUSHPRESET, name);

        mEditor->brushes()->setCurrentPresetName(name);
        loadBrushesIntoList();
    });
}
