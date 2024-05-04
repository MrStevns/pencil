#include "pencildocktitlebarwidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QPushButton>
#include <QResizeEvent>
#include <QPropertyAnimation>

#include <QDebug>

PencilDockTitleBarWidget::PencilDockTitleBarWidget(QWidget *parent)
    : QWidget{parent}
{

    QVBoxLayout* vLayout = new QVBoxLayout();

    vLayout->setContentsMargins(0,3,0,0);
    vLayout->setSpacing(3);

    QFrame* lineFrame = new QFrame(this);
    lineFrame->setFrameShape(QFrame::HLine);
    lineFrame->setFrameShadow(QFrame::Sunken);

    mNormalTitleBarWidget = createNormalTitleBarWidget();

    mCompactTitleBarWidget = createCompactTitleBarWidget();
    vLayout->addWidget(mNormalTitleBarWidget);
    vLayout->addWidget(mCompactTitleBarWidget);
    vLayout->addWidget(lineFrame);

    setLayout(vLayout);
}

QWidget* PencilDockTitleBarWidget::createNormalTitleBarWidget()
{
    QWidget* containerWidget = new QWidget();

    QHBoxLayout* layout = new QHBoxLayout();

    QPushButton* closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon("://icons/themes/playful/window/window-close-button.svg"));
    closeButton->setFlat(true);
    closeButton->setIconSize(QSize(15,15));
    closeButton->setFixedSize(QSize(16,16));
    closeButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    QPushButton* undockButton = new QPushButton(this);
    undockButton->setIcon(QIcon("://icons/themes/playful/window/window-undock-button.svg"));
    undockButton->setFlat(true);
    undockButton->setIconSize(QSize(15,15));
    undockButton->setFixedSize(QSize(16,16));
    undockButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    mTitleLabel = new QLabel(this);
    QFont font;
    font.setPointSize(11);
    mTitleLabel->setFont(font);
    mTitleLabel->setAlignment(Qt::AlignVCenter);

    layout->addWidget(closeButton);
    layout->addWidget(undockButton);
    layout->addWidget(mTitleLabel);
    layout->setSpacing(3);
    layout->setContentsMargins(3,0,3,2);

    containerWidget->setLayout(layout);

    return containerWidget;
}

void PencilDockTitleBarWidget::setWindowTitle(const QString &title)
{
    mTitleLabel->setText(title);
}

QWidget* PencilDockTitleBarWidget::createCompactTitleBarWidget()
{
    QWidget* containerWidget = new QWidget();

    QHBoxLayout* layout = new QHBoxLayout(containerWidget);

    QLabel* handleLabel = new QLabel(containerWidget);
    handleLabel->setFixedSize(QSize(16,16));
    handleLabel->setPixmap(QPixmap("://icons/themes/playful/window/window-drag-handle.svg"));

    layout->addWidget(handleLabel);
    layout->setContentsMargins(3,0,2,2);
    layout->setSpacing(0);

    containerWidget->setLayout(layout);
    return containerWidget;

}

QSize PencilDockTitleBarWidget::minimumSizeHint() const
{
    return QSize(22, 100);
}

void PencilDockTitleBarWidget::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);

    if (resizeEvent->size().width() < 60) {
        mNormalTitleBarWidget->setVisible(false);
        mCompactTitleBarWidget->setVisible(true);
    } else if (resizeEvent->size().width() >= 60) {
        mNormalTitleBarWidget->setVisible(true);
        mCompactTitleBarWidget->setVisible(false);
    }
}
