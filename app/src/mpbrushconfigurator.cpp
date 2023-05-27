#include "mpbrushconfigurator.h"

#include <QTreeWidget>
#include <QLayout>
#include <QSpinBox>
#include <QScrollArea>
#include <QDebug>
#include <QtMath>
#include <QSplitter>
#include <QPushButton>
#include <QToolBar>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QJsonDocument>
#include <QSettings>

#include "errordialog.h"
#include "toolmanager.h"
#include "colormanager.h"

#include "spinslider.h"
#include "brushsettingitem.h"
#include "brushsettingeditwidget.h"
#include "editor.h"
#include "mpbrushutils.h"
#include "mpbrushinfodialog.h"
#include "mpbrushpreview.h"

MPBrushConfigurator::MPBrushConfigurator(QWidget *parent)
  : QDialog(parent, Qt::Tool)
{
    setBaseSize(QSize(450,400));
    setWindowTitle(tr("Brush Configurator", "Window title of mypaint brush configurator"));
    setObjectName("MPBrushConfigurator");

    mNavigatorWidget = new QTreeWidget(this);
    mNavigatorWidget->setRootIsDecorated(false);
    mNavigatorWidget->setHeaderHidden(true);

    mBrushImageWidget = new QLabel(this);
    mBrushImageWidget->setMaximumSize(mImageSize);

    mBrushPreviewWidget = new MPBrushPreview();
    mBrushPreviewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mBrushPreviewWidget->setFixedHeight(mImageSize.height());

    QSplitter* viewSplitter = new QSplitter;
    QHBoxLayout* hLayout = new QHBoxLayout();
    QVBoxLayout* vLayout = new QVBoxLayout();

    vLayout->setMargin(0);
    QScrollArea* scrollArea = new QScrollArea(nullptr);

    QToolBar* toolbar = new QToolBar(this);

    QPushButton* cloneBrushButton = new QPushButton(this);
    cloneBrushButton->setDefault(false);
    cloneBrushButton->setText("Clone");

    QPushButton* editBrushButton = new QPushButton(this);
    editBrushButton->setText(tr("Edit Brush"));
    editBrushButton->setToolTip(tr("Here you can rename, change icon, change description and notes of the current brush"));

    mDiscardChangesButton = new QPushButton(this);
    mDiscardChangesButton->setText(tr("Discard changes"));
    mDiscardChangesButton->setToolTip(tr("Discard current changes"));
    mDiscardChangesButton->setEnabled(false);

    QPushButton* resetButton = new QPushButton(this);
    resetButton->setText(tr("Reset"));
    mDiscardChangesButton->setToolTip(tr("Factory reset settings"));
    resetButton->setEnabled(true);

    QPushButton* deleteBrushButton = new QPushButton(this);
    deleteBrushButton->setText(tr("Delete"));
    deleteBrushButton->setToolTip(tr("Delete current brush and close window"));

    setLayout(vLayout);

    QWidget* settingsContainer = new QWidget(this);
    settingsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    settingsContainer->setLayout(hLayout);

    toolbar->addWidget(cloneBrushButton);
    toolbar->addSeparator();
    toolbar->addWidget(editBrushButton);
    toolbar->addSeparator();

    QWidget* resetSpacer = new QWidget(this);
    resetSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(resetSpacer);
    toolbar->addWidget(mDiscardChangesButton);
    toolbar->addWidget(deleteBrushButton);
    toolbar->addWidget(resetButton);

    QHBoxLayout* topLayout = new QHBoxLayout();

    topLayout->setContentsMargins(5,5,5,0);

    vLayout->addLayout(topLayout);
    vLayout->setContentsMargins(5,0,0,0);

    topLayout->addWidget(mBrushImageWidget);
    topLayout->addWidget(mBrushPreviewWidget);

    vLayout->addWidget(settingsContainer);

    vLayout->addWidget(toolbar);

    viewSplitter->addWidget(mNavigatorWidget);
    viewSplitter->addWidget(scrollArea);
    hLayout->addWidget(viewSplitter);
    hLayout->setMargin(0);

    viewSplitter->setSizes({150, 600});
    viewSplitter->setStretchFactor(1,4);
    viewSplitter->setStretchFactor(0,0);

    mBrushSettingsWidget = new QWidget(this);

    scrollArea->setWidget(mBrushSettingsWidget);
    scrollArea->setWidgetResizable(true);

    vBoxLayout = new QVBoxLayout();
    mBrushSettingsWidget->setLayout(vBoxLayout);

    connect(mNavigatorWidget, &QTreeWidget::itemPressed, this, &MPBrushConfigurator::brushCategorySelected);
    connect(mNavigatorWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MPBrushConfigurator::brushCategorySelectionChanged);

    connect(cloneBrushButton, &QPushButton::pressed, this, &MPBrushConfigurator::pressedCloneBrush);
    connect(deleteBrushButton, &QPushButton::pressed, this, &MPBrushConfigurator::pressedRemoveBrush);

    connect(mDiscardChangesButton, &QPushButton::pressed, this, &MPBrushConfigurator::pressedDiscardBrush);
    connect(editBrushButton, &QPushButton::pressed, this, &MPBrushConfigurator::pressedEditBrush);
    connect(cloneBrushButton, &QPushButton::pressed, this, &MPBrushConfigurator::pressedEditBrush);
    connect(resetButton, &QPushButton::pressed, this, &MPBrushConfigurator::onResetButtonPressed);
}

