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
#include <QResizeEvent>
#include <QStyle>
#include <QPainter>
#include <QMenu>
#include <QEvent>
#include <QDebug>

#include "platformhandler.h"

TitleBarWidget::TitleBarWidget(QWidget* parent)
    : QWidget(parent)
{

    QVBoxLayout* vLayout = new QVBoxLayout();

    vLayout->setContentsMargins(3,4,3,4);
    vLayout->setSpacing(0);

    mNormalTitleBarWidget = createNormalTitleBarWidget(this);

    vLayout->addWidget(mNormalTitleBarWidget);

    setLayout(vLayout);
}

TitleBarWidget::~TitleBarWidget()
{
}

QWidget* TitleBarWidget::createNormalTitleBarWidget(QWidget* parent)
{
    bool isDarkmode = PlatformHandler::isDarkMode();
    QWidget* containerWidget = new QWidget(parent);

    mContainerLayout = new QHBoxLayout(parent);

    mCloseButton = new QPushButton(parent);
    mCloseButton->setFlat(true);

    QSize iconSize = QSize(14,14);
    QSize padding = QSize(2,2);

    IconResource closeButtonRes;
    closeButtonRes.lightMode = QIcon("://icons/themes/playful/window/window-close-button-normal.svg");
    closeButtonRes.darkMode = QIcon("://icons/themes/playful/window/window-close-button-normal-darkm.svg");

    QIcon closeIcon = closeButtonRes.iconForMode(isDarkmode);

    IconResource closeHoverButtonRes;
    closeHoverButtonRes.lightMode = QIcon("://icons/themes/playful/window/window-close-button-active.svg");
    closeHoverButtonRes.darkMode = closeHoverButtonRes.lightMode;

    mCloseButton->setIcon(closeIcon);
    mCloseButton->setIconSize(iconSize);
    mCloseButton->setFixedSize(iconSize + padding);
    mCloseButton->installEventFilter(new ButtonAppearanceWatcher(closeButtonRes,
                                                                 closeHoverButtonRes,
                                                            this));

    connect(mCloseButton, &QPushButton::clicked, this, [this] {
        emit closeButtonPressed();
    });

    IconResource dockButtonRes;
    dockButtonRes.lightMode = QIcon("://icons/themes/playful/window/window-float-button-normal.svg");
    dockButtonRes.darkMode = QIcon("://icons/themes/playful/window/window-float-button-normal-darkm.svg");

    IconResource dockHoverButtonRes;
    dockHoverButtonRes.lightMode = QIcon("://icons/themes/playful/window/window-float-button-active.svg");
    dockHoverButtonRes.darkMode = dockHoverButtonRes.lightMode;

    mDockButton = new QPushButton(parent);

    QIcon dockIcon = dockButtonRes.iconForMode(isDarkmode);
    mDockButton->setIcon(dockIcon);
    mDockButton->setFlat(true);

    mDockButton->setIconSize(iconSize);
    mDockButton->setFixedSize(iconSize + padding);
    mDockButton->installEventFilter(new ButtonAppearanceWatcher(dockButtonRes, dockHoverButtonRes, this));

    connect(mDockButton, &QPushButton::clicked, this, [this] {
       emit undockButtonPressed();
    });

    mMenu = new QMenu(this);
    mMenu->addAction(closeIcon, tr("Close"), [=] {
        emit closeButtonPressed();
    });
    mDockAction = mMenu->addAction(dockIcon, "", [=] {
        emit undockButtonPressed();
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [=](const QPoint& pos) {

        if (mIsFloating) {
            mDockAction->setText(tr("Dock"));
        } else {
            mDockAction->setText(tr("Undock"));
        }
        mMenu->exec(mapToGlobal(pos));
    });

    mTitleLabel = new QLabel(parent);
    mTitleLabel->setAlignment(Qt::AlignVCenter);

#ifdef __APPLE__
    mContainerLayout->addWidget(mCloseButton);
    mContainerLayout->addWidget(mDockButton);
    mContainerLayout->addWidget(mTitleLabel);
#else
    mContainerLayout->addWidget(mTitleLabel);
    mContainerLayout->addWidget(mDockButton);
    mContainerLayout->addWidget(mCloseButton);
#endif

    mContainerLayout->setSpacing(3);
    mContainerLayout->setContentsMargins(0,0,0,0);

    containerWidget->setLayout(mContainerLayout);
    containerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    return containerWidget;
}

void TitleBarWidget::setWindowTitle(const QString &title)
{
    mTitleLabel->setText(title);
}

void TitleBarWidget::hideButtons(bool hide)
{
    mCloseButton->setHidden(hide);
    mDockButton->setHidden(hide);
}

void TitleBarWidget::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);

    hideButtonsIfNeeded(resizeEvent->size().width());
}

