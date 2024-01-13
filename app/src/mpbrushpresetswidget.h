#ifndef MPBRUSHPRESETSWIDGET_H
#define MPBRUSHPRESETSWIDGET_H

#include <QVector>
#include <QItemSelection>
#include <QListWidgetItem>

#include "mpbrushutils.h"
#include "mpbrushmanager.h"

struct MPBrushPreset;

namespace Ui {
class MPBrushPresetsWidget;
}

class MPBrushPresetsWidget : public QWidget
{

    enum class PresetState {
        ADDING,
        RENAMING,
        REMOVING,
        NONE
    };

    Q_OBJECT
public:
    explicit MPBrushPresetsWidget(QWidget* parent = nullptr);

    void setBrushManager(MPBrushManager* manager) { mBrushManager = manager; }
signals:
    void presetsChanged();

private:
    void loadPresets();
    void addNewPreset();
    void removePreset();

    QString createBlankName() const;

    void itemDoubleClicked(QListWidgetItem *item);
    void didChangeSelection(const QItemSelection &selected, const QItemSelection &deselected);

    void didCommitChanges(QWidget* widgetItem);
    void didChangeItem(QListWidgetItem* item);
    void didPressResetButton();

    Ui::MPBrushPresetsWidget* ui = nullptr;

    QVector<QString> mPresets;

    PresetState mState = PresetState::NONE;
    MPBrushManager* mBrushManager = nullptr;
};

#endif // MPBRUSHPRESETSWIDGET_H
