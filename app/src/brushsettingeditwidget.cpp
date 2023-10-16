#include "brushsettingeditwidget.h"

#include <QLayout>
#include <QSignalBlocker>
#include <QDebug>
#include <QtMath>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QVector>
#include <QCheckBox>
#include <QSettings>

#include "spinslider.h"
#include "mpmappingwidget.h"
#include "mappingdistributionwidget.h"
#include "mpmappingoptionswidget.h"
#include "editor.h"
#include "preferencemanager.h"
#include "toolmanager.h"
#include "mpbrushmanager.h"

#include "mathutils.h"

BrushSettingEditWidget::BrushSettingEditWidget(BrushSettingCategoryType settingCategoryType, BrushSetting setting, QWidget* parent)
    : BrushSettingEditWidget(settingCategoryType, setting.name, setting.type, setting.min, setting.max, parent)
{
}

BrushSettingEditWidget::BrushSettingEditWidget(BrushSettingCategoryType settingCategoryType, const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent) :
    QWidget(parent), mMin(min), mMax(max)
{
    QGridLayout* gridLayout = new QGridLayout(this);
    setLayout(gridLayout);

    mSettingCategoryType = settingCategoryType;
    mSettingWidget = new BrushSettingWidget(name, settingType, min, max, this);
    mSettingType = settingType;
    mVisibleCheck = new QCheckBox(this);
    mMappingButton = new QToolButton(this);

    // TODO: platform specific styling should be put in one place...
#ifdef __APPLE__
    // Only Mac needs this. ToolButton is naturally borderless on Win/Linux.
    QString sStyle =
        "QToolButton { border: 0px; }"
        "QToolButton:pressed { border: 1px solid #ADADAD; border-radius: 2px; background-color: #D5D5D5; }"
        "QToolButton:checked { border: 1px solid #ADADAD; border-radius: 2px; background-color: #D5D5D5; }";
    mMappingButton->setStyleSheet(sStyle);
#endif
    mMappingButton->setIcon(QIcon("://icons/new/mapping-icon.png"));
    mMappingButton->setIconSize(QSize(20,20));

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    gridLayout->setContentsMargins(0,0,0,0);

    gridLayout->addWidget(mVisibleCheck, 0, 0);
    gridLayout->addWidget(mSettingWidget, 0, 1);
    gridLayout->addWidget(mMappingButton, 0, 2);
    connect(mMappingButton, &QToolButton::pressed, this, &BrushSettingEditWidget::openMappingWindow);
    connect(mVisibleCheck, &QCheckBox::toggled, this, &BrushSettingEditWidget::visibilityChanged);
    connect(mSettingWidget, &BrushSettingWidget::brushSettingChanged, this, &BrushSettingEditWidget::updateSetting);
}

void BrushSettingEditWidget::hideMappingUI()
{
    if (mMappingOptionsWidget && mMappingOptionsWidget->isVisible()) {
        mMappingOptionsWidget->hide();
    }
}

void BrushSettingEditWidget::setCore(Editor* editor)
{
    mEditor = editor;
}

BrushSettingType BrushSettingEditWidget::settingType()
{
    return mSettingWidget->setting();
}

QString BrushSettingEditWidget::settingName()
{
    return mSettingWidget->name();
}

void BrushSettingEditWidget::updateSetting(qreal value, BrushSettingType setting)
{
    emit brushSettingChanged(value, setting);
}

void BrushSettingEditWidget::updateUIInternal()
{
    QString toolGroup = mEditor->brushes()->currentToolBrushIdentifier();

//    qDebug() << "getting value for : " << toolGroup;
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.beginGroup(toolGroup);
    settings.beginGroup(getBrushSettingIdentifier(settingType()));

    bool isChecked = settings.value("visible").toBool();

    settings.endGroup();
    settings.endGroup();

    QSignalBlocker b(mVisibleCheck);
    mVisibleCheck->setChecked(isChecked);
}

void BrushSettingEditWidget::initUI()
{
    mSettingWidget->setCore(mEditor);
    mSettingWidget->initUI();

    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

    if (info.isConstant) {
        mMappingButton->setEnabled(false);
        mMappingButton->setToolTip(tr("This setting does not have any dynamics"));
    }

    updateUIInternal();
    closeMappingWindow();
}

void BrushSettingEditWidget::updateUI()
{
    updateUIInternal();
    mSettingWidget->updateUI();

    if (mMappingOptionsWidget) {
        mMappingOptionsWidget->notifyMappingWidgetNeedsUpdate(mCurrentInputType);
    }
}

void BrushSettingEditWidget::setValue(qreal value)
{
    mSettingWidget->setValue(value);
}

void BrushSettingEditWidget::updateBrushMapping(QVector<QPointF> newPoints, BrushInputType inputType)
{
    mCurrentInputType = inputType;
    qDebug() << "updating brush mapping";
    emit brushMappingForInputChanged(newPoints, settingType(), inputType);
}

void BrushSettingEditWidget::notifyInputMappingRemoved(BrushInputType input)
{
    emit brushMappingRemoved(settingType(), input);
}

void BrushSettingEditWidget::visibilityChanged(bool state)
{
    QString toolSetting = mEditor->brushes()->currentToolBrushIdentifier();

    QSettings settings(PENCIL2D, PENCIL2D);

    settings.beginGroup(toolSetting);
    settings.beginGroup(getBrushSettingIdentifier(settingType()));
    settings.setValue("visible", state);
    settings.setValue("name", settingName());
    settings.setValue("min", mMin);
    settings.setValue("max", mMax);
    settings.endGroup();
    settings.endGroup();

    emit brushSettingToggled(mSettingCategoryType, settingName(), settingType(), mMin, mMax, state);
}

void BrushSettingEditWidget::openMappingWindow()
{
    QVector<QPointF> tempPoints = { QPointF(0.0,0.0), QPointF(0.5,0.5), QPointF(1.0,1.0) };
    mMappingOptionsWidget = new MPMappingOptionsWidget(settingName(), settingType());
    mMappingOptionsWidget->setCore(mEditor);
    mMappingOptionsWidget->initUI();

    mMappingOptionsWidget->show();

    connect(mMappingOptionsWidget, &MPMappingOptionsWidget::mappingForInputUpdated, this, &BrushSettingEditWidget::updateBrushMapping);
    connect(mMappingOptionsWidget, &MPMappingOptionsWidget::removedInputOption, this, &BrushSettingEditWidget::notifyInputMappingRemoved);
    connect(this, &BrushSettingEditWidget::notifyMappingWidgetNeedsUpdate, mMappingOptionsWidget, &MPMappingOptionsWidget::notifyMappingWidgetNeedsUpdate);
}

void BrushSettingEditWidget::closeMappingWindow()
{
    if (mMappingOptionsWidget) {
        mMappingOptionsWidget->close();
    }
}

bool BrushSettingEditWidget::isChecked() const
{
    return mVisibleCheck->checkState();
}