void TitleBarWidget::hideButtonsIfNeeded(int width)
{
    if (width <= mWidthOfFullLayout) {
        hideButtons(true);
    } else {
        hideButtons(false);
    }
}

void TitleBarWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // This is to ensure that after the titlebar has been hidden with buttons hidden
    // the layout width is smaller, so we enable them again briefly to get the correct width.
    hideButtons(false);

    mWidthOfFullLayout = layout()->sizeHint().width();
    hideButtonsIfNeeded(size().width());
}

void TitleBarWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.save();
    painter.setBrush(palette().color(QPalette::Midlight));
    painter.setPen(Qt::NoPen);
    painter.drawRect(this->rect());

    QPen pen(palette().color(QPalette::Mid));
    int penWidth = 1;
    pen.setWidth(penWidth);
    painter.setPen(pen);
    painter.drawLine(QPoint(this->rect().x(),
                            this->rect().height()-penWidth),
                     QPoint(this->rect().width(),
                            this->rect().height()-penWidth));
    painter.restore();
}

ButtonAppearanceWatcher::ButtonAppearanceWatcher(IconResource normalIconResource,
                                                 IconResource hoverIconResource,
                                                 QObject* parent) :
    QObject(parent),
    mNormalIconResource(normalIconResource),
    mHoverIconResource(hoverIconResource)
{}

bool ButtonAppearanceWatcher::eventFilter(QObject* watched, QEvent* event)
{
    QAbstractButton* button = qobject_cast<QAbstractButton*>(watched);
    if (!button) {
        return false;
    }

    IconResource res = mNormalIconResource;
    AppearanceEventType apType = determineAppearanceEvent(event);

    if (shouldUpdateResource(event, apType)) {
        if (event->type() == QEvent::ApplicationPaletteChange) {
            res = mNormalIconResource;
        }
        else if (event->type() == QEvent::Enter) {
            res = mHoverIconResource;
        }
        else if (event->type() == QEvent::Leave) {
            res = mNormalIconResource;
        }
        mOldAppearanceType = apType;

        bool isDarkmode = PlatformHandler::isDarkMode();
        button->setIcon(res.iconForMode(isDarkmode));
        return true;
    }

    return false;
}

AppearanceEventType ButtonAppearanceWatcher::determineAppearanceEvent(QEvent *event) const
{
    if (event->type() == QEvent::ApplicationPaletteChange) {
        bool isDarkmode = PlatformHandler::isDarkMode();
        if (isDarkmode) {
            return AppearanceEventType::DARK_APPEARANCE;
        } else {
            return AppearanceEventType::LIGHT_APPEARANCE;
        }
    } else if (event->type() == QEvent::Enter) {
        return AppearanceEventType::ICON_ACTIVE;
    } else if (event->type() == QEvent::Leave) {
        return AppearanceEventType::ICON_NORMAL;
    }

    return AppearanceEventType::NONE;
}

bool ButtonAppearanceWatcher::shouldUpdateResource(QEvent* event, AppearanceEventType appearanceType) const
{
    if (appearanceType == mOldAppearanceType) { return false; }

    return determineAppearanceEvent(event) != AppearanceEventType::NONE;
}
