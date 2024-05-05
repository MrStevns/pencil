#include "pencildocktitlebarwidget.h"

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

PencilDockTitleBarWidget::PencilDockTitleBarWidget()
    : QWidget()
{

    QVBoxLayout* vLayout = new QVBoxLayout(this);

    vLayout->setContentsMargins(0,3,0,0);
    vLayout->setSpacing(3);
    vLayout->setSizeConstraint(QVBoxLayout::SetMinAndMaxSize);

    QFrame* lineFrame = new QFrame(this);
    lineFrame->setFrameShape(QFrame::HLine);
    lineFrame->setFrameShadow(QFrame::Sunken);
    lineFrame->setLineWidth(1);
    lineFrame->setPalette(palette().color(QPalette::Base));
    lineFrame->setContentsMargins(0,0,0,0);

    mNormalTitleBarWidget = createNormalTitleBarWidget(this);
    mCompactTitleBarWidget = createCompactTitleBarWidget(this);

    vLayout->addWidget(mNormalTitleBarWidget);
    vLayout->addWidget(mCompactTitleBarWidget);
    vLayout->addWidget(lineFrame);

    setLayout(vLayout);
}

QWidget* PencilDockTitleBarWidget::createNormalTitleBarWidget(QWidget* parent)
{
    QWidget* containerWidget = new QWidget(parent);

    QHBoxLayout* layout = new QHBoxLayout(parent);

    PencilDockWidgetTitleButton* closeButton = new PencilDockWidgetTitleButton(parent);

    QSize iconSize = QSize(14,14);
    QSize padding = QSize(2,2);
    closeButton->setIcon(QIcon("://icons/themes/playful/window/window-close-button-14.svg"));
    closeButton->setIconSize(iconSize);
    closeButton->setFixedSize(iconSize + padding);

    connect(closeButton, &PencilDockWidgetTitleButton::clicked, this, [this] {
        emit closeButtonPressed();
    });

    PencilDockWidgetTitleButton* undockButton = new PencilDockWidgetTitleButton(parent);
    undockButton->setIcon(QIcon("://icons/themes/playful/window/window-float-button-14.svg"));
    undockButton->setIconSize(iconSize);
    undockButton->setFixedSize(iconSize + padding);

    connect(undockButton, &PencilDockWidgetTitleButton::clicked, this, [this] {
       emit undockButtonPressed();
    });


    mTitleLabel = new QLabel(parent);
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
    containerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    return containerWidget;
}

void PencilDockTitleBarWidget::setWindowTitle(const QString &title)
{
    mTitleLabel->setText(title);
}

QWidget* PencilDockTitleBarWidget::createCompactTitleBarWidget(QWidget* parent)
{
    QWidget* containerWidget = new QWidget(parent);

    QHBoxLayout* layout = new QHBoxLayout(parent);

    QLabel* handleLabel = new QLabel(parent);
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
    return QSize(16, 100);
}

void PencilDockTitleBarWidget::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);

    qDebug() << resizeEvent->size();
    if (resizeEvent->size().width() < 80) {
        mNormalTitleBarWidget->setVisible(false);
        mCompactTitleBarWidget->setVisible(true);
    } else if (resizeEvent->size().width() >= 80) {
        mNormalTitleBarWidget->setVisible(true);
        mCompactTitleBarWidget->setVisible(false);
    }
}


PencilDockWidgetTitleButton::PencilDockWidgetTitleButton(QWidget* widget)
    : QAbstractButton(widget)
{
    setFocusPolicy(Qt::NoFocus);
}

QSize PencilDockWidgetTitleButton::sizeHint() const
{
    ensurePolished();
    int size = 2 * style()->pixelMetric(QStyle::PM_TitleBarButtonIconSize, nullptr, this);

    if (!icon().isNull()) {
        const QSize sz = icon().actualSize(iconSize());
        size += qMax(sz.width(), sz.height());
    }
    return QSize(size, size);
}

void PencilDockWidgetTitleButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QStyleOptionToolButton opt;
    opt.init(this);

    opt.icon = icon();
    opt.subControls = { };
    opt.activeSubControls = { };
    opt.features = QStyleOptionToolButton::None;
    opt.arrowType = Qt::NoArrow;
    opt.iconSize = iconSize();
    style()->drawComplexControl(QStyle::CC_ToolButton, &opt, &p, this);
}


