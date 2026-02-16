#ifndef WIDTHBRUSHSETTINGWIDGET_H
#define WIDTHBRUSHSETTINGWIDGET_H

#include "brushsettingwidget.h"

class WidthBrushSettingWidget : public DefaultBrushSettingWidget
{
public:
    WidthBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent);

    void setPixelValue(qreal pixelValue) override;
    void setValue(qreal value) override;
    void setRange(qreal min, qreal max) override;
};

#endif // WIDTHBRUSHSETTINGWIDGET_H
