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

    void paint(QPainter& painter, QTransform viewTransform, QImage& imageToPaint, QPoint origin, bool colorize, int currentFrame, int frameIteration);
    void paint(QPainter& painter, QTransform viewTransform, QImage& imageToPaint, QPoint origin);

private:
    void applyColorize(QPainter& painter, QBrush brush);
};

#endif // IMAGEPAINTER_H
