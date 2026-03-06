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

    qreal boundValue = qBound(mMinValue, pixelValue, mMaxValue);
    mValueSlider->setValue(boundValue);

    updateSetting(boundValue);
}

void WidthBrushSettingWidget::setValue(qreal value)
{
    QSignalBlocker b(mValueSlider);

    qreal boundValue = qBound(mMinValue, qExp(value) * 2.0, mMaxValue);

    mValueSlider->setValue(boundValue);
}

void WidthBrushSettingWidget::setRange(qreal, qreal)
{
    mMinValue = 1.0;
    mMaxValue = 2000.0;
    mValueSlider->setRange(mMinValue, mMaxValue);
}

void WidthBrushSettingWidget::updateSetting(qreal value)
{
    emit brushSettingChanged(qLn(value * 0.5), this->mSettingType);
}
