#include "widthbrushsettingwidget.h"

#include "inlineslider.h"

WidthBrushSettingWidget::WidthBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent)
    : DefaultBrushSettingWidget(name, settingType, min, max, parent)
{
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

    // qreal expValue = value;
    mValueSlider->setValue(value);
    mValueSlider->setCosmeticValue(exp(value));

    mCurrentValue = value;
}

void WidthBrushSettingWidget::setRange(qreal, qreal)
{
    DefaultBrushSettingWidget::setRange(qLn(0.1), qLn(1000.0) + log(2.0));
}