void MPBrushConfigurator::initUI()
{
    mBrushName = mEditor->brushes()->currentBrushName();
    mPreset = mEditor->brushes()->currentPresetName();
    mToolType = mEditor->tools()->currentTool()->type();

    mActiveTreeRoot = addTreeRoot(BrushSettingCategoryType::Active, mNavigatorWidget, tr("Active settings"));
    auto basicRoot = addTreeRoot(BrushSettingCategoryType::Basic, mNavigatorWidget, tr("Basic settings"));

    auto advanceRoot = addTreeRoot(BrushSettingCategoryType::Advanced, mNavigatorWidget, tr("Advanced settings"));
    addTreeChild(BrushSettingCategoryType::Opacity, advanceRoot, tr("Opacity settings"));
    addTreeChild(BrushSettingCategoryType::Dab, advanceRoot, tr("Dab settings"));
    addTreeChild(BrushSettingCategoryType::Random, advanceRoot, tr("Random settings"));
    addTreeChild(BrushSettingCategoryType::Speed, advanceRoot, tr("Speed settings"));
    addTreeChild(BrushSettingCategoryType::Offset, advanceRoot, tr("Offset settings"));
    addTreeChild(BrushSettingCategoryType::Tracking, advanceRoot, tr("Tracking settings"));
    addTreeChild(BrushSettingCategoryType::Color, advanceRoot, tr("Color settings"));
    addTreeChild(BrushSettingCategoryType::Smudge, advanceRoot, tr("Smudge settings"));
    addTreeChild(BrushSettingCategoryType::Eraser, advanceRoot, tr("Eraser setting"));
    addTreeChild(BrushSettingCategoryType::Stroke, advanceRoot, tr("Stroke settings"));
    addTreeChild(BrushSettingCategoryType::Custom, advanceRoot, tr("Custom Input settings"));
    addTreeChild(BrushSettingCategoryType::Elliptical, advanceRoot, tr("Elliptical settings"));
    addTreeChild(BrushSettingCategoryType::Other, advanceRoot, tr("Other settings"));

    connect(mEditor->color(), &ColorManager::colorChanged, this, &MPBrushConfigurator::prepareUpdateBrushPreview);

    updateSettingsView(basicRoot);

    prepareUpdateBrushPreview();
}

