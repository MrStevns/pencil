#include "defaultbrushsettingwidget.h"

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

DefaultBrushSettingWidget::DefaultBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent)
    : BrushSettingWidget(parent),
    mSettingName(name), mSettingType(settingType), mParent(parent)
{
    mHBoxLayout = new QHBoxLayout(this);
    setLayout(mHBoxLayout);

    mValueSlider = new InlineSlider(this, min, max, name);

    mInputMinValue = min;
    mInputMaxValue = max;

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mHBoxLayout->setContentsMargins(0,0,0,0);
    mHBoxLayout->addWidget(mValueSlider);

    mValueSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(mValueSlider, &InlineSlider::valueChanged, this, &DefaultBrushSettingWidget::updateSetting);
}

void DefaultBrushSettingWidget::initUI()
{
    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

    mValueSlider->setRange(mInputMinValue, mInputMaxValue);

    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    setRange(static_cast<qreal>(info.min), static_cast<qreal>(info.max));
    setValue(baseValue);
    setToolTip(info.tooltip);
}

void DefaultBrushSettingWidget::updateUI()
{
    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    setValue(baseValue);
}

void DefaultBrushSettingWidget::setValue(qreal value)
{
    qreal mappedValue = qBound(mInputMinValue, MathUtils::linearMap(value, mOutputMinValue, mOutputMaxValue, mInputMinValue, mInputMaxValue), mInputMaxValue);

    QSignalBlocker b(mValueSlider);

    mValueSlider->setValue(mappedValue);
}

void DefaultBrushSettingWidget::setPixelValue(qreal pixelValue)
{
    QSignalBlocker b(mValueSlider);

    mValueSlider->setValue(pixelValue);
}

void DefaultBrushSettingWidget::setRange(qreal min, qreal max)
{
    mOutputMinValue = min;
    mOutputMaxValue = max;
}

void DefaultBrushSettingWidget::setToolTip(const QString& toolTip)
{
    mValueSlider->setToolTip(toolTip);
}

void DefaultBrushSettingWidget::updateSetting(qreal value)
{
    qreal mappedToOrig = MathUtils::linearMap(value, mInputMinValue, mInputMaxValue, mOutputMinValue, mOutputMaxValue);

    if (qFuzzyIsNull(mappedToOrig)) {
        mappedToOrig = 0.0;
    }

    emit brushSettingChanged(mappedToOrig, this->mSettingType);
}
