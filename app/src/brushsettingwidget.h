#ifndef BRUSHSETTINGWIDGET_H
#define BRUSHSETTINGWIDGET_H

#include <QWidget>

#include "brushsetting.h"

class QToolButton;
class InlineSlider;
class QDoubleSpinBox;
class Editor;
class MPMappingOptionsWidget;
class QHBoxLayout;

class BrushSettingWidget : public QWidget
{
    Q_OBJECT
public:
    BrushSettingWidget(const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent = nullptr);

    void setValue(qreal value);
    void setRange(qreal min, qreal max);
    void setToolTip(QString toolTip);
    void setCore(Editor* editor) { mEditor = editor; }
    void updateUI();
    void initUI();

    void changeText();

    BrushSettingType setting() const { return mSettingType; }
    QString name() const { return mSettingName; }
    qreal currentValue() const { return mLogValue; }

    void setValueFromUnmapped(qreal value);

Q_SIGNALS:
    void brushSettingChanged(qreal unmappedValue, qreal mappedValue, BrushSettingType setting);

private:
    void updateSetting(qreal value);
    float logToLinear(float logValue) const;

    InlineSlider* mValueSlider = nullptr;
    BrushSettingType mSettingType;

    Editor* mEditor = nullptr;

    qreal mMinLog = 0.0;
    qreal mMaxLog = 0.0;
    qreal mLogValue = 0.0;

    QWidget* mParent = nullptr;

    const QString mSettingName;

    QHBoxLayout* mHBoxLayout;
};

#endif // BRUSHSETTINGWIDGET_H