void MPBrushConfigurator::updateUI()
{
    for (BrushSettingEditWidget* item : mBrushWidgets) {
        item->updateUI();
    }

    mDiscardChangesButton->setEnabled(mEditor->brushes()->brushModificationsForTool());

    QPixmap pix(QPixmap(mEditor->brushes()->getBrushImagePath(mPreset, mBrushName)));
    mBrushImageWidget->setPixmap(pix.scaled(mImageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    mBrushImageWidget->setToolTip(mBrushName);
    prepareUpdateBrushPreview();
}

void MPBrushConfigurator::hideUI()
{
    hide();
    for (BrushSettingEditWidget* widget : mBrushWidgets) {
        if (widget) {
            widget->hideMappingUI();
        }
    }
    if (mBrushInfoWidget && mBrushInfoWidget->isVisible()) {
        mBrushInfoWidget->hide();
    }
}

void MPBrushConfigurator::updateConfig()
{
    auto currentBrushName = mEditor->brushes()->currentBrushName();
    auto currentPresetName = mEditor->brushes()->currentPresetName();

    mToolType = mEditor->tools()->currentTool()->type();
    mBrushName = currentBrushName;
    mPreset = currentPresetName;
    updateUI();
}

void MPBrushConfigurator::setupActiveSettings()
{
    QSettings settings(PENCIL2D, PENCIL2D);
    auto groups = settings.childGroups();

    QString toolGroup = mEditor->brushes()->currentToolBrushIdentifier();

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
            BrushSettingEditWidget* settingWidget = new BrushSettingEditWidget(BrushSettingCategoryType::Other, name, getBrushSetting(key), min, max, this);
            mBrushWidgets.append(settingWidget);
        }
    }
}

void MPBrushConfigurator::setupBasicBrushSettings()
{
    for (auto category : settingCategories.basicBrushSettings()) {

        for (auto setting : category.settings) {
            mBrushWidgets << new BrushSettingEditWidget(category.categoryType, setting, this);
        }
    }
}


void MPBrushConfigurator::setupAdvancedBrushSettings()
{
    setupSettingsFor(BrushSettingCategoryType::Opacity);
    setupSettingsFor(BrushSettingCategoryType::Dab);
    setupSettingsFor(BrushSettingCategoryType::Random);
    setupSettingsFor(BrushSettingCategoryType::Speed);
    setupSettingsFor(BrushSettingCategoryType::Offset);
    setupSettingsFor(BrushSettingCategoryType::Tracking);
    setupSettingsFor(BrushSettingCategoryType::Color);
    setupSettingsFor(BrushSettingCategoryType::Smudge);
    setupSettingsFor(BrushSettingCategoryType::Eraser);
    setupSettingsFor(BrushSettingCategoryType::Stroke);
    setupSettingsFor(BrushSettingCategoryType::Custom);
    setupSettingsFor(BrushSettingCategoryType::Elliptical);
    setupSettingsFor(BrushSettingCategoryType::Other);
}

void MPBrushConfigurator::setupSettingsFor(BrushSettingCategoryType type)
{
    auto category = settingCategories.categoryForType(type);
    for (auto setting : category.settings) {
        mBrushWidgets << new BrushSettingEditWidget(category.categoryType, setting, this);
    }
}

BrushSettingTreeItem* MPBrushConfigurator::addTreeRoot(BrushSettingCategoryType category, QTreeWidget* treeWidget, const QString name)
{
    BrushSettingTreeItem* treeItem = new BrushSettingTreeItem(category, treeWidget);
    treeItem->setText(0, name);
    return treeItem;
}

BrushSettingTreeItem* MPBrushConfigurator::addTreeChild(BrushSettingCategoryType category, QTreeWidgetItem* parent, const QString name)
{
    BrushSettingTreeItem *treeItem = new BrushSettingTreeItem(category, parent);
    treeItem->setText(0, name);
    parent->addChild(treeItem);
    return treeItem;
}

