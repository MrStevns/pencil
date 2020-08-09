#ifndef IMAGEPAINTER_H
#define IMAGEPAINTER_H

#include <QPainter>
#include <QImage>
#include <QPoint>
#include <QTransform>
#include <QBrush>

class ImagePainter
{
public:
    ImagePainter();

    void paintPixmap(QPainter& painter, QRect visibleArea, QTransform viewTransform, QPixmap& pixmap, QPoint topLeft);
    void paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QImage& imageToPaint, QPoint origin, bool colorize, int currentFrame, int frameIteration);
    void paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QImage& imageToPaint, QPoint origin);
    void paintTiled(QPainter &painter, QPainter& imagePainter, QRect visibleArea, QTransform viewTransform, bool useCanvasBuffer, QImage& image, QRect imageBounds, const QPixmap& pixmapToPaint, QPoint origin);

private:
    void applyColorize(QPainter& painter, QBrush brush);
};

#endif // IMAGEPAINTER_H
