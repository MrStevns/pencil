#ifndef MPBRUSHCONFIGURATOR_H
#define MPBRUSHCONFIGURATOR_H

#include "basedockwidget.h"

#include <QDialog>
#include <QTreeWidgetItem>

#include "mpbrushsettingcategories.h"
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
    void notifyBrushSettingToggled(BrushSettingCategoryType settingCategoryType, QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
private:

    void prepareUpdateBrushPreview();
    void updateMapValuesButton();
    void updateSettingsView(QTreeWidgetItem* item);

    void prepareBrushInputChanges(QVector<QPointF> points, BrushSettingType settingType, BrushInputType input);
    void removeBrushMappingForInput(BrushSettingType setting, BrushInputType input);

    void addBrushSettingsSpacer();
    void removeBrushSettingSpacer();

    void setupActiveSettings();
    void setupBasicBrushSettings();
    void setupAdvancedBrushSettings();

    void setupSettingsFor(BrushSettingCategoryType type);

    void showNotImplementedPopup();

    void pressedRemoveBrush();
    void pressedEditBrush();
    void pressedCloneBrush();
    void pressedDiscardBrush();
    void onResetButtonPressed();

    void openBrushInfoWidget(DialogContext dialogContext);

    BrushSettingTreeItem* addTreeRoot(BrushSettingCategoryType category, QTreeWidget* treeWidget, const QString name);
    BrushSettingTreeItem* addTreeChild(BrushSettingCategoryType category, QTreeWidgetItem* parent, const QString name);

    void brushCategorySelected(QTreeWidgetItem* item, int);
    void brushCategorySelectionChanged(const QItemSelection &selected, const QItemSelection &);

    QVBoxLayout* vBoxLayout = nullptr;
    QWidget* mBrushSettingsWidget = nullptr;
    QTreeWidget* mNavigatorWidget = nullptr;
    QLabel* mBrushImageWidget = nullptr;
    MPBrushPreview* mBrushPreviewWidget = nullptr;
    QSpacerItem* mLayoutSpacer = nullptr;

    BrushSettingTreeItem* mActiveTreeRoot = nullptr;

    QPointer<MPBrushInfoDialog> mBrushInfoWidget = nullptr;

    QPushButton* mDiscardChangesButton = nullptr;
    QPushButton* mMapValuesButton = nullptr;
    bool mMapValuesButtonPressed = false;

    Editor* mEditor = nullptr;

    QList<BrushSettingEditWidget*> mBrushWidgets;
    QString mBrushName;
    QString mPreset;
    ToolType mToolType;

    QSize mImageSize = QSize(100,100);

    QList<QMetaObject::Connection> mListOfConnections;
    MPBrushSettingCategories settingCategories;
};

#endif // MPBRUSHCONFIGURATOR_H
