#ifndef PENCILDOCKTITLEBARWIDGET_H
#define PENCILDOCKTITLEBARWIDGET_H

#include <QWidget>

class QHBoxLayout;
class QPropertyAnimation;

class PencilDockTitleBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PencilDockTitleBarWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

    void resizeEvent(QResizeEvent* resizeEvent) override;

signals:

private:
    QWidget* createNormalTitleBarWidget();
    QWidget* createCompactTitleBarWidget();

    QWidget* mNormalTitleBarWidget = nullptr;
    QWidget* mCompactTitleBarWidget = nullptr;

    QPropertyAnimation* animation = nullptr;
    // QHBoxLayout* mHCompactLayout = nullptr;
    // QHBoxLayout* mHorizontalButtonLayout = nullptr;
};

#endif // PENCILDOCKTITLEBARWIDGET_H
