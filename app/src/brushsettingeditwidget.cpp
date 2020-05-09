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

#include "mathutils.h"

BrushSettingEditWidget::BrushSettingEditWidget(BrushSetting setting, QWidget* parent)
    : BrushSettingEditWidget(setting.name, setting.type, setting.min, setting.max, parent)
{
}

BrushSettingEditWidget::BrushSettingEditWidget(const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent) :
    QWidget(parent), mMin(min), mMax(max)
{
    QGridLayout* gridLayout = new QGridLayout(this);
    setLayout(gridLayout);

    mSettingWidget = new BrushSettingWidget(name, settingType, min, max, this);

    mVisibleCheck = new QCheckBox(this);
    mMappingButton = new QToolButton(this);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    gridLayout->setMargin(0);

    gridLayout->addWidget(mVisibleCheck, 0, 0);
    gridLayout->addWidget(mSettingWidget, 0,1);
    gridLayout->addWidget(mMappingButton, 0, 2);
    connect(mMappingButton, &QToolButton::pressed, this, &BrushSettingEditWidget::openMappingWindow);
    connect(mVisibleCheck, &QCheckBox::toggled, this, &BrushSettingEditWidget::visibilityChanged);
    connect(mSettingWidget, &BrushSettingWidget::brushSettingChanged, this, &BrushSettingEditWidget::updateSetting);
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
    emit brushSettingChanged(mInitialValue, value, setting);
}

void BrushSettingEditWidget::updateUIInternal()
{
    QString toolGroup = QString(SETTING_BRUSHSETTINGTOOL + mEditor->tools()->currentTool()->typeName()).toLower();

    qDebug() << "getting value for : " << toolGroup;
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.beginGroup(toolGroup);
    settings.beginGroup(getBrushSettingIdentifier(settingType()));

    bool isChecked = settings.value("visible").toBool();

    settings.endGroup();
    settings.endGroup();

    mVisibleCheck->setChecked(isChecked);
}

void BrushSettingEditWidget::initUI()
{
    mSettingWidget->setCore(mEditor);
    mSettingWidget->initUI();

    mInitialValue = mSettingWidget->currentValue();
    updateUIInternal();
    closeMappingWindow();
}

void BrushSettingEditWidget::updateUI()
{
    updateUIInternal();
    mSettingWidget->updateUI();
}

void BrushSettingEditWidget::setValue(qreal value)
{
    mSettingWidget->setValue(value);
}

void BrushSettingEditWidget::updateBrushMapping(QVector<QPointF> newPoints, BrushInputType inputType)
{
    qDebug() << "updating brush mapping";
    emit brushMappingForInputChanged(newPoints, settingType(), inputType);
}

void BrushSettingEditWidget::notifyInputMappingRemoved(BrushInputType input)
{
    emit brushMappingRemoved(settingType(), input);
}

void BrushSettingEditWidget::visibilityChanged(bool state)
{
    QString toolSetting = QString(SETTING_BRUSHSETTINGTOOL + mEditor->tools()->currentTool()->typeName()).toLower();
    QSettings settings(PENCIL2D, PENCIL2D);

    settings.beginGroup(toolSetting);
    settings.beginGroup(getBrushSettingIdentifier(settingType()));
    settings.setValue("visible", state);
    settings.setValue("name", settingName());
    settings.setValue("min", mMin);
    settings.setValue("max", mMax);
    settings.endGroup();
    settings.endGroup();

    emit toggleSettingFor(settingName(), settingType(), mMin, mMax, state);
}

void BrushSettingEditWidget::openMappingWindow()
{
    QVector<QPointF> tempPoints = { QPointF(0.0,0.0), QPointF(0.5,0.5), QPointF(1.0,1.0) };
    mMappingWidget = new MPMappingOptionsWidget(settingName(), settingType());
    mMappingWidget->setCore(mEditor);
    mMappingWidget->initUI();

    mMappingWidget->show();

    connect(mMappingWidget, &MPMappingOptionsWidget::mappingForInputUpdated, this, &BrushSettingEditWidget::updateBrushMapping);
    connect(mMappingWidget, &MPMappingOptionsWidget::removedInputOption, this, &BrushSettingEditWidget::notifyInputMappingRemoved);
}

void BrushSettingEditWidget::closeMappingWindow()
{
    if (mMappingWidget) {
        mMappingWidget->close();
    }
}


