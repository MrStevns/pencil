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
    qreal currentValue() const { return mCurrentValue; }

    void setValueFromUnmapped(qreal value);

Q_SIGNALS:
    void brushSettingChanged(qreal value, BrushSettingType setting);

private:
    void updateSetting(qreal value);

    InlineSlider* mValueSlider = nullptr;
    BrushSettingType mSettingType;

    Editor* mEditor = nullptr;

    qreal mMin = 0.0;
    qreal mMax = 0.0;

    qreal mMappedMin = 0.0;
    qreal mMappedMax = 0.0;

    qreal mMappedValue = 0.0;

    qreal mCurrentValue;

    QWidget* mParent = nullptr;

    const QString mSettingName;

    bool first = false;

    QHBoxLayout* mHBoxLayout;
};

#endif // BRUSHSETTINGWIDGET_H
