#ifndef MPBRUSHCONFIGURATOR_H
#define MPBRUSHCONFIGURATOR_H

#include "basedockwidget.h"

#include <QDialog>
#include <QTreeWidgetItem>

#include "brushsetting.h"
#include "brushsettingitem.h"

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
class MPBrushPreview;

class MPBrushConfigurator : public QDialog
{
    Q_OBJECT
    Q_ENUM(BrushSettingType)
    Q_ENUM(BrushInputType)
public:
    MPBrushConfigurator(QWidget* parent = nullptr);

    void initUI();
    void updateUI();
    void hideUI();

    void setCore(Editor* editor) { mEditor = editor; }

    void updateConfig();

    void setBrushSettingValue(qreal value, BrushSettingType setting);
    void prepareBrushChanges(qreal value, BrushSettingType setting);

signals:
    void brushRemoved();
    void brushSettingChanged(qreal value, BrushSettingType setting);

    /** reloadBrushSettings
     *  Reloads brush settings from disk
     *  Use in case the brush needs to refresh after modifications.
     */
    void reloadBrushSettings();

    void notifyBrushInfoUpdated(QString brushName, QString brushPreset);
    void notifyBrushSettingToggled(QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
private:

    void backupBrushSetting(BrushSettingType setting);
    void backupBrushMapping(BrushSettingType setting, BrushInputType input);
    void prepareUpdateBrushPreview();
    void updateMapValuesButton();
    void updateSettingsView(QTreeWidgetItem* item);

    void prepareBrushInputChanges(QVector<QPointF> points, BrushSettingType settingType, BrushInputType input);
    void removeBrushMappingForInput(BrushSettingType setting, BrushInputType input);

    void addBrushSettingsSpacer();
    void removeBrushSettingSpacers();

    void prepareActiveSettings();

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

    void openBrushInfoWidget(DialogContext dialogContext);

    BrushSettingItem* addTreeRoot(BrushSettingItem::Category category, QTreeWidget* treeWidget, const QString name);
    BrushSettingItem* addTreeChild(BrushSettingItem::Category category, QTreeWidgetItem* parent, const QString name);

    void brushCategorySelected(QTreeWidgetItem* item, int);
    void brushCategorySelectionChanged(const QItemSelection &selected, const QItemSelection &);

    QVBoxLayout* vBoxLayout = nullptr;
    QWidget* mBrushSettingsWidget = nullptr;
    QTreeWidget* mNavigatorWidget = nullptr;
    QLabel* mBrushImageWidget = nullptr;
    MPBrushPreview* mBrushPreviewWidget = nullptr;

    BrushSettingItem* mActiveTreeRoot = nullptr;

    QPointer<MPBrushInfoDialog> mBrushInfoWidget = nullptr;

    QPushButton* mDiscardChangesButton = nullptr;
    QPushButton* mMapValuesButton = nullptr;
    bool mMapValuesButtonPressed = false;

    Editor* mEditor = nullptr;

    QList<BrushSettingEditWidget*> mBrushWidgets;
    QHash<int, BrushChanges> mCurrentModifications;
    QHash<int, BrushChanges> mOldModifications;

    QSize mImageSize = QSize(100,100);

    QList<QMetaObject::Connection> mListOfConnections;
};

#endif // MPBRUSHCONFIGURATOR_H
