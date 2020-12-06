#include "mpbrushpreview.h"

#include <QResizeEvent>
#include <QPainter>
#include <QDebug>
#include <QPixmapCache>
#include <QElapsedTimer>
#include <QtMath>
#include <QImage>

#include "mphandler.h"
#include "mpsurface.h"

MPBrushPreview::MPBrushPreview(QWidget* parent) : QWidget(parent)
{
    mMypaintHandler = new MPHandler();

    connect(mMypaintHandler, &MPHandler::tileAdded, this, &MPBrushPreview::updateTile);
    connect(mMypaintHandler, &MPHandler::tileUpdated, this, &MPBrushPreview::updateTile);

    update();

    perfTimer = new QElapsedTimer();
    perfTimer->start();
}

MPBrushPreview::~MPBrushPreview()
{
    perfTimer->invalidate();
}

void MPBrushPreview::applyBackground()
{
    mSurfaceBackground = new QImage(this->width()/2,this->height(),QImage::Format_ARGB32_Premultiplied);
    mSurfaceBackground->fill(Qt::white);

    mMypaintHandler->loadImage(*mSurfaceBackground, QPoint(0,0));
}

void MPBrushPreview::updatePreview(const QByteArray &content)
{
    mMypaintHandler->loadBrush(content);
    mMypaintHandler->clearSurface();

    applyBackground();
    drawStroke();

    update();
}

void MPBrushPreview::drawStroke() const
{
    mMypaintHandler->startStroke();

    qreal delta = 0.05;

    // Basically just spacers, draw stroke 10% from left and right
    qreal widthMax = this->width()*0.90;
    qreal widthMin = this->width()*0.10;

    qreal pressure = 0.0;
    qreal x = 0.0;

    widthMax = widthMax-widthMin;
    qreal halfWidth = static_cast<qreal>(widthMax/2);
    qreal halfHeight = static_cast<qreal>(this->height()/2);

    // Stepper, will adjust to a variable width, to avoid drawing too many steps
    // when it's not neccesary.
    qreal step = widthMax/30;

    // Sine wave parameters
    qreal freq = widthMax/(2 * M_PI);
    qreal amplitude = 1;
    qreal waveHeight = 30.0;

    perfTimer->start();
    while (x < widthMax+step) {

        if (x < halfWidth) {
            // build pressure from 0 to 1
            pressure = (1.0/(halfWidth))*x;
            if (pressure < 0) {
                pressure = 0;
            }
        } else {
            // We've reached midway, decrease from 1 to 0
            pressure = 2.0-((1.0/halfWidth)*x);
        }

        qreal y = halfHeight + amplitude * sin(x/freq);
        mMypaintHandler->strokeTo(static_cast<float>((widthMin)+x),
                                  static_cast<float>(halfHeight - waveHeight * (sin(y))),
                                  static_cast<float>(pressure), 1,1, delta);

        const qint64 elapsed = perfTimer->elapsed();

        // Increase steps to draw less dabs that are slow to create
        if (elapsed > 300) {
            step += 10;
        }
        // Dabs are too slow to draw, break out of the loop.
        if (elapsed > 700) {
            qDebug() << "stopped drawing, too slow";
            break;
        }
        x += step;
    }

    mMypaintHandler->endStroke();

    perfTimer->restart();
}

void MPBrushPreview::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), QBrush(QPixmap(":background/checkerboard.png")));

    if (mMypaintSurface == nullptr) { return; }
    QHash<QString, MPTile*> tiles = mMypaintSurface->getTiles();

    for (MPTile* item : tiles) {
        const QPixmap& pix = item->pixmap();

        QRect tileRect = QRect(item->pos(), item->pixmap().size());

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawPixmap(tileRect, pix, pix.rect());
    }
    painter.end();
}

void MPBrushPreview::updateTile(MPSurface *surface, MPTile*)
{
    mMypaintSurface = surface;
    update();
}

void MPBrushPreview::resizeEvent(QResizeEvent* event)
{
    mMypaintHandler->setSurfaceSize(event->size());
    mMypaintHandler->clearSurface();
    applyBackground();
    drawStroke();
    QWidget::resizeEvent(event);
}




