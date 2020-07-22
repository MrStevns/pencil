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

#include <QJsonObject>
#include <QJsonValueRef>
#include <QJsonDocument>
#include <QJsonArray>

#include "toolmanager.h"

#include "spinslider.h"
#include "brushsettingitem.h"
#include "brushsettingeditwidget.h"
#include "editor.h"
#include "mpbrushutils.h"
#include "mpbrushinfodialog.h"

MPBrushConfigurator::MPBrushConfigurator(QWidget *parent)
  : QDialog(parent, Qt::Tool)
{
    setBaseSize(QSize(450,400));
    setWindowTitle(tr("Brush Configurator", "Window title of mypaint brush configurator"));

    mNavigatorWidget = new QTreeWidget(parent);
    mNavigatorWidget->setRootIsDecorated(false);
    mNavigatorWidget->setHeaderHidden(true);

    mBrushImageWidget = new QLabel();
    mBrushImageWidget->setFixedSize(mImageSize);

    mBrushNameWidget = new QLabel();

    QSplitter* viewSplitter = new QSplitter;
    QHBoxLayout* hLayout = new QHBoxLayout(parent);
    QVBoxLayout* vLayout = new QVBoxLayout(parent);

    vLayout->setMargin(0);
    QScrollArea* scrollArea = new QScrollArea(nullptr);

    QToolBar* toolbar = new QToolBar(this);

    QPushButton* cloneBrushButton = new QPushButton(parent);
    cloneBrushButton->setDefault(false);
    cloneBrushButton->setText("Clone");

    QPushButton* editBrushButton = new QPushButton(parent);
    editBrushButton->setText(tr("Edit Brush"));
    editBrushButton->setToolTip(tr("Here you can rename, change icon, change description and notes of the current brush"));

    mDiscardChangesButton = new QPushButton(parent);
    mDiscardChangesButton->setText(tr("Discard changes"));
    mDiscardChangesButton->setToolTip(tr("Discard current changes"));
    mDiscardChangesButton->setEnabled(false);

    QPushButton* deleteBrushButton = new QPushButton(parent);
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

    QWidget* resetSpacer = new QWidget(parent);
    resetSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    toolbar->addWidget(resetSpacer);
    toolbar->addWidget(mDiscardChangesButton);
    toolbar->addWidget(deleteBrushButton);

    QHBoxLayout* topLayout = new QHBoxLayout(this);

    topLayout->setContentsMargins(5,5,0,0);

    vLayout->addLayout(topLayout);
    vLayout->setContentsMargins(5,0,0,0);

    topLayout->addWidget(mBrushImageWidget);
    topLayout->addWidget(mBrushNameWidget);

    vLayout->addWidget(settingsContainer);

    vLayout->addWidget(toolbar);

    viewSplitter->addWidget(mNavigatorWidget);
    viewSplitter->addWidget(scrollArea);
    hLayout->addWidget(viewSplitter);
    hLayout->setMargin(0);

    viewSplitter->setSizes({150, 600});
    viewSplitter->setStretchFactor(1,4);
    viewSplitter->setStretchFactor(0,0);

    mBrushSettingsWidget = new QWidget(parent);

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
}

void MPBrushConfigurator::initUI()
{
    mBrushName = mEditor->brushes()->currentBrushName();
    mPreset = mEditor->brushes()->currentPresetName();

    auto basicRoot = addTreeRoot(BrushSettingItem::Basic, mNavigatorWidget, tr("Basic settings"));

    auto advanceRoot = addTreeRoot(BrushSettingItem::Advanced, mNavigatorWidget, tr("Advanced settings"));
    addTreeChild(BrushSettingItem::Opacity, advanceRoot, tr("Opacity settings"));
    addTreeChild(BrushSettingItem::Dab, advanceRoot, tr("Dab settings"));
    addTreeChild(BrushSettingItem::Random, advanceRoot, tr("Random settings"));
    addTreeChild(BrushSettingItem::Speed, advanceRoot, tr("Speed settings"));
    addTreeChild(BrushSettingItem::Offset, advanceRoot, tr("Offset settings"));
    addTreeChild(BrushSettingItem::Tracking, advanceRoot, tr("Tracking settings"));
    addTreeChild(BrushSettingItem::Color, advanceRoot, tr("Color settings"));
    addTreeChild(BrushSettingItem::Smudge, advanceRoot, tr("Smudge settings"));
    addTreeChild(BrushSettingItem::Eraser, advanceRoot, tr("Eraser setting"));
    addTreeChild(BrushSettingItem::Stroke, advanceRoot, tr("Stroke settings"));
    addTreeChild(BrushSettingItem::Custom_Input, advanceRoot, tr("Custom Input settings"));
    addTreeChild(BrushSettingItem::Elliptical_Dab, advanceRoot, tr("Elliptical settings"));
    addTreeChild(BrushSettingItem::Other, advanceRoot, tr("Other settings"));

    updateSettingsView(basicRoot);
}