void MPBrushConfigurator::updateSettingsView(QTreeWidgetItem* item)
{
    if (!mBrushWidgets.isEmpty()) {
        qDeleteAll(mBrushSettingsWidget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly));
        for (int i = 0; i < mListOfConnections.count(); i++) {
            disconnect(mListOfConnections.takeAt(i));
        }
        mBrushWidgets.clear();
    }

    removeBrushSettingSpacer();

    auto treeItem = static_cast<BrushSettingTreeItem*>(item);

    switch (treeItem->categoryType()) {
    case BrushSettingCategoryType::Active:
        setupActiveSettings(); break;
    case BrushSettingCategoryType::Basic:
        setupBasicBrushSettings(); break;
    case BrushSettingCategoryType::Advanced:
        setupAdvancedBrushSettings(); break;
    default:
        setupSettingsFor(static_cast<BrushSettingTreeItem*>(item)->categoryType());
    }

    for (BrushSettingEditWidget* item : mBrushWidgets) {
        vBoxLayout->addWidget(item);

        mListOfConnections << connect(item, &BrushSettingEditWidget::brushSettingChanged, this, &MPBrushConfigurator::prepareBrushChanges);

        mListOfConnections << connect(item, &BrushSettingEditWidget::brushMappingForInputChanged, this, &MPBrushConfigurator::prepareBrushInputChanges);
        mListOfConnections << connect(item, &BrushSettingEditWidget::brushMappingRemoved, this, &MPBrushConfigurator::removeBrushMappingForInput);
        mListOfConnections << connect(item, &BrushSettingEditWidget::brushSettingToggled, this, &MPBrushConfigurator::notifyBrushSettingToggled);

        item->setCore(mEditor);
        item->initUI();
    }

    addBrushSettingsSpacer();
    updateUI();
}

void MPBrushConfigurator::prepareUpdateBrushPreview()
{
    mBrushPreviewWidget->updatePreview(mEditor->brushes()->currentBrushData(), mEditor->color()->frontColor());
}

void MPBrushConfigurator::brushCategorySelectionChanged(const QItemSelection &, const QItemSelection &)
{
    updateSettingsView(mNavigatorWidget->currentItem());
}

void MPBrushConfigurator::brushCategorySelected(QTreeWidgetItem* item, int )
{
    updateSettingsView(item);
}

void MPBrushConfigurator::removeBrushSettingSpacer()
{
    vBoxLayout->removeItem(mLayoutSpacer);
    delete mLayoutSpacer;
}

