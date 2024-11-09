#include "brushsettingwidget.h"

#include <QLayout>
#include <QSignalBlocker>
#include <QDebug>
#include <QtMath>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QVector>

#include "mpbrushmanager.h"

#include "editor.h"

#include "mathutils.h"

BrushSettingWidget::BrushSettingWidget(const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent) : QWidget(parent),
    mSettingType(settingType), mParent(parent), mSettingName(name)
{
    mHBoxLayout = new QHBoxLayout(this);
    setLayout(mHBoxLayout);

    mValueSlider = new SpinSlider(this);
    mValueSlider->init(name, SpinSlider::GROWTH_TYPE::LINEAR, SpinSlider::VALUE_TYPE::FLOAT, min, max);
    mValueBox = new QDoubleSpinBox(this);

    mValueBox->setRange(min, max);

    #if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
        mValueBox->setStepType(QDoubleSpinBox::StepType::AdaptiveDecimalStepType);
    #endif

    mValueBox->setDecimals(2);

    mMappedMin = min;
    mMappedMax = max;

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mHBoxLayout->setContentsMargins(0,0,0,0);
    mHBoxLayout->addWidget(mValueSlider);
    mHBoxLayout->addWidget(mValueBox);

    mValueSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    connect(mValueSlider, &SpinSlider::valueChanged, this, &BrushSettingWidget::updateSetting);
    connect(mValueSlider, &SpinSlider::valueOnRelease, this, &BrushSettingWidget::updateSetting);
    connect(mValueBox, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged), this, &BrushSettingWidget::updateSetting);
}

void BrushSettingWidget::initUI()
{
    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    setRange(static_cast<qreal>(info.min), static_cast<qreal>(info.max));
    setValue(baseValue);
    setToolTip(info.tooltip);
}

void BrushSettingWidget::updateUI()
{
    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    setValue(baseValue);
}

void BrushSettingWidget::setValue(qreal value)
{
    qreal mappedValue = qBound(mMappedMin, MathUtils::linearMap(value, mMin, mMax, mMappedMin, mMappedMax), mMappedMax);

    QSignalBlocker b(mValueSlider);
    mValueSlider->setValue(mappedValue);

    QSignalBlocker b2(mValueBox);
    mValueBox->setValue(mappedValue);

    mCurrentValue = value;

#ifdef Q_OS_MAC
    // HACK: Qt bug prevents the view from being updated properly, so sliders that needs to be
    // updated at the same time, intertwine and causes offsetting...
    // there's still some of it left even after this but it does make it somewhat useable now...
    repaint();
#endif
}

void BrushSettingWidget::setValueFromUnmapped(qreal value)
{
    updateSetting(value);
}

void BrushSettingWidget::setRange(qreal min, qreal max)
{
    mMin = min;
    mMax = max;
    mValueSlider->setRange(mMappedMin, mMappedMax);
}

void BrushSettingWidget::setToolTip(QString toolTip)
{
    mValueBox->setToolTip(toolTip);
    mValueSlider->setToolTip(toolTip);
}

void BrushSettingWidget::updateSetting(qreal value)
{
    qreal mappedToOrig = MathUtils::linearMap(value, mMappedMin, mMappedMax, mMin, mMax);

    setValue(value);

    emit brushSettingChanged(mappedToOrig, this->mSettingType);
}
