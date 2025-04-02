#include "brushsettingwidget.h"

#include <QLayout>
#include <QSignalBlocker>
#include <QDebug>
#include <QtMath>
#include <QDoubleSpinBox>
#include <QToolButton>
#include <QVector>

#include "inlineslider.h"

#include "mpbrushmanager.h"

#include "editor.h"

#include "mathutils.h"

BrushSettingWidget::BrushSettingWidget(const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent) : QWidget(parent),
    mSettingType(settingType), mParent(parent), mSettingName(name)
{
    mHBoxLayout = new QHBoxLayout(this);
    setLayout(mHBoxLayout);

    SliderStartPosType startPos = SliderStartPosType::LEFT;
    if (min < 0) {
        startPos = SliderStartPosType::MIDDLE;
    }

    mValueSlider = new InlineSlider(this, name, min, max, startPos);

    mMappedMin = min;
    mMappedMax = max;

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mHBoxLayout->setContentsMargins(0,0,0,0);
    mHBoxLayout->addWidget(mValueSlider);

    mValueSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(mValueSlider, &InlineSlider::sliderReleased, this, &BrushSettingWidget::updateSetting);
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

    mCurrentValue = value;
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
    mValueSlider->setToolTip(toolTip);
}

void BrushSettingWidget::updateSetting(qreal value)
{
    qreal mappedToOrig = MathUtils::linearMap(value, mMappedMin, mMappedMax, mMin, mMax);

    setValue(value);

    emit brushSettingChanged(mappedToOrig, this->mSettingType);
}
