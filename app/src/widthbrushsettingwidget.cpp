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

    // qDebug() << "WidthBrushSetting::initUI settingType: " << static_cast<int>(mSettingType);
    qreal baseValue = static_cast<qreal>(mEditor->getMPBrushSettingBaseValue(mSettingType));
    setRange(qLn(1), qLn(1000.0));
    // qDebug() << "WidthBrushSetting::initUI baseValue" << baseValue;
    // qDebug() << "WidthBrushSetting::initUI exp(baseValue)" << exp(baseValue);

    setValue(baseValue);
    setToolTip(info.tooltip);
}

void WidthBrushSettingWidget::setPixelValue(qreal pixelValue)
{
    // qDebug() << "WidthBrushSettingWidget::setPixelValue: " << pixelValue;

    QSignalBlocker b(mValueSlider);

    qreal radValue = qLn(pixelValue * 0.5);
    qreal boundValue = qBound(mMinValue, radValue, mMaxValue);

    mValueSlider->setValue(radValue);
    mValueSlider->setCosmeticValue(boundValue * 2.0);

    updateSetting(radValue);
}

void WidthBrushSettingWidget::setValue(qreal value)
{
    // qDebug() << "WidthBrushSettingWidget::setValue: " << value;

    QSignalBlocker b(mValueSlider);

    mValueSlider->setValue(value);

    qreal boundValue = qBound(mMinValue, value, mMaxValue);
    mValueSlider->setCosmeticValue(exp(boundValue) * 2.0);
}

void WidthBrushSettingWidget::setRange(qreal, qreal)
{
    mMinValue = qLn(1.0);
    mMaxValue = qLn(1000.0);
    mValueSlider->setRange(mMinValue, mMaxValue);
}

void WidthBrushSettingWidget::updateSetting(qreal value)
{
    emit brushSettingChanged(value, this->mSettingType);
}

