#ifndef BRUSHSETTINGEDITWIDGET_H
#define BRUSHSETTINGEDITWIDGET_H

#include "brushsetting.h"
#include "brushsettingwidget.h"

class QToolButton;
class SpinSlider;
class QDoubleSpinBox;
class Editor;
class MPMappingOptionsWidget;
class QCheckBox;


class BrushSettingEditWidget : public QWidget
{
    Q_OBJECT
public:
    BrushSettingEditWidget(const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent = nullptr);
    BrushSettingEditWidget(BrushSetting setting, QWidget* parent = nullptr);

    void setCore(Editor* editor);
    void updateUI();
    void initUI();

    void setValue(qreal value);

    void closeMappingWindow();

    const BrushSettingWidget* brushSettingWidget() { return mSettingWidget; }
    BrushSettingType settingType();
    QString settingName();

    qreal initialValue() { return mInitialValue; }

public slots:
    void notifyInputMappingRemoved(BrushInputType input);
    void visibilityChanged(bool state);

Q_SIGNALS:
    void brushMappingForInputChanged(QVector<QPointF> points, BrushSettingType setting, BrushInputType inputType);
    void brushMappingRemoved(BrushSettingType setting, BrushInputType);
    void toggleSettingFor(QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
    void brushSettingChanged(qreal initialValue, qreal value, BrushSettingType setting);

private:
    void openMappingWindow();
    void updateBrushMapping(QVector<QPointF> newPoints, BrushInputType inputType);
    void updateUIInternal();
    void updateSetting(qreal value, BrushSettingType setting);

    QToolButton* mMappingButton = nullptr;
    QCheckBox* mVisibleCheck = nullptr;

    Editor* mEditor = nullptr;

    const qreal mMin;
    const qreal mMax;

    qreal mInitialValue;

    BrushSettingWidget* mSettingWidget = nullptr;
    MPMappingOptionsWidget* mMappingWidget = nullptr;
};

#endif // BRUSHSETTINGEDITWIDGET_H
