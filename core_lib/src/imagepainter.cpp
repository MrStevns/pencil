#include "imagepainter.h"

ImagePainter::ImagePainter()
{

}

void ImagePainter::paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QImage& imageToPaint, QPoint origin)
{
    paint(painter, visibleArea, viewTransform, imageToPaint, origin, false, 0, 0);
}

void ImagePainter::paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QImage& imageToPaint, QPoint origin, bool colorize, int currentFrame, int frameIteration)
{
    QPixmap colorDevice = QPixmap(visibleArea.size());
    colorDevice.fill(Qt::transparent);
    QPainter areaPainter(&colorDevice);

    areaPainter.save();
    areaPainter.setTransform(viewTransform);
    areaPainter.translate(origin);
    areaPainter.drawImage(QPoint(), imageToPaint);
    areaPainter.restore();

    if (colorize) {
        QBrush colorBrush = Qt::NoBrush; // No color for the current frame

        if (frameIteration < currentFrame)
        {
            colorBrush = QBrush(Qt::red);
        }
        else if (frameIteration > currentFrame)
        {
            colorBrush = QBrush(Qt::blue);
        }
        applyColorize(areaPainter, colorBrush);
    }

    QRectF croppedRect = visibleArea;
    croppedRect = viewTransform.inverted().mapRect(croppedRect);

    painter.drawPixmap(QPoint(), colorDevice);
}

void ImagePainter::paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QPixmap& pixmap, QPoint origin)
{
    QPixmap colorDevice = QPixmap(painter.viewport().size());
    colorDevice.fill(Qt::transparent);
    QPainter areaPainter(&colorDevice);

    areaPainter.setTransform(viewTransform);
    areaPainter.translate(origin);
    areaPainter.drawPixmap(QPoint(), pixmap, pixmap.rect());

    QRectF croppedRect = visibleArea;
    croppedRect = viewTransform.inverted().mapRect(croppedRect);

    painter.setTransform(viewTransform);
    painter.drawPixmap(croppedRect, colorDevice, colorDevice.rect());
}

void ImagePainter::applyColorize(QPainter& painter, QBrush brush)
{
    painter.setBrush(brush);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawRect(painter.viewport());
}
