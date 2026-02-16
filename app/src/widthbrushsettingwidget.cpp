#include "widthbrushsettingwidget.h"

#include "inlineslider.h"
#include "editor.h"

WidthBrushSettingWidget::WidthBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent)
    : DefaultBrushSettingWidget(name, settingType, min, max, parent)
{
}

void WidthBrushSettingWidget::initUI()
{
    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

    qDebug() << "WidthBrushSetting::initUI settingType: " << static_cast<int>(mSettingType);
    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    setRange(qLn(0.1), qLn(2000.0));
    qDebug() << "WidthBrushSetting::initUI baseValue" << baseValue;
    qDebug() << "WidthBrushSetting::initUI exp(baseValue)" << exp(baseValue);

    // qreal logValue = baseValue;
    // qreal maxLogRadius = logValue;

    // // Calculate the new base radius from all our inputs
    // qreal maxInputContribution = 0.0;
    // for (int input = 0; input < (int)BrushInputType::BRUSH_INPUTS_COUNT; input += 1) {
    //     auto inputMap = mEditor->getBrushInputMapping(BrushSettingType::BRUSH_SETTING_RADIUS_LOGARITHMIC, static_cast<BrushInputType>(input));

    //     for (QPointF point : inputMap.controlPoints.points) {
    //         maxInputContribution = qMax(maxInputContribution, maxInputContribution + point.y());
    //     }
    // }

    // baseValue = maxLogRadius + maxInputContribution;

    // qDebug() << "baseValue: " << baseValue;b
    setValue(baseValue);
    setToolTip(info.tooltip);
}

void WidthBrushSettingWidget::setPixelValue(qreal pixelValue)
{
    qDebug() << "WidthBrushSettingWidget::setPixelValue: " << pixelValue;

    QSignalBlocker b(mValueSlider);

    mValueSlider->setValue(qLn(pixelValue));
    mValueSlider->setCosmeticValue(pixelValue);

    mCurrentValue = pixelValue;
}

void WidthBrushSettingWidget::setValue(qreal value)
{
    qDebug() << "WidthBrushSettingWidget::setValue: " << value;

    QSignalBlocker b(mValueSlider);

    mValueSlider->setValue(value);
    mValueSlider->setCosmeticValue(exp(value));

    mCurrentValue = value;
}

void WidthBrushSettingWidget::setRange(qreal, qreal)
{
    // mMinValue = min;
    // mMaxValue = max;
    mValueSlider->setRange(qLn(0.1), qLn(2000.0));
}

void WidthBrushSettingWidget::updateSetting(qreal value)
{
    setValue(value);
    // qreal newValue = value;
    emit brushSettingChanged(value, this->mSettingType);
}

