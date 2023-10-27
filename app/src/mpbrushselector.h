/* brushlib - The MyPaint Brush Library (demonstration project)
 * Copyright (C) 2013 POINTCARRE SARL / Sebastien Leon email: sleon at pointcarre.com
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */
#ifndef MPBRUSHSELECTOR_H
#define MPBRUSHSELECTOR_H

#include <QMap>
#include <QString>
#include <QPointer>

#include "mpbrushutils.h"
#include "basedockwidget.h"
#include "pencildef.h"
#include "mpbrushsettingcategories.h"

class QListWidgetItem;
class QTabWidget;
class QListWidget;
class MPBrushConfigurator;
class ComboBox;
class QVBoxLayout;
class MPBrushPresetsWidget;
class BrushSettingWidget;

class MPBrushSelector : public BaseDockWidget
{

  Q_OBJECT
public:
  MPBrushSelector(QWidget* parent = nullptr );

  void loadToolBrushes(ToolType toolType);

  void initUI() override;
  void updateUI() override;

  void setCore(Editor* editor) { mEditor = editor; }

  MPBrushConfigurator* brushConfigurator() { return mBrushConfiguratorWidget; }

public slots:
  void setBrushAsSelected(QString brushName);
  void typeChanged(ToolType);
  void reloadBrushes();
  void updateBrushList(QString brushName, QString brushPreset);
  void showPresetManager(bool show);
  void showBrushConfigurator(bool show);
  void onDidReloadBrushSettings();

signals:
  void brushSelected();
  void notifyBrushSettingToggled(BrushSettingCategoryType settingCategoryType, QString name, BrushSettingType setting, qreal min, qreal max, bool visible);
  void notifyBrushSettingChanged(qreal value, BrushSettingType setting);
  void didReloadBrush();

protected slots:
  void itemClicked ( QListWidgetItem *);

private:

  Editor* mEditor = nullptr;

  void setLastUsedBrush(ToolType toolType, MPBrushPreset preset, QString brushName);
  bool loadBrushFromFile(const QString& brushName);
  void loadBrushesIntoList();
  void loadPresets();
  void addToolTabs();

  void changeBrushPreset(int index, QString name, int data);

  QVBoxLayout* mVLayout = nullptr;
  ComboBox* mPresetComboBox;
  QWidget* mTopAreaWidget = nullptr;
  QListWidget* mBrushListWidget = nullptr;

  MPBrushPreset currentBrushPreset;

  MPBrushConfigurator* mBrushConfiguratorWidget = nullptr;

  ToolType oldToolType;
  ToolType currentToolType;
  QByteArray currentBrushData;

  QPointer<MPBrushPresetsWidget> mPresetsWidget;
};



#endif // MPBRUSHSELECTOR_H
