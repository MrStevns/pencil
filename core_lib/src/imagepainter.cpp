#include "imagepainter.h"

ImagePainter::ImagePainter()
{

}

void ImagePainter::paint(QPainter& painter, QTransform viewTransform, QImage& imageToPaint, QPoint origin)
{
    paint(painter, viewTransform, imageToPaint, origin, false, 0, 0);
}

void ImagePainter::paint(QPainter& painter, QTransform viewTransform, QImage& imageToPaint, QPoint origin, bool colorize, int currentFrame, int frameIteration)
{
    QPixmap colorDevice = QPixmap(painter.viewport().size());
    colorDevice.fill(Qt::transparent);
    QPainter imagePainter(&colorDevice);

    QRectF croppedRect = painter.viewport();
    croppedRect = viewTransform.inverted().mapRect(croppedRect);

    painter.setTransform(viewTransform);

    imagePainter.setTransform(viewTransform);
    imagePainter.translate(origin);
    imagePainter.drawImage(QPoint(), imageToPaint, imageToPaint.rect());

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
        applyColorize(imagePainter, colorBrush);
    }

    painter.drawPixmap(croppedRect, colorDevice, colorDevice.rect());
}

void ImagePainter::applyColorize(QPainter& painter, QBrush brush)
{
    painter.setBrush(brush);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawRect(painter.viewport());
}
