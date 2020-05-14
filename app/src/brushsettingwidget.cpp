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

    mVisualBox = new QDoubleSpinBox(this);

    mMappedMin = min;
    mMappedMax = max;

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mHBoxLayout->setMargin(0);
    mHBoxLayout->addWidget(mValueSlider);
    mHBoxLayout->addWidget(mValueBox);
    mHBoxLayout->addWidget(mVisualBox);

    mVisualBox->setGeometry(mValueBox->geometry());
    mVisualBox->setHidden(true);

    mValueSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    connect(mValueSlider, &SpinSlider::valueChanged, this, &BrushSettingWidget::updateSetting);
    connect(mValueSlider, &SpinSlider::valueOnRelease, this, &BrushSettingWidget::updateSetting);
    connect(mValueBox, static_cast<void(QDoubleSpinBox::*)(qreal)>(&QDoubleSpinBox::valueChanged), this, &BrushSettingWidget::updateSetting);
}

void BrushSettingWidget::initUI()
{
    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSetting(mSettingType));
    setValue(baseValue);
    setRange(static_cast<qreal>(info.min), static_cast<qreal>(info.max));
    setToolTip(info.tooltip);
}

void BrushSettingWidget::updateUI()
{
    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSetting(mSettingType));
    setValue(baseValue);
}

void BrushSettingWidget::onSliderChanged(qreal value)
{
    setValueInternal(value);
}

void BrushSettingWidget::setValue(qreal value)
{
    qDebug() << "value changing";
    qreal normalize = MathUtils::normalize(value, mMin, mMax);
    qreal mappedValue = MathUtils::mapFromNormalized(normalize, mMappedMin, mMappedMax);

    QSignalBlocker b(mValueSlider);
    mValueSlider->setValue(mappedValue);

    QSignalBlocker b2(mValueBox);
    mValueBox->setValue(mappedValue);

    QSignalBlocker b3(mVisualBox);
    mVisualBox->setValue(value);

    mCurrentValue = value;
}

void BrushSettingWidget::changeText()
{
    bool shouldHide = !mVisualBox->isHidden();
    mVisualBox->setHidden(shouldHide);
    mValueBox->setHidden(!shouldHide);
}

void BrushSettingWidget::setValueInternal(qreal value)
{
    QSignalBlocker b(mValueSlider);
    mValueSlider->setValue(value);
    QSignalBlocker b2(mValueBox);
    mValueBox->setValue(value);

    qreal normalize = MathUtils::normalize(value, mMappedMin, mMappedMax);
    qreal mappedToOrig = MathUtils::mapFromNormalized(normalize, mMin, mMax);

    QSignalBlocker b3(mVisualBox);
    mVisualBox->setValue(mappedToOrig);

    mCurrentValue = value;
}

void BrushSettingWidget::setRange(qreal min, qreal max)
{

    mMin = min;
    mMax = max;

    mValueSlider->setRange(mMin, mMax);
    setValue(mCurrentValue);
}

void BrushSettingWidget::setToolTip(QString toolTip)
{
    mValueBox->setToolTip(toolTip);
    mValueSlider->setToolTip(toolTip);
}

void BrushSettingWidget::updateSetting(qreal value)
{
    qreal normalize = MathUtils::normalize(value, mMappedMin, mMappedMax);
    qreal mappedToOrig = MathUtils::mapFromNormalized(normalize, mMin, mMax);

    setValueInternal(value);

    emit brushSettingChanged(mappedToOrig, this->mSettingType);
}
