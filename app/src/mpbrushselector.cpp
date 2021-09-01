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
    QToolButton* presetsManagerButton = new QToolButton(this);

    configuratorButton->setText(tr("config"));
    configuratorButton->setToolTip(tr("Open brush configurator window"));

    mTopAreaWidget = new QWidget();
    mVLayout->addWidget(mTopAreaWidget);
    mTopAreaWidget->setLayout(hLayout);

    hLayout->addWidget(configuratorButton);
    hLayout->addWidget(mPresetComboBox);
    hLayout->addWidget(presetsManagerButton);

    mVLayout->setContentsMargins(8,2,8,8);

    containerWidget->setLayout(mVLayout);
    setWidget(containerWidget);

    connect(configuratorButton, &QToolButton::pressed, [=](void) { showBrushConfigurator(true); } );
    connect(mPresetComboBox, &ComboBox::activated, this, &MPBrushSelector::changeBrushPreset);
    connect(presetsManagerButton, &QToolButton::pressed, [=](void) { showPresetManager(true); });
}

void MPBrushSelector::initUI()
{
    // First, we parse the "brushes.conf" file to fill m_brushLib
    loadBrushes();
}

void MPBrushSelector::updateUI()
{

}

void MPBrushSelector::loadBrushes()
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

    for (MPBrushPreset preset : presets) {
        mPresetComboBox->addItem(preset.name);
    }

    QSettings settings(PENCIL2D,PENCIL2D);
    QString lastPreset = settings.value(SETTING_MPBRUSHPRESET).toString();

    if (!lastPreset.isEmpty()) {
        mPresetComboBox->setCurrentItemFrom(lastPreset);
    }

    if (!mTabsLoaded) {
        addToolTabs();
        mTabsLoaded = true;
    }
    populateList();
}

void MPBrushSelector::addToolTabs()
{

    for (int i = 0; i < TOOL_TYPE_COUNT; i++) {
        QListWidget* listWidget = new QListWidget(nullptr);
        listWidget->setUniformItemSizes(true);
        listWidget->setViewMode        (QListView::IconMode);
        listWidget->setResizeMode      (QListView::Adjust);
        listWidget->setMovement        (QListView::Static);
        listWidget->setFlow            (QListView::LeftToRight);
        listWidget->setSelectionMode   (QAbstractItemView::SingleSelection);
        listWidget->setIconSize        (QSize(ICON_SZ,ICON_SZ));
        connect(listWidget, SIGNAL(itemClicked(QListWidgetItem*)), SLOT(itemClicked(QListWidgetItem*)));

        ToolType tool = static_cast<ToolType>(i);
        QString toolName = BaseTool::TypeName(tool);

        listWidget->setFocusPolicy(Qt::NoFocus);
        mToolListWidgets.insert(toolName, listWidget);
    }
}

void MPBrushSelector::updateBrushList(QString brushName, QString brushPreset)
{
    mEditor->brushes()->setCurrentPresetName(brushPreset);

    loadBrushes();
    selectBrush(brushName);
}

void MPBrushSelector::reloadBrushList()
{
    loadBrushes();
}

void MPBrushSelector::populateList()
{
    // Make sure currentBrushPreset is loaded here
    auto presets = mEditor->brushes()->presets();
    auto currentPresetName = mEditor->brushes()->currentPresetName();
    for (const MPBrushPreset& pre : presets) {
        if (pre.name == currentPresetName) {
            currentBrushPreset = pre;
            break;
        }
    }
    MPBrushPreset preset = currentBrushPreset;

    QMapIterator<QString, QListWidget*> toolListIt(mToolListWidgets);
    while(toolListIt.hasNext()) {
        toolListIt.next();
        QListWidget* widget = toolListIt.value();

        // clear existing widgets before re-adding
        widget->clear();

        const MPBrushPreset subList = preset;
        if (subList.isEmpty()) continue; // this should not happen...

        int brushIndex = 0;
        for (const QString& brush : subList.brushesForTool(toolListIt.key().toLower())) {

            QIcon preview(mEditor->brushes()->brushesPath() + "/" + currentPresetName + "/" + brush + BRUSH_PREVIEW_EXT);
            QListWidgetItem* p_item = new QListWidgetItem(preview, nullptr, widget, brushIndex);
            p_item->setToolTip(QString("%1").arg(brush));
            p_item->setData(Qt::UserRole, QVariant::fromValue(brush));
            brushIndex++;
        }
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
        int brushIndex = 0;
        for (const QString& brush : preset.brushesForTool(currentToolName)) {
            if (itemWidget->data(Qt::UserRole).toString().compare(brush, Qt::CaseInsensitive) == 0) {
                brushName = brush;
                break;
            }
            brushIndex++;
        }

        loadBrushFromFile(brushName);

        mEditor->preference()->set(QString(SETTING_MPBRUSHFORTOOL+currentToolName+preset.name).toLower(), brushName.toLower());
    }
}

void MPBrushSelector::loadBrushFromFile(const QString& brushName)
{
    auto brushMan = mEditor->brushes();
    Status status = brushMan->readBrushFromCurrentPreset(brushName);
    if (status == Status::OK)
    {
        emit brushSelected(); // Read the whole file and broadcast is as a char* buffer
    } else {
        ErrorDialog errorDialog(status.title(), status.description(), status.details().str());
        errorDialog.exec();
    }
}

