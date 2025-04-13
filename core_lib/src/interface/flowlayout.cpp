/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QWidget>
#include <QLayout>
#include <QtMath>
#include <QDebug>

#include "flowlayout.h"

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int FlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}

int FlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return nullptr;
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true).height;
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    mState = doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    QLayoutItem *item;
    foreach (item, itemList)
        size = size.expandedTo(item->minimumSize());
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    size += QSize(left + right, top + bottom);
    return size;
}

RowLayoutInfo FlowLayout::alignJustifiedRow(int startIndex, int count, const QRect& effectiveRect, int spaceX) const
{
    int rowWidth = calculateRowWidth(count, spaceX);
    int extraSpace = effectiveRect.width() - rowWidth;
    int spacing = (count > 1) ? extraSpace / count : 0;

    int itemX = (count == 1)
            ? effectiveRect.left() // Align first item to the left
            : spaceX + qFloor((effectiveRect.width() - spacing) / count) - itemList.at(startIndex)->geometry().width();

    RowLayoutInfo row;

    row.startX = itemX;
    row.startIndex = startIndex;
    row.spacing = spaceX + spacing;

    for (int j = startIndex; j < startIndex + count; j += 1) {
        QLayoutItem *rowItem = itemList.at(j);
        QSize itemSize = rowItem->geometry().size();
        rowItem->setGeometry(QRect(QPoint(itemX, rowItem->geometry().y()), itemSize));
        itemX += itemSize.width() + spaceX + spacing;
    }

    return row;
}

RowLayoutInfo FlowLayout::alignHCenterRow(int startIndex, int count, const QRect &effectiveRect, int spaceX) const
{
    int rowWidth = calculateRowWidth(count, spaceX);
    int offset = (effectiveRect.width() - rowWidth) / 2;
    int rowOffsetX = effectiveRect.x() + offset;

    RowLayoutInfo row;

    row.startX = rowOffsetX;
    row.startIndex = startIndex;
    row.spacing = spaceX;

    for (int i = startIndex; i < startIndex + count; i += 1) {
        QLayoutItem *rowItem = itemList.at(i);

        QSize itemSize = rowItem->geometry().size();
        rowItem->setGeometry(QRect(QPoint(rowOffsetX, rowItem->geometry().y()), itemSize));
        rowOffsetX += itemSize.width() + spaceX;
    }

    return row;
}


void FlowLayout::alignRowFromRowInfo(int startIndex, int count, RowLayoutInfo rowInfo) const
{
    int x = rowInfo.startX;

    for (int i = startIndex; i < startIndex + count; i += 1) {

        if (i > itemList.length() - 1) {
            break;
        }
        QLayoutItem *item = itemList.at(i);
        QSize size = item->geometry().size();
        item->setGeometry(QRect(QPoint(x, item->geometry().y()), size));
        x += size.width() + rowInfo.spacing;
    }
}


FlowLayoutState FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    int spaceX = horizontalSpacing();
    int spaceY = verticalSpacing();

    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;
    int rowCount = 0;

    QLayoutItem *item;

    QVector<RowLayoutInfo> rowAlignments;
    for (int i = 0; i < itemList.length(); i += 1) {
        item = itemList.at(i);

        int rowWidth = calculateRowWidth(rowCount, spaceX);

        if (rowWidth + item->sizeHint().width() >= effectiveRect.width() && rowCount > 0) {
            if (!testOnly && alignment() & Qt::AlignHCenter) {
                rowAlignments.append(alignHCenterRow(i - rowCount, rowCount, effectiveRect, spaceX));
            } else if (!testOnly && alignment() & Qt::AlignJustify) {
                rowAlignments.append(alignJustifiedRow(i - rowCount, rowCount, effectiveRect, spaceX));
            }

            y = y + lineHeight + spaceY;
            lineHeight = 0;
            rowCount = 0;
        }

        // Set item back to start of next row
        if (!testOnly) {
            item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
        }

        lineHeight = qMax(lineHeight, item->sizeHint().height());
        rowCount++;
    }

    if (rowCount > 0 && !testOnly) {
        if (rowAlignments.count() > 0) {
            alignRowFromRowInfo(itemList.length() - rowCount, rowCount, rowAlignments.last());
        } else {
            RowLayoutInfo fallback = {
                        itemList.length() - rowCount,
                        effectiveRect.x(),
                        spaceX
                    };
            alignRowFromRowInfo(itemList.length() - rowCount, rowCount, fallback);
        }
    }

    FlowLayoutState state;
    state.rowCount = rowCount;
    state.height = bottom + y + lineHeight - rect.y();

    return state;
}

int FlowLayout::calculateRowWidth(int rowCount, int spacing) const
{
    if (itemList.isEmpty()) { return 0; }

    int totalWidth = 0;
    // Calculate the total width of all item in row
    for (int i = 0; i < rowCount; i += 1) {
        totalWidth += itemList.at(i)->sizeHint().width() + spacing;
    }

    return totalWidth;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject* parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}