void MPBrushConfigurator::updateUI()
{
    for (BrushSettingEditWidget* item : mBrushWidgets) {
        item->updateUI();
    }
    mDiscardChangesButton->setEnabled(!mOldModifications.isEmpty());

    QPixmap pix(QPixmap(mEditor->brushes()->getBrushImagePath(mPreset, mBrushName)));
    mBrushImageWidget->setPixmap(pix.scaled(mImageSize, Qt::KeepAspectRatio));
    mBrushNameWidget->setText(mBrushName);
}

void MPBrushConfigurator::updateConfig()
{
    auto currentBrushName = mEditor->brushes()->currentBrushName();
    auto currentPresetName = mEditor->brushes()->currentPresetName();

    if (mBrushName != currentBrushName && mPreset != currentPresetName) {
        mOldModifications.clear();
    }

    mToolType = mEditor->tools()->currentTool()->type();
    mBrushName = currentBrushName;
    mPreset = currentPresetName;
    updateUI();
}

void MPBrushConfigurator::prepareBasicBrushSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(Opacity));
    mBrushWidgets.append(new BrushSettingEditWidget(RadiusLog));
    mBrushWidgets.append(new BrushSettingEditWidget(Hardness));
    mBrushWidgets.append(new BrushSettingEditWidget(AntiAliasing));
    mBrushWidgets.append(new BrushSettingEditWidget(PressureGain));
}

void MPBrushConfigurator::prepareAdvancedBrushSettings()
{
    prepareOpacitySettings();
    prepareDabSettings();
    prepareRandomSettings();
    prepareSpeedSettings();
    prepareOffsetSettings();
    prepareTrackingSettings();
    prepareColorSettings();
    prepareSmudgeSettings();
    prepareEraserSetting();
    prepareStrokeSettings();
    prepareCustomInputSettings();
    prepareEllipticalDabSettings();
    prepareOtherSettings();
}

void MPBrushConfigurator::prepareOpacitySettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(Opacity));
    mBrushWidgets.append(new BrushSettingEditWidget(OpacityMultiply));
    mBrushWidgets.append(new BrushSettingEditWidget(OpacityLinearize));
}

void MPBrushConfigurator::prepareDabSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(RadiusLog));
    mBrushWidgets.append(new BrushSettingEditWidget(Hardness));
    mBrushWidgets.append(new BrushSettingEditWidget(DabsPerBasicRadius));
    mBrushWidgets.append(new BrushSettingEditWidget(DabsPerActualRadius));
    mBrushWidgets.append(new BrushSettingEditWidget(DabsPerSecond));
    mBrushWidgets.append(new BrushSettingEditWidget(DabScale));
    mBrushWidgets.append(new BrushSettingEditWidget(DabScaleX));
    mBrushWidgets.append(new BrushSettingEditWidget(DabScaleY));
}

void MPBrushConfigurator::prepareRandomSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(RadiusRandom));
}

void MPBrushConfigurator::prepareSpeedSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(SpeedStart));
    mBrushWidgets.append(new BrushSettingEditWidget(SpeedEnd));
    mBrushWidgets.append(new BrushSettingEditWidget(SpeedGammaStart));
    mBrushWidgets.append(new BrushSettingEditWidget(SpeedGammaEnd));
}