void MPBrushSelector::onDidReloadBrushSettings()
{
    mEditor->loadBrush();
    emit didReloadBrush();
}

void MPBrushSelector::loadToolBrushes(QString toolName)
{
    QMap<QString, QListWidget*>::iterator widgetIt;

    for (widgetIt = mToolListWidgets.begin(); widgetIt != mToolListWidgets.end(); ++widgetIt)
    {
        const QString& toolNameKey = widgetIt.key();
        QListWidget* newWidget = widgetIt.value();

        newWidget->setHidden(true);

        if (toolName == "empty") {
            mTopAreaWidget->setVisible(false);
        } else {
            mTopAreaWidget->setVisible(true);
        }
        if (toolNameKey.compare(toolName,Qt::CaseInsensitive) == 0) {

            // make sure the widget exists already and has been added to the layout
            if (newWidget != currentListWidget) {
                if (currentListWidget != nullptr) {
                    mVLayout->replaceWidget(currentListWidget, newWidget);
                } else {
                    mVLayout->addWidget(newWidget);
                }
            }
            currentListWidget = newWidget;

            // We found current tool, so show the related listwidget
            if (currentListWidget->isHidden()) {
                currentListWidget->setHidden(false);
            }
        }
    }

    const MPBrushPreset subList = currentBrushPreset;

    if (!subList.isEmpty()) {

        // Make sure we don't get stuck with same brush on different tools
        if (toolName != oldToolname) {
            QString lastBrushForTool = mEditor->preference()->get(QString(SETTING_MPBRUSHFORTOOL+toolName+subList.name).toLower());

            if (toolName == "empty") {
                // Do nothing
            } else if (lastBrushForTool.isEmpty() || !subList.brushesForTool(toolName).contains(lastBrushForTool, Qt::CaseInsensitive)) {
                // No brush has been selected before, select the first

                if (!subList.brushesForTool(toolName).isEmpty()) {
                    QString brushName (subList.brushesForTool(toolName).first());
                    selectBrush(brushName);
                } else {
                    qDebug() << "NO BRUSH EXISTS FOR TOOL";
                }
            } else {
                selectBrush(lastBrushForTool);
            }
        }
    }
}

void MPBrushSelector::typeChanged(ToolType eToolMode)
{
    QString toolName = QString(mEditor->getTool(eToolMode)->typeName()).toLower();
    switch ( eToolMode )
    {
    case ToolType::PENCIL:
    case ToolType::ERASER:
    case ToolType::PEN:
    case ToolType::BRUSH:
    case ToolType::POLYLINE:
    case ToolType::SMUDGE:
        break;
    default:
        toolName = "empty";
        break;
    }

    currentToolType = eToolMode;
    currentToolName = toolName;

    loadToolBrushes(toolName);

    oldToolname = currentToolName;
}

void MPBrushSelector::selectBrush(QString brushName)
{
    Q_UNUSED(brushName)
    if (mEditor->brushes()->presets().isEmpty()) return;
    QListWidget* listWidget = currentListWidget;
    QListWidgetItem* itemWidget = nullptr;

    const MPBrushPreset preset = currentBrushPreset;

    Q_ASSERT(listWidget != nullptr);

    int listWidgetIdx = 0;
    for (int i = 0; i < listWidget->count(); i++)
    {
        QListWidgetItem* item = listWidget->item(i);
        if (item->data(Qt::UserRole).toString().compare(brushName, Qt::CaseInsensitive) == 0) {
            itemWidget = listWidget->item(listWidgetIdx);
            break;
        }
        listWidgetIdx++;
    }

    // default one : we use the first tab page & the first item available:
    if (!itemWidget && listWidget && listWidget->count() > 0)
    {
        itemWidget = listWidget->item(0);
    }

    // Update GUI + load the brush (if any)
    if (itemWidget)
    {
        listWidget->setCurrentItem(itemWidget);
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
        connect(mBrushConfiguratorWidget, &MPBrushConfigurator::brushRemoved, this, &MPBrushSelector::reloadBrushList);
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

void MPBrushSelector::showPresetManager(bool show)
{
    if (mPresetsWidget == nullptr) {
        mPresetsWidget = new MPBrushPresetsWidget(mEditor->brushes()->presets(), this);
        connect(mPresetsWidget.data(), &MPBrushPresetsWidget::presetsChanged, this, &MPBrushSelector::reloadBrushList);
    }

    if (show) {
        if (!mPresetsWidget->isVisible()) {
            mPresetsWidget->show();
        }
    } else {
        mPresetsWidget->hide();
    }
}

void MPBrushSelector::changeBrushPreset(int index, QString name, int data)
{
    Q_UNUSED(index)
    Q_UNUSED(data)

    QSettings settings(PENCIL2D,PENCIL2D);
    settings.setValue(SETTING_MPBRUSHPRESET, name);

    mEditor->brushes()->setCurrentPresetName(name);
    populateList();
}

void MPBrushSelector::showNotImplementedPopup()
{
    QMessageBox::information(this, tr("Not implemented"),
                                  tr("This feature is coming soon"));
}
