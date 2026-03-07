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
    setRange(1, 2000.0);

    mValueSlider->setValuePostFix("px");
    mValueSlider->setScaleType(InlineSlider::ScaleType::LOG);
    setValue(baseValue);
    setToolTip(info.tooltip);
}

void WidthBrushSettingWidget::setPixelValue(qreal pixelValue)
{
    QSignalBlocker b(mValueSlider);

    qreal boundValue = qBound(mOutputMinValue, pixelValue, mOutputMaxValue);
    mValueSlider->setValue(boundValue);

    updateSetting(boundValue);
}

void WidthBrushSettingWidget::setValue(qreal value)
{
    QSignalBlocker b(mValueSlider);

    qreal boundValue = qBound(mOutputMinValue, qExp(value) * 2.0, mOutputMaxValue);

    mValueSlider->setValue(boundValue);
}

void WidthBrushSettingWidget::setRange(qreal, qreal)
{
    mOutputMinValue = 1.0;
    mOutputMaxValue = 2000.0;
    mValueSlider->setRange(mOutputMinValue, mOutputMaxValue);
}

void WidthBrushSettingWidget::updateSetting(qreal value)
{
    emit brushSettingChanged(qLn(value * 0.5), this->mSettingType);
}