void MPBrushConfigurator::prepareOffsetSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetRandom));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetX));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetY));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetAngleLeft));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetAngleLeftAscend));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetAngleRight));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetAngleRightAscend));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetAngleAdjecent));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetMultiplier));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetBySpeed));
    mBrushWidgets.append(new BrushSettingEditWidget(OffsetSpeedSlowness));
}

void MPBrushConfigurator::prepareTrackingSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(SlowTracking));
    mBrushWidgets.append(new BrushSettingEditWidget(SlowTrackingPerDab));
    mBrushWidgets.append(new BrushSettingEditWidget(TrackingNoise));
}

void MPBrushConfigurator::prepareColorSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(ChangeColorHue));
    mBrushWidgets.append(new BrushSettingEditWidget(ChangeColorLightness));
    mBrushWidgets.append(new BrushSettingEditWidget(ChangeColorHLSSaturation));
    mBrushWidgets.append(new BrushSettingEditWidget(ChangeColorValue));
    mBrushWidgets.append(new BrushSettingEditWidget(ChangeColorHSVSaturation));
    mBrushWidgets.append(new BrushSettingEditWidget(RestoreColor));
}

void MPBrushConfigurator::prepareSmudgeSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(Smudge));
    mBrushWidgets.append(new BrushSettingEditWidget(SmudgeLength));
    mBrushWidgets.append(new BrushSettingEditWidget(SmudgeRadius));
}

void MPBrushConfigurator::prepareEraserSetting()
{
    mBrushWidgets.append(new BrushSettingEditWidget(Eraser));
}

void MPBrushConfigurator::prepareStrokeSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(StrokeThreshold));
    mBrushWidgets.append(new BrushSettingEditWidget(StrokeDuration));
    mBrushWidgets.append(new BrushSettingEditWidget(StrokeHoldTime));
}

void MPBrushConfigurator::prepareCustomInputSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(CustomInput));
    mBrushWidgets.append(new BrushSettingEditWidget(CustomInputSlowness));
}

void MPBrushConfigurator::prepareEllipticalDabSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(EllepticalDabRatio));
    mBrushWidgets.append(new BrushSettingEditWidget(EllepticalDabAngle));
}

void MPBrushConfigurator::prepareOtherSettings()
{
    mBrushWidgets.append(new BrushSettingEditWidget(AntiAliasing));
    mBrushWidgets.append(new BrushSettingEditWidget(LockAlpha));
    mBrushWidgets.append(new BrushSettingEditWidget(Colorize));
    mBrushWidgets.append(new BrushSettingEditWidget(SnapToPixel));
    mBrushWidgets.append(new BrushSettingEditWidget(PressureGain));
}

BrushSettingItem* MPBrushConfigurator::addTreeRoot(BrushSettingItem::Category category, QTreeWidget* treeWidget, const QString name)
{
    BrushSettingItem* treeItem = new BrushSettingItem(category, treeWidget);
    treeItem->setText(0, name);
    return treeItem;
}

BrushSettingItem* MPBrushConfigurator::addTreeChild(BrushSettingItem::Category category, QTreeWidgetItem* parent, const QString name)
{
    BrushSettingItem *treeItem = new BrushSettingItem(category, parent);
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

    removeBrushSettingSpacers();

    switch(static_cast<BrushSettingItem*>(item)->ItemCategory()) {
        case BrushSettingItem::Basic: prepareBasicBrushSettings(); break;
        case BrushSettingItem::Advanced: prepareAdvancedBrushSettings(); break;
        case BrushSettingItem::Opacity: prepareOpacitySettings(); break;
        case BrushSettingItem::Speed: prepareSpeedSettings(); break;
        case BrushSettingItem::Dab: prepareDabSettings(); break;
        case BrushSettingItem::Random: prepareRandomSettings(); break;
        case BrushSettingItem::Offset: prepareOffsetSettings(); break;
        case BrushSettingItem::Tracking: prepareTrackingSettings(); break;
        case BrushSettingItem::Color: prepareColorSettings(); break;
        case BrushSettingItem::Smudge: prepareSmudgeSettings(); break;
        case BrushSettingItem::Eraser: prepareEraserSetting(); break;
        case BrushSettingItem::Stroke: prepareStrokeSettings(); break;
        case BrushSettingItem::Custom_Input: prepareCustomInputSettings(); break;
        case BrushSettingItem::Elliptical_Dab: prepareEllipticalDabSettings(); break;
        case BrushSettingItem::Other: prepareOtherSettings(); break;
        case BrushSettingItem::Unknown: return;
    }

    for (BrushSettingEditWidget* item : mBrushWidgets) {
        vBoxLayout->addWidget(item);

        mListOfConnections << connect(item, &BrushSettingEditWidget::brushSettingChanged, this, &MPBrushConfigurator::saveChanges);
        mListOfConnections << connect(item, &BrushSettingEditWidget::brushMappingForInputChanged, this, &MPBrushConfigurator::updateBrushMapping);
        mListOfConnections << connect(item, &BrushSettingEditWidget::brushMappingRemoved, this, &MPBrushConfigurator::removeBrushMappingForInput);
        mListOfConnections << connect(item, &BrushSettingEditWidget::toggleSettingFor, this, &MPBrushConfigurator::toggleSettingForBrushSetting);

        item->setCore(mEditor);
        item->initUI();
    }

    addBrushSettingsSpacer();
    updateUI();
}

