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

DefaultBrushSettingWidget::DefaultBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent)
    : BrushSettingWidget(parent),
    mSettingType(settingType), mParent(parent), mSettingName(name)
{
    mHBoxLayout = new QHBoxLayout(this);
    setLayout(mHBoxLayout);

    SliderStartPosType startPos = SliderStartPosType::LEFT;
    // if (min < 0) {
        // startPos = SliderStartPosType::MIDDLE;
    // }

    mValueSlider = new InlineSlider(this);
    mValueSlider->init(name, min, max, startPos);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mHBoxLayout->setContentsMargins(0,0,0,0);
    mHBoxLayout->addWidget(mValueSlider);

    mValueSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(mValueSlider, &InlineSlider::valueChanged, this, &DefaultBrushSettingWidget::updateSetting);
}

void DefaultBrushSettingWidget::initUI()
{
    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

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

    qDebug() << "BrushSettingWidget::setValue: " << value;

    QSignalBlocker b(mValueSlider);

    // qreal expValue = value;
    mValueSlider->setValue(value);
    mValueSlider->setCosmeticValue(exp(value));

    mCurrentValue = value;
}

void DefaultBrushSettingWidget::setPixelValue(qreal pixelValue)
{
    qDebug() << "BrushSettingWidget::setPixelValue: " << pixelValue;

    QSignalBlocker b(mValueSlider);

    mValueSlider->setValue(qLn(pixelValue));
    mValueSlider->setCosmeticValue(pixelValue);

    mCurrentValue = pixelValue;
}

void DefaultBrushSettingWidget::setRange(qreal min, qreal max)
{
    mMinValue = min;
    mMaxValue = max;
    mValueSlider->setRange(mMinValue, mMaxValue);
}

void DefaultBrushSettingWidget::setToolTip(const QString& toolTip)
{
    mValueSlider->setToolTip(toolTip);
}

void DefaultBrushSettingWidget::updateSetting(qreal value)
{
    setValue(value);

    qDebug() << "updateSetting: " << value;

    qreal newValue = value;
    emit brushSettingChanged(newValue, newValue, this->mSettingType);
}
