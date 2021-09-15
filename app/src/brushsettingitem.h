#ifndef BRUSHSETTINGITEM_H
#define BRUSHSETTINGITEM_H

#include <QTreeWidgetItem>

#include "mpbrushsettingcategories.h"

class BrushSettingTreeItem : public QTreeWidgetItem
{

public:

    BrushSettingTreeItem(BrushSettingCategoryType category, QTreeWidget* parent = nullptr);
    BrushSettingTreeItem(BrushSettingCategoryType category, QTreeWidgetItem* parent = nullptr);

    BrushSettingCategoryType categoryType() { return mCategory; }

private:

    BrushSettingCategoryType mCategory;
};

#endif // BRUSHSETTINGITEM_H
