#ifndef BITMAPEDITOR_H
#define BITMAPEDITOR_H

#include <QImage>
#include <QRect>
#include <QPainter>
#include <QPainterPath>
#include <QHash>
#include <QtMath>

#include "keyframeeditor.h"

class TiledBuffer;

class BitmapEditor: public KeyFrameEditor
{
public:
    BitmapEditor();
    BitmapEditor(const BitmapEditor& editor);
    BitmapEditor(const QRect& rectangle, const QColor& color);
    BitmapEditor(const QPoint& topLeft, const QImage& image);
    BitmapEditor(const QPoint& topLeft, const QString& path);
    ~BitmapEditor() override;

    QRect bounds() const { return mBounds; }

    void moveTopLeft(QPoint& point) { mBounds.moveTopLeft(point); }
    void moveTopLeft(QPointF& point) { mBounds.moveTopLeft(point.toPoint()); }

    void transform(QRect newBoundaries, bool smoothTransform);
    BitmapEditor transformed(const QRect& newBoundaries, const QTransform& transform, bool smoothTransform) const;
    // const QImage& transformed(const QImage& image, const QRect& selection, const QTransform& transform, bool smoothTransform);
    // const QImage& transformed(const QImage& image, const QRect& newBoundaries, bool smoothTransform);

    bool updateBounds(const QRect& newBoundaries);
    bool extend(QRect rectangle);

    void setCompositionModeBounds(const QRect& sourceBounds, bool isSourceMinBounds, QPainter::CompositionMode cm);

    bool contains(QPoint P) { return mBounds.contains(P); }

    QPoint topLeft() { autoCrop(); return mBounds.topLeft(); }
    QPoint topRight() { autoCrop(); return mBounds.topRight(); }
    QPoint bottomLeft() { autoCrop(); return mBounds.bottomLeft(); }
    QPoint bottomRight() { autoCrop(); return mBounds.bottomRight(); }
    int left() { autoCrop(); return mBounds.left(); }
    int right() { autoCrop(); return mBounds.right(); }
    int top() { autoCrop(); return mBounds.top(); }
    int bottom() { autoCrop(); return mBounds.bottom(); }
    int width() { autoCrop(); return mBounds.width(); }
    int height() { autoCrop(); return mBounds.height(); }
    QSize size() { autoCrop(); return mBounds.size(); }

    bool isMinimallyBounded() const { return mMinBound; }
    void enableAutoCrop(bool b) { mEnableAutoCrop = b; }
    void setOpacity(qreal opacity) { mOpacity = opacity; }
    qreal getOpacity() const { return mOpacity; }

    QRgb constScanLine(int x, int y) const;
    void scanLine(int x, int y, QRgb color);
    QRgb pixel(const QPoint& p) const;
    void setPixel(const QPoint& p, QRgb color);

    BitmapEditor copyArea(const QRect& rect) const;

    void autoCrop();

    void loadFile();
    void unloadFile();
    bool isLoaded();

    QImage* image();
    const QImage* constImage() const;

    void paintImage(QPainter& painter);
    void paintImage(QPainter& painter, QImage& image, QRect sourceRect, QRect destRect);

    void drawLine(QPointF P1, QPointF P2, QPen pen, QPainter::CompositionMode cm, bool antialiasing);
    void drawRect(QRectF rectangle, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing);
    void drawEllipse(QRectF rectangle, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing);
    void drawPath(QPainterPath path, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing);

    void clear();
    void clear(const QRect& rectangle);
    void paste(const BitmapEditor& bitmapEditor, QPainter::CompositionMode mode = QPainter::CompositionMode::CompositionMode_SourceOver);
    void paste(const TiledBuffer* tiledBuffer, QPainter::CompositionMode cm);

    quint64 memoryUsage() const override;

private:
    QImage mImage;
    QRect mBounds{0, 0, 0, 0};

    /** @see isMinimallyBounded() */
    bool mMinBound = true;
    bool mEnableAutoCrop = false;
    qreal mOpacity = 1.0;
};

#endif // BITMAPEDITOR_H
