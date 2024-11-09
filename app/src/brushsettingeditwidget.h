#ifndef BRUSHSETTINGEDITWIDGET_H
#define BRUSHSETTINGEDITWIDGET_H

#include "brushsetting.h"
#include "brushsettingwidget.h"
#include "mpbrushsettingcategories.h"

class QToolButton;
class QDoubleSpinBox;
class Editor;
class MPMappingOptionsWidget;
class QCheckBox;


class BrushSettingEditWidget : public QWidget
{
    Q_OBJECT
public:
    BrushSettingEditWidget(BrushSettingCategoryType settingCategoryType, const QString name, BrushSettingType settingType, qreal min, qreal max, QWidget* parent = nullptr);
    BrushSettingEditWidget(BrushSettingCategoryType settingCategoryType, BrushSetting setting, QWidget* parent = nullptr);

    void setCore(Editor* editor);
    void updateUI();
    void initUI();

    void hideMappingUI();

    void setValue(qreal value);

    void closeMappingWindow();

    const BrushSettingWidget* brushSettingWidget() { return mSettingWidget; }
    BrushSettingType settingType();
    QString settingName();

    bool isChecked() const;

public slots:
    void notifyInputMappingRemoved(BrushInputType input);
    void visibilityChanged(bool state);

Q_SIGNALS:
    void notifyMappingWidgetNeedsUpdate(BrushInputType input);

    void brushMappingForInputChanged(QVector<QPointF> points, BrushSettingType setting, BrushInputType inputType);
    void brushMappingRemoved(BrushSettingType setting, BrushInputType);
    void brushSettingToggled(BrushSettingCategoryType settingCategoryType, QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
    void brushSettingChanged(qreal value, BrushSettingType setting);

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

    BrushInputType mCurrentInputType;
    BrushSettingType mSettingType;
    BrushSettingCategoryType mSettingCategoryType;

    BrushSettingWidget* mSettingWidget = nullptr;
    MPMappingOptionsWidget* mMappingOptionsWidget = nullptr;
};

#endif // BRUSHSETTINGEDITWIDGET_H
