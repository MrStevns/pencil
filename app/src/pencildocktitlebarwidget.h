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

signals:
    void closeButtonPressed();
    void undockButtonPressed();

private:

    QWidget* createNormalTitleBarWidget(QWidget* parent);
    QWidget* createCompactTitleBarWidget(QWidget* parent);

    QWidget* mNormalTitleBarWidget = nullptr;
    QWidget* mCompactTitleBarWidget = nullptr;

    QLabel* mTitleLabel = nullptr;
    QPushButton* mCloseButton = nullptr;
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
