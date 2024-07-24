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

    QVBoxLayout* vLayout = new QVBoxLayout();

    vLayout->setContentsMargins(0,3,0,0);
    vLayout->setSpacing(3);
    vLayout->setSizeConstraint(QVBoxLayout::SetMinAndMaxSize);

    mNormalTitleBarWidget = createNormalTitleBarWidget(this);

    vLayout->addWidget(mNormalTitleBarWidget);

    setLayout(vLayout);
}

QWidget* PencilDockTitleBarWidget::createNormalTitleBarWidget(QWidget* parent)
{
    QWidget* containerWidget = new QWidget(parent);

    QHBoxLayout* layout = new QHBoxLayout(parent);

    mCloseButton = new PencilDockWidgetTitleButton(parent);

    QSize iconSize = QSize(14,14);
    QSize padding = QSize(2,2);
    QIcon closeIcon("://icons/themes/playful/window/window-close-button-14.svg");

    mCloseButton->setIcon(closeIcon);
    mCloseButton->setIconSize(iconSize);
    mCloseButton->setFixedSize(iconSize + padding);

    connect(mCloseButton, &PencilDockWidgetTitleButton::clicked, this, [this] {
        emit closeButtonPressed();
    });

    mDockButton = new PencilDockWidgetTitleButton(parent);
    mDockButton->setIcon(QIcon("://icons/themes/playful/window/window-float-button-14.svg"));
    mDockButton->setIconSize(iconSize);
    mDockButton->setFixedSize(iconSize + padding);

    connect(mDockButton, &PencilDockWidgetTitleButton::clicked, this, [this] {
       emit undockButtonPressed();
    });


    mTitleLabel = new QLabel(parent);
    QFont font;
    font.setPointSize(11);
    mTitleLabel->setFont(font);
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

void PencilDockTitleBarWidget::setWindowTitle(const QString &title)
{
    mTitleLabel->setText(title);
}

QSize PencilDockTitleBarWidget::minimumSizeHint() const
{
    return QSize(16, 100);
}

void PencilDockTitleBarWidget::hideButtons(bool hide)
{
    mCloseButton->setHidden(hide);
    mDockButton->setHidden(hide);
}

void PencilDockTitleBarWidget::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);

    if (resizeEvent->size().width() < 75) {
        hideButtons(true);
    } else if (resizeEvent->size().width() >= 75) {
        hideButtons(false);
    }
}

void PencilDockTitleBarWidget::paintEvent(QPaintEvent *)
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


