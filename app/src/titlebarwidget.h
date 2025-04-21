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
#ifndef TITLEBARWIDGET_H
#define TITLEBARWIDGET_H

#include <QWidget>
#include <QAbstractButton>

class QLabel;
class QHBoxLayout;
class QPushButton;

class TitleBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBarWidget();

    QSize minimumSizeHint() const override;

    void resizeEvent(QResizeEvent* resizeEvent) override;
    void setWindowTitle(const QString& title);
    void paintEvent(QPaintEvent*) override;

signals:
    void closeButtonPressed();
    void undockButtonPressed();

private:

    void hideButtons(bool hide);

    QWidget* createNormalTitleBarWidget(QWidget* parent);

    QWidget* mNormalTitleBarWidget = nullptr;

    QLabel* mTitleLabel = nullptr;
    QPushButton* mCloseButton = nullptr;
    QPushButton* mDockButton = nullptr;
};

#endif // TITLEBARWIDGET_H