void MPBrushConfigurator::addBrushSettingsSpacer()
{
    mLayoutSpacer = new QSpacerItem(0, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vBoxLayout->addSpacerItem(mLayoutSpacer);
}

void MPBrushConfigurator::setBrushSettingValue(qreal value, BrushSettingType setting)
{
    this->blockSignals(true);
    prepareBrushChanges(value, setting);
    prepareUpdateBrushPreview();
    this->blockSignals(false);
    for (BrushSettingEditWidget* widget : mBrushWidgets)
    {
        if (widget->settingType() == setting) {
            widget->setValue(value);
            return;
        }
    }
}

void MPBrushConfigurator::prepareBrushChanges(qreal value, BrushSettingType setting)
{
    mEditor->brushes()->backupBrushSettingChanges(setting, value);

    if (mEditor->brushes()->brushModificationsForTool()) {
        mDiscardChangesButton->setEnabled(true);
    }

    mEditor->setMPBrushSettingBaseValue(setting, static_cast<float>(value));

    auto st = mEditor->brushes()->applyChangesToBrushFile(false);

    if (st.fail()) {
        ErrorDialog dialog(st, this);
        dialog.exec();
        return;
    }

    emit brushSettingChanged(value, setting);

    prepareUpdateBrushPreview();
}

void MPBrushConfigurator::prepareBrushInputChanges(QVector<QPointF> points, BrushSettingType setting, BrushInputType input)
{
    mEditor->brushes()->backupBrushInputChanges(setting, input, points);

    if (mEditor->brushes()->brushModificationsForTool()) {
        mDiscardChangesButton->setEnabled(true);
    }

    mEditor->setBrushInputMapping(points, setting, input);
    mEditor->brushes()->applyChangesToBrushFile(false);

    prepareUpdateBrushPreview();
}

void MPBrushConfigurator::removeBrushMappingForInput(BrushSettingType setting, BrushInputType input)
{
    auto currentMapping = mEditor->getBrushInputMapping(setting, input).controlPoints.points;
    mEditor->brushes()->backupBrushInputChanges(setting, input, currentMapping);

    if (mEditor->brushes()->brushModificationsForTool()) {
        mDiscardChangesButton->setEnabled(true);
    }

    mEditor->brushes()->removeBrushInputMapping(setting, input);

    mEditor->setBrushInputMapping({}, setting, input);
    mEditor->brushes()->applyChangesToBrushFile(false);
    prepareUpdateBrushPreview();
}

void MPBrushConfigurator::updateMapValuesButton()
{
    mMapValuesButtonPressed = !mMapValuesButtonPressed;
    if (mMapValuesButtonPressed) {
        mMapValuesButton->setText(tr("Pencil2D values"));
    } else {
        mMapValuesButton->setText(tr("MyPaint values"));
    }
}

void MPBrushConfigurator::pressedRemoveBrush()
{   
    QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("Delete brush"),
                                   tr("Are you sure you want to delete this brush?"),
                                   QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (ret == QMessageBox::Yes) {
        auto st = MPCONF::blackListBrushFile(mPreset, mBrushName);

        if (st.ok()) {
            emit brushRemoved();
            close();
        } else {
            QMessageBox::warning(this, st.title(),
                                       st.description());
        }
    }
}

void MPBrushConfigurator::onResetButtonPressed()
{
    QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("Factory reset brush"),
                                   tr("This will reset brush settings for the chosen brush, are you sure you want to factory reset?"),
                                   QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);

    if (ret == QMessageBox::Yes) {
        Status st = mEditor->brushes()->resetCurrentBrush();

        if (st.fail()) {
            ErrorDialog dialog(st, this);
            dialog.exec();

            return;
        }

        reloadBrushSettings();
        updateUI();

        mEditor->brushes()->clearCurrentBrushModifications();
    }
}

void MPBrushConfigurator::openBrushInfoWidget(DialogContext dialogContext)
{
    if (mBrushInfoWidget == nullptr) {
        mBrushInfoWidget = new MPBrushInfoDialog(dialogContext, this);
        mBrushInfoWidget->setAttribute(Qt::WA_DeleteOnClose);

        connect(mBrushInfoWidget.data(), &MPBrushInfoDialog::updatedBrushInfo, this, &MPBrushConfigurator::notifyBrushInfoUpdated);
    }
    mBrushInfoWidget->setCore(mEditor);
    mBrushInfoWidget->initUI();

    auto brushMan = mEditor->brushes();

    auto status = brushMan->readBrushFromFile(mPreset, mBrushName);
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(brushMan->currentBrushData(), &error);

    mBrushInfoWidget->setBrushInfo(mBrushName, mPreset, mToolType, doc);
    mBrushInfoWidget->show();
}

void MPBrushConfigurator::pressedEditBrush()
{
    openBrushInfoWidget(DialogContext::Edit);
}

void MPBrushConfigurator::pressedCloneBrush()
{
    openBrushInfoWidget(DialogContext::Clone);
}

void MPBrushConfigurator::pressedDiscardBrush()
{
    mEditor->brushes()->discardBrushChanges();
    mEditor->brushes()->applyChangesToBrushFile(true);
    updateUI();

    reloadBrushSettings();
    prepareUpdateBrushPreview();

    mEditor->brushes()->clearCurrentBrushModifications();
    mDiscardChangesButton->setEnabled(false);
}

void MPBrushConfigurator::showNotImplementedPopup()
{
    QMessageBox::information(this, tr("Not implemented"),
                                  tr("This feature is coming soon"));
}

