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

    QString visualName = name;
    if (settingType == BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC) {
        visualName = tr("Width");
    }

    mValueSlider = new InlineSlider(this);
    mValueSlider->init(visualName, min, max, startPos);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    mHBoxLayout->setContentsMargins(0,0,0,0);
    mHBoxLayout->addWidget(mValueSlider);

    mValueSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(mValueSlider, &InlineSlider::valueChanged, this, &BrushSettingWidget::updateSetting);
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
    qDebug() << "BrushSettingWidget::setValue: " << value;

    QSignalBlocker b(mValueSlider);

    qreal expValue = exp(value);
    if (mSettingType == BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC) {
    //     visualValue *= 2.0;
        expValue *= 2.0;
    }
    mValueSlider->setValue(expValue);
    // mValueSlider->setCosmeticValue(visualValue);

    mLogValue = expValue;
}

void BrushSettingWidget::setValueFromUnmapped(qreal value)
{
    updateSetting(value);
}

void BrushSettingWidget::setRange(qreal min, qreal max)
{
    if (mSettingType == BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC) {
        mMinLog = exp(min)*2.0;
        mMaxLog = exp(max)*2.0;
        mValueSlider->setRange(mMinLog, mMaxLog);
    } else {
        mMinLog = exp(min);
        mMaxLog = exp(max);
        mValueSlider->setRange(mMinLog, mMaxLog);
    }
}

void BrushSettingWidget::setToolTip(QString toolTip)
{
    mValueSlider->setToolTip(toolTip);
}

void BrushSettingWidget::updateSetting(qreal value)
{
    setValue(value);

    qreal newValue = qLn(value);

    if (mSettingType == BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC) {
        newValue = qLn(value * 0.5);
    }
    emit brushSettingChanged(newValue, newValue, this->mSettingType);
}
