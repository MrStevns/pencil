/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "titlebarwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include <QToolButton>
#include <QResizeEvent>
#include <QAbstractButton>
#include <QStyle>
#include <QPainter>
#include <QStyleOptionToolButton>

#include <QDebug>

TitleBarWidget::TitleBarWidget()
    : QWidget()
{

    QVBoxLayout* vLayout = new QVBoxLayout();

    vLayout->setContentsMargins(0,3,0,0);
    vLayout->setSpacing(3);
    vLayout->setSizeConstraint(QVBoxLayout::SetMinAndMaxSize);

    mNormalTitleBarWidget = createNormalTitleBarWidget(this);

    vLayout->addWidget(mNormalTitleBarWidget);

    setLayout(vLayout);
}

QWidget* TitleBarWidget::createNormalTitleBarWidget(QWidget* parent)
{
    QWidget* containerWidget = new QWidget(parent);

    QHBoxLayout* layout = new QHBoxLayout(parent);

    mCloseButton = new QPushButton(parent);
    mCloseButton->setFlat(true);

    QSize iconSize = QSize(14,14);
    QSize padding = QSize(2,2);
    QIcon closeIcon(":/icons/themes/playful/window/window-close-button.svg");

    mCloseButton->setIcon(closeIcon);
    mCloseButton->setIconSize(iconSize);
    mCloseButton->setFixedSize(iconSize + padding);

    connect(mCloseButton, &QPushButton::clicked, this, [this] {
        emit closeButtonPressed();
    });

    mDockButton = new QPushButton(parent);
    mDockButton->setIcon(QIcon("://icons/themes/playful/window/window-float-button.svg"));
    mDockButton->setFlat(true);

    mDockButton->setIconSize(iconSize);
    mDockButton->setFixedSize(iconSize + padding);

    connect(mDockButton, &QPushButton::clicked, this, [this] {
       emit undockButtonPressed();
    });

    mTitleLabel = new QLabel(parent);
    mTitleLabel->setAlignment(Qt::AlignVCenter);

    layout->addWidget(mCloseButton);
    layout->addWidget(mDockButton);
    layout->addWidget(mTitleLabel);
    layout->setSpacing(3);
    layout->setContentsMargins(3,0,3,2);

    containerWidget->setLayout(layout);
    containerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    containerWidget->setFixedHeight(18);

    return containerWidget;
}

void TitleBarWidget::setWindowTitle(const QString &title)
{
    mTitleLabel->setText(title);
}

QSize TitleBarWidget::minimumSizeHint() const
{
    return QSize(16, 32);
}

void TitleBarWidget::hideButtons(bool hide)
{
    mCloseButton->setHidden(hide);
    mDockButton->setHidden(hide);
}

void TitleBarWidget::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);

    if (resizeEvent->size().width() < 75) {
        hideButtons(true);
    } else if (resizeEvent->size().width() >= 75) {
        hideButtons(false);
    }
}

void TitleBarWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.save();
    painter.setBrush(palette().color(QPalette::Midlight));
    painter.setPen(Qt::NoPen);
    painter.drawRect(this->rect());

    QPen pen(palette().color(QPalette::Mid));
    pen.setWidth(1);
    painter.setPen(pen);
    painter.drawLine(QPoint(this->rect().x(), this->rect().height()), QPoint(this->rect().width(), this->rect().height()));
    painter.restore();
}