void MPBrushConfigurator::brushCategorySelectionChanged(const QItemSelection &, const QItemSelection &)
{
    updateSettingsView(mNavigatorWidget->currentItem());
}

void MPBrushConfigurator::brushCategorySelected(QTreeWidgetItem* item, int )
{
    updateSettingsView(item);
}

void MPBrushConfigurator::removeBrushSettingSpacers()
{
    for (int i = 0; i < vBoxLayout->count(); i++) {
        if (static_cast<QSpacerItem*>(vBoxLayout->itemAt(i)) != nullptr) {
            vBoxLayout->removeItem(vBoxLayout->itemAt(i));
        }
    }
}

void MPBrushConfigurator::addBrushSettingsSpacer()
{
    QSpacerItem *spacer = new QSpacerItem(0, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);
    vBoxLayout->addItem(spacer);
}

void MPBrushConfigurator::updateBrushSetting(qreal value, BrushSettingType setting)
{
    for (BrushSettingEditWidget* widget : mBrushWidgets)
    {
        if (widget->settingType() == setting) {
            widget->setValue(value);
            return;
        }
    }
}

void MPBrushConfigurator::saveChanges(qreal startValue, qreal value, BrushSettingType settingType)
{
    int settingTypeInt = static_cast<int>(settingType);

    // Adds old brush value and only save once, so we can discard it later if needed
    if (!mOldModifications.contains(settingTypeInt)) {
        BrushChanges changes;
        changes.baseValue = startValue;
        changes.settingsType = settingType;
        mOldModifications.insert(settingTypeInt, changes);
    }

    if (!mCurrentModifications.contains(settingTypeInt)) {
        BrushChanges changes;
        changes.baseValue = value;
        changes.settingsType = settingType;

        mCurrentModifications.insert(settingTypeInt, changes);
    } else {
        BrushChanges outerChanges;
        QHashIterator<int, BrushChanges> changesHash(mCurrentModifications);
        while (changesHash.hasNext()) {
            changesHash.next();

            BrushChanges innerChanges = changesHash.value();
            if (settingType == innerChanges.settingsType) {
                innerChanges.baseValue = value;
                innerChanges.settingsType = settingType;
                outerChanges = innerChanges;
                mCurrentModifications.insert(settingTypeInt, outerChanges);
                break;
            }
        }
    }

    if (!mCurrentModifications.isEmpty()) {
        mDiscardChangesButton->setEnabled(true);
    }

    mEditor->setMPBrushSetting(settingType, static_cast<float>(value));

    auto st = mEditor->brushes()->applyChangesToBrushFile(mCurrentModifications);

    if (st.fail()) {
        QMessageBox::warning(this, st.title(), st.description());
        return;
    }

    emit brushSettingChanged(value, settingType);
}

