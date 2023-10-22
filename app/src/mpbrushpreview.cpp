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

    connect(mMypaintHandler, &MPHandler::tileAdded, this, &MPBrushPreview::loadTile);
    connect(mMypaintHandler, &MPHandler::tileUpdated, this, &MPBrushPreview::updateTile);
    connect(mMypaintHandler, &MPHandler::tileCleared, this, &MPBrushPreview::tileRemoved);
    connect(mMypaintHandler, &MPHandler::surfaceCleared, this, &MPBrushPreview::surfaceTilesRemoved);

    perfTimer = new QElapsedTimer();
    perfTimer->start();

    mSurfaceBackground = QImage(size(),QImage::Format_ARGB32_Premultiplied);
    updatePaintSurface(size());
}

MPBrushPreview::~MPBrushPreview()
{
    perfTimer->invalidate();
    delete perfTimer;
    delete mMypaintHandler;
}

void MPBrushPreview::updatePreview(const QByteArray &content, const QColor& brushColor)
{
    mMypaintHandler->loadBrush(content);
    mMypaintHandler->setBrushColor(brushColor);
    mMypaintHandler->clearSurface();

    paintTestBackground();
    mMypaintHandler->setSurfaceSize(this->size());

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

void MPBrushPreview::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (mSurfaceTiles.isEmpty()) { return; }
    const QHash<TileIndex, MPTile*> tiles = mSurfaceTiles;

    painter.drawImage(QPoint(), mSurfaceBackground);
    painter.setCompositionMode(QPainter::CompositionMode_Source);

    for (MPTile* item : tiles) {
        const QImage& img = item->image();

        const QRect& tileRect = QRect(item->pos(), img.size());

        painter.drawImage(tileRect, img, img.rect());
    }
    painter.end();

    event->accept();
}

void MPBrushPreview::updateTile(MPSurface *surface, MPTile* tile)
{
    Q_UNUSED(surface)
    update(QRect(tile->pos(), tile->boundingRect().size()));
}

void MPBrushPreview::tileRemoved(MPSurface* surface, QRect tileRect)
{
    Q_UNUSED(surface)
    mSurfaceTiles.remove(surface->getTileIndex(tileRect.x(), tileRect.y()));
    update(tileRect);
}

void MPBrushPreview::loadTile(MPSurface* surface, MPTile* tile)
{
    Q_UNUSED(surface)
    mMypaintHandler->loadTile(mSurfaceBackground, QPoint(), tile);

    mSurfaceTiles.insert(surface->getTileIndex(tile->pos()), tile);
    update(QRect(tile->pos(), tile->boundingRect().size()));
}

void MPBrushPreview::surfaceTilesRemoved(MPSurface* surface)
{
    Q_UNUSED(surface)
    mSurfaceTiles.clear();
    update();
}

void MPBrushPreview::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    mSurfaceBackground = QImage(event->size(),QImage::Format_ARGB32_Premultiplied);
    updatePaintSurface(event->size());
    drawStroke();
}

void MPBrushPreview::updatePaintSurface(QSize size)
{
    mSurfaceBackground.fill(Qt::gray);
    paintTestBackground();
    mMypaintHandler->setSurfaceSize(size);
}

void MPBrushPreview::paintTestBackground()
{
    QPainter painter(&mSurfaceBackground);

    int middleX = qFloor(static_cast<qreal>(this->width()) / 2.0);
    painter.fillRect(QRect(middleX, 0, middleX, this->height()), QBrush(QPixmap(":background/checkerboard.png")));
}




