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

    areaPainter.setTransform(viewTransform);
    areaPainter.translate(origin);
    areaPainter.drawImage(QPoint(), imageToPaint, imageToPaint.rect());

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

    painter.setTransform(viewTransform);
    painter.drawPixmap(croppedRect, colorDevice, colorDevice.rect());
}

void ImagePainter::paintPixmap(QPainter& painter, QRect visibleArea, QTransform viewTransform, QPixmap& pixmap, QPoint origin)
{
    QPixmap colorDevice = QPixmap(visibleArea.size());
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

void ImagePainter::paintTiled(QPainter &painter, QPainter& imagePainter, QRect visibleArea, QTransform viewTransform, bool useCanvasBuffer, QImage& image, QRect imageBounds, const QPixmap &pixmapToPaint, QPoint origin)
{
    QPixmap pix = pixmapToPaint;

    QRect tileRect = QRect(QPoint(origin), QSize(pixmapToPaint.size()));

     // Tools that require continous clearing should not get in here
     // eg. polyline because it's already clearing its surface per dab it creates

    // Fixes not drawing on the same tile, that could otherwise cause small artifacts.
    if (useCanvasBuffer) {
        if (imageBounds.contains(tileRect)) {

            imagePainter.save();
            imagePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            imagePainter.drawPixmap(tileRect, pix, pix.rect());
            imagePainter.restore();
        } else {
            // Fixes polyline being rendered on top of itself because the image has been painted already
            painter.save();
            painter.translate(-visibleArea.width()/2, -visibleArea.height()/2);
            painter.setTransform(viewTransform);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
            painter.drawPixmap(tileRect, pix, pix.rect());
            painter.restore();
        }
    } else {
        imagePainter.save();
        imagePainter.setCompositionMode(QPainter::CompositionMode_Source);
        imagePainter.translate(-imageBounds.topLeft());
        imagePainter.drawPixmap(tileRect, pix, pix.rect());
        imagePainter.restore();

        if (!imageBounds.contains(tileRect)) {
            painter.save();
            painter.translate(-painter.viewport().width()/2, -painter.viewport().height()/2);
            painter.setTransform(viewTransform);
            painter.drawPixmap(tileRect, pix, pix.rect());
            painter.restore();
        }
    }
}

void ImagePainter::applyColorize(QPainter& painter, QBrush brush)
{
    painter.setBrush(brush);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawRect(painter.viewport());
}
