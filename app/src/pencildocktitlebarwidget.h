#ifndef PENCILDOCKTITLEBARWIDGET_H
#define PENCILDOCKTITLEBARWIDGET_H

#include <QWidget>
#include <QAbstractButton>
#include <QDockWidget>


class QLabel;
class QHBoxLayout;
class QPushButton;

class PencilDockTitleBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PencilDockTitleBarWidget();

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
    QAbstractButton* mCloseButton = nullptr;
    QAbstractButton* mDockButton = nullptr;
};


class PencilDockWidgetTitleButton : public QAbstractButton
{
    Q_OBJECT
public:
    PencilDockWidgetTitleButton(QWidget* widget = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return sizeHint(); }
    void paintEvent(QPaintEvent* event) override;

};


#endif // PENCILDOCKTITLEBARWIDGET_H
