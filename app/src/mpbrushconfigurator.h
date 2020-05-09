#ifndef MPBRUSHCONFIGURATOR_H
#define MPBRUSHCONFIGURATOR_H

#include "basedockwidget.h"

#include <QDialog>
#include <QTreeWidgetItem>

#include "brushsetting.h"
#include "brushsettingitem.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>

#include <QPointer>

#include "mpbrushmanager.h"
#include "pencildef.h"
#include "mpbrushinfodialog.h"

class QVBoxLayout;
class QSpacerItem;
class BrushSettingEditWidget;
class QLabel;
class MPBrushInfoDialog;
class BrushSettingWidget;

class MPBrushConfigurator : public QDialog
{
    Q_OBJECT
public:
    MPBrushConfigurator(QWidget* parent = nullptr);

    void initUI();
    void updateUI();

    void setCore(Editor* editor) { mEditor = editor; }

    void updateConfig();

    void saveChanges(qreal startValue, qreal value, BrushSettingType settingType);
    void updateBrushSetting(qreal value, BrushSettingType setting);

signals:
    void updateBrushList(QString brushName, QString brushPreset);
    void refreshBrushList();
    void brushSettingChanged(qreal value, BrushSettingType setting);

    /** reloadBrushSettings
     *  Reloads brush settings from disk
     *  Use in case the brush needs to refresh after modifications.
     */
    void reloadBrushSettings();
    void toggleSettingForBrushSetting(QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
private:

    void updateMapValuesButton();
    void updateSettingsView(QTreeWidgetItem* item);

    void updateBrushMapping(QVector<QPointF> points, BrushSettingType settingType, BrushInputType input);
    void removeBrushMappingForInput(BrushSettingType setting, BrushInputType input);

    void addBrushSettingsSpacer();
    void removeBrushSettingSpacers();

    void prepareBasicBrushSettings();
    void prepareAdvancedBrushSettings();

    void prepareOpacitySettings();
    void prepareDabSettings();
    void prepareRandomSettings();
    void prepareSpeedSettings();
    void prepareOffsetSettings();
    void prepareTrackingSettings();
    void prepareColorSettings();
    void prepareSmudgeSettings();
    void prepareEraserSetting();
    void prepareStrokeSettings();
    void prepareCustomInputSettings();
    void prepareEllipticalDabSettings();
    void prepareOtherSettings();

    void showNotImplementedPopup();

    void pressedRemoveBrush();
    void pressedEditBrush();
    void pressedCloneBrush();
    void pressedDiscardBrush();

//    void applyChanges(QHash<int, BrushChanges> changes);
//    void writeModifications(QJsonDocument& document, QJsonParseError& error, QHash<int, BrushChanges> modifications);

    void openBrushInfoWidget(DialogContext dialogContext);

    BrushSettingItem* addTreeRoot(BrushSettingItem::Category category, QTreeWidget* treeWidget, const QString name);
    BrushSettingItem* addTreeChild(BrushSettingItem::Category category, QTreeWidgetItem* parent, const QString name);

//    QWidget* addBrushSetting(const QString name, BrushSettingType setting);
    void brushCategorySelected(QTreeWidgetItem* item, int);
    void brushCategorySelectionChanged(const QItemSelection &selected, const QItemSelection &);

    QVBoxLayout* vBoxLayout = nullptr;
    QWidget* mBrushSettingsWidget = nullptr;
    QTreeWidget* mNavigatorWidget = nullptr;
    QLabel* mBrushImageWidget = nullptr;
    QLabel* mBrushNameWidget = nullptr;

    QPointer<MPBrushInfoDialog> mBrushInfoWidget = nullptr;

    QPushButton* mDiscardChangesButton = nullptr;
    QPushButton* mMapValuesButton = nullptr;
    bool mMapValuesButtonPressed = false;

    Editor* mEditor = nullptr;

    QList<BrushSettingEditWidget*> mBrushWidgets;
    QHash<int, BrushChanges> mCurrentModifications;
    QHash<int, BrushChanges> mOldModifications;
    QString mBrushName;
    QString mPreset;
    ToolType mToolType;

    QSize mImageSize = QSize(32,32);

    QList<QMetaObject::Connection> mListOfConnections;
//    QHash<int, BrushSettingWidget*> exportedBrushWidgets;
//    QList<BrushSettingWidget*> mVisibleWidgets;

};

#endif // MPBRUSHCONFIGURATOR_H
