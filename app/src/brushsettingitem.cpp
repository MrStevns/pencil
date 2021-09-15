#include "brushsettingitem.h"

BrushSettingTreeItem::BrushSettingTreeItem(BrushSettingCategoryType categoryType, QTreeWidget* parent) : QTreeWidgetItem(parent),
    mCategory(categoryType)
{
}

BrushSettingTreeItem::BrushSettingTreeItem(BrushSettingCategoryType categoryType, QTreeWidgetItem* parent) : QTreeWidgetItem(parent),
    mCategory(categoryType)
{
}
