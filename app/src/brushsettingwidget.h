#ifndef BRUSHSETTINGWIDGET_H
#define BRUSHSETTINGWIDGET_H

#include "basewidget.h"

#include "brushsetting.h"

class QToolButton;
class InlineSlider;
class QDoubleSpinBox;
class Editor;
class MPMappingOptionsWidget;
class QHBoxLayout;

class BrushSettingWidget : public BaseWidget
{
    Q_OBJECT
public:
    BrushSettingWidget(QWidget* parent = nullptr) : BaseWidget(parent) {}
    virtual ~BrushSettingWidget() = default;

    virtual void initUI() = 0;
    virtual void updateUI() = 0;
    virtual void setCore(Editor* editor) = 0;

    virtual void setPixelValue(qreal pixelValue) = 0;
    virtual void setValue(qreal value) = 0;
    virtual void setRange(qreal min, qreal max) = 0;
    virtual void setToolTip(const QString& toolTip) = 0;

    virtual BrushSettingType setting() const = 0;

    virtual QString name() const = 0;
};

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

#endif // BRUSHSETTINGWIDGET_H