void MPBrushConfigurator::updateBrushMapping(QVector<QPointF> points, BrushSettingType setting, BrushInputType input)
{

    // if no base value has been provided, use the default value from brush settings
    qreal baseValue = static_cast<qreal>(mEditor->getBrushSettingInfo(setting).defaultValue);
    if (mCurrentModifications.isEmpty()) {
        BrushChanges changes;
        changes.baseValue = baseValue;
        changes.settingsType = setting;
        auto mappedInputs = points;

        changes.listOfinputChanges.insert(static_cast<int>(input), InputChanges { mappedInputs, input });

        mCurrentModifications.insert(static_cast<int>(setting),changes);
    } else {
        QHashIterator<int, BrushChanges> it(mCurrentModifications);
        while (it.hasNext()) {
            it.next();
            BrushChanges changes = it.value();
            changes.settingsType = setting;
            if (setting == changes.settingsType) {
                auto mappedInputs = points;
                changes.listOfinputChanges.insert(static_cast<int>(input), InputChanges { mappedInputs, input });

                mCurrentModifications.insert(static_cast<int>(setting), changes);
            }
        }

        QHashIterator<int, BrushChanges> itBrush(mCurrentModifications);
        while (itBrush.hasNext()) {
            itBrush.next();

            QHashIterator<int, InputChanges> i(itBrush.value().listOfinputChanges);
            while (i.hasNext()) {
                  i.next();
            }
        }
    }

    if (!mCurrentModifications.isEmpty()) {
        mDiscardChangesButton->setEnabled(true);
    }

    mEditor->setBrushInputMapping(points, setting, input);
    mEditor->brushes()->applyChangesToBrushFile(mCurrentModifications);

    mCurrentModifications.clear();
}

void MPBrushConfigurator::removeBrushMappingForInput(BrushSettingType setting, BrushInputType input)
{
    if (mCurrentModifications.isEmpty()) {
        BrushChanges changes;
        changes.baseValue = static_cast<qreal>(mEditor->getBrushSettingInfo(setting).defaultValue);
        changes.settingsType = setting;

        changes.listOfinputChanges.insert(static_cast<int>(input), InputChanges { {}, input, false });

        mCurrentModifications.insert(static_cast<int>(setting),changes);
    } else {
        QHashIterator<int, BrushChanges> brushIt(mCurrentModifications);
        while (brushIt.hasNext()) {
            brushIt.next();

            BrushChanges changes = brushIt.value();
            if (changes.settingsType == setting) {

                QHashIterator<int, InputChanges> inputIt(brushIt.value().listOfinputChanges);
                while (inputIt.hasNext()) {
                      inputIt.next();

                      InputChanges inputChanges = inputIt.value();
                      if (inputChanges.inputType == input) {
                          inputChanges.enabled = false;
                          inputChanges.inputType = input;
                          changes.listOfinputChanges.find(inputIt.key()).value() = inputChanges;
                          mCurrentModifications.insert(static_cast<int>(setting), changes);
                      }
                }
            }
        }
    }

    mEditor->setBrushInputMapping({}, setting, input);
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
            emit refreshBrushList();
            close();
        } else {
            QMessageBox::warning(this, st.title(),
                                       st.description());
        }
    }
}

void MPBrushConfigurator::openBrushInfoWidget(DialogContext dialogContext)
{
    if (mBrushInfoWidget == nullptr) {
        mBrushInfoWidget = new MPBrushInfoDialog(dialogContext, this);
        mBrushInfoWidget->setAttribute(Qt::WA_DeleteOnClose);

        connect(mBrushInfoWidget, SIGNAL(updatedBrushInfo()), this, SLOT(updateBrushList()));
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
    QHashIterator<int, BrushChanges> changesIt(mOldModifications);

    while (changesIt.hasNext()) {
        changesIt.next();

        mEditor->setMPBrushSetting(static_cast<BrushSettingType>(changesIt.key()),
                                   static_cast<float>(changesIt.value().baseValue));
    }

    mEditor->brushes()->applyChangesToBrushFile(mOldModifications);
    updateUI();

    mCurrentModifications.clear();
    mOldModifications.clear();
    mDiscardChangesButton->setEnabled(false);
}

void MPBrushConfigurator::showNotImplementedPopup()
{
    QMessageBox::information(this, tr("Not implemented"),
                                  tr("This feature is coming soon"));
}

