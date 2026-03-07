#ifndef DEFAULTBRUSHSETTINGWIDGET_H
#define DEFAULTBRUSHSETTINGWIDGET_H

#include "brushsettingwidget.h"

class DefaultBrushSettingWidget : public BrushSettingWidget
{
    Q_OBJECT
public:
    DefaultBrushSettingWidget(const QString& name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent = nullptr);
    ~DefaultBrushSettingWidget() override { }

    virtual void initUI() override;
    void updateUI() override;
    void setCore(Editor* editor) override { mEditor = editor; }

    virtual void setPixelValue(qreal pixelValue) override;
    virtual void setValue(qreal value) override;
    virtual void setRange(qreal min, qreal max) override;
    virtual QString name() const override { return mSettingName; }

    void setToolTip(const QString& toolTip) override;

    BrushSettingType setting() const override { return mSettingType; }

    InlineSlider* inlineSlider() { return mValueSlider; }

Q_SIGNALS:
    void brushSettingChanged(qreal value, BrushSettingType setting);

protected:
    virtual void updateSetting(qreal value);

    InlineSlider* mValueSlider = nullptr;

    qreal mInputMinValue = 0.0;
    qreal mInputMaxValue = 0.0;

    const QString mSettingName;
    qreal mOutputMinValue = 0.0;
    qreal mOutputMaxValue = 0.0;

    Editor* mEditor = nullptr;

    BrushSettingType mSettingType;

private:
    QWidget* mParent = nullptr;

    QHBoxLayout* mHBoxLayout;
};


#endif // DEFAULTBRUSHSETTINGWIDGET_H
