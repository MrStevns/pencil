#ifndef PENCILDOCKTITLEBARWIDGET_H
#define PENCILDOCKTITLEBARWIDGET_H

#include <QWidget>


class QLabel;
class QHBoxLayout;
class QPropertyAnimation;

class PencilDockTitleBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PencilDockTitleBarWidget(QWidget *parent = nullptr);

    QSize minimumSizeHint() const override;

    void resizeEvent(QResizeEvent* resizeEvent) override;
    void setWindowTitle(const QString& title);

signals:

private:
    QWidget* createNormalTitleBarWidget();
    QWidget* createCompactTitleBarWidget();

    QWidget* mNormalTitleBarWidget = nullptr;
    QWidget* mCompactTitleBarWidget = nullptr;

    QLabel* mTitleLabel = nullptr;
    QPropertyAnimation* animation = nullptr;
};

#endif // PENCILDOCKTITLEBARWIDGET_H
