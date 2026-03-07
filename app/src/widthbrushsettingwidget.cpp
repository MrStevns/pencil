#include "widthbrushsettingwidget.h"

#include "inlineslider.h"
#include "editor.h"

#include "QtMath"

WidthBrushSettingWidget::WidthBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent)
    : DefaultBrushSettingWidget(name, settingType, min, max, parent)
{
}

void WidthBrushSettingWidget::initUI()
{
    BrushSettingInfo info = mEditor->getBrushSettingInfo(mSettingType);

    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    mValueSlider->setRange(mInputMinValue, mInputMaxValue);

    mValueSlider->setValuePostFix("px");
    mValueSlider->setScaleType(InlineSlider::ScaleType::LOG);
    setRange(info.min, info.max);
    setValue(baseValue);
    setToolTip(info.tooltip);
}

void WidthBrushSettingWidget::setPixelValue(qreal pixelValue)
{
    QSignalBlocker b(mValueSlider);

    qreal boundValue = qBound(mInputMinValue, pixelValue, mInputMaxValue);
    mValueSlider->setValue(boundValue);

    updateSetting(boundValue);
}

void WidthBrushSettingWidget::setValue(qreal value)
{
    QSignalBlocker b(mValueSlider);

    qreal boundValue = qBound(mInputMinValue, qExp(value) * 2.0, mInputMaxValue);

    mValueSlider->setValue(boundValue);
}

void WidthBrushSettingWidget::setRange(qreal min, qreal max)
{
    mOutputMinValue = min;
    mOutputMaxValue = max;
}

void WidthBrushSettingWidget::updateSetting(qreal value)
{
    emit brushSettingChanged(qLn(value * 0.5), this->mSettingType);
}
