#ifndef CANVASCURSORSIZINGTOOL_H
#define CANVASCURSORSIZINGTOOL_H

#include <QObject>
#include <QPainterPath>

#include "canvascursorpainter.h"

class PointerEvent;

class CanvasCursorSizingTool : public QObject
{
    Q_OBJECT
public:
    CanvasCursorSizingTool(QObject* parent = nullptr);
    ~CanvasCursorSizingTool();

    /// Setup which modifier and tool setting this tool should
    /// identity with
    void setup(Qt::KeyboardModifiers modifiers, int toolSetting, bool showCross);

    void setWidth(qreal width) { mWidth = width; }
    bool pointerEvent(PointerEvent* event);
    bool startAdjusting(PointerEvent* event);
    void stopAdjusting(PointerEvent* event);
    void adjust(PointerEvent* event);
    void paint(QPainter& painter, const QRect& blitRect);

    void updateCursor(const QPointF& point);

signals:
    void sizeChanged(qreal newSize);
    void blitRectChanged(const QRect&);

private:
    Qt::KeyboardModifiers mKbModifiers;
    int mToolSetting = 0;

    CanvasCursorPainter mCanvasCursorPainter;

    bool mShowCross = false;
    bool mIsAdjusting = false;
    qreal mWidth = 0.;
    QPointF mAdjustPoint = QPointF();
};

#endif // CANVASCURSORSIZINGTOOL_H
