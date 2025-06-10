#include "canvascursorsizingtool.h"

#include "pointerevent.h"

#include <QDebug>
#include <QPainter>

CanvasCursorSizingTool::CanvasCursorSizingTool(QObject* parent) : QObject(parent)
{

}

CanvasCursorSizingTool::~CanvasCursorSizingTool()
{
}

void CanvasCursorSizingTool::setup(Qt::KeyboardModifiers modifiers, int toolSetting, bool showCross)
{
    mKbModifiers = modifiers;
    mToolSetting = toolSetting;
    mShowCross = showCross;
}

bool CanvasCursorSizingTool::pointerEvent(PointerEvent* event)
{
    // updateCursor();
    if (event->eventType() == PointerEvent::Press) {
        // if (mQuickSizingEnabled) {
            return startAdjusting(event);
        // }
    } else if (event->eventType() == PointerEvent::Move) {
        if (event->buttons() & Qt::LeftButton && mIsAdjusting) {
            adjust(event);
            return true;
        }
    } else if (event->eventType() == PointerEvent::Release) {
        if (mIsAdjusting) {
            stopAdjusting(event);
            return true;
        }
    }

    return false;
}

void CanvasCursorSizingTool::adjust(PointerEvent* event)
{
    if (mKbModifiers != event->modifiers())
    {
        return;
    }

    const qreal newValue = QLineF(mAdjustPoint, event->canvasPos()).length() * 2.0;

    emit sizeChanged(newValue);

    updateCursor();
}

bool CanvasCursorSizingTool::startAdjusting(PointerEvent* event)
{
    if (mKbModifiers != event->modifiers())
    {
        return false;
    }

    const qreal rad = mWidth * 0.5;

    QPointF direction(-1, -1); // 45 deg back
    QLineF line(event->canvasPos(), event->canvasPos() + direction);
    line.setLength(rad);

    mAdjustPoint = line.p2(); // Adjusted point on circle boundary

    mIsAdjusting = true;
    updateCursor();
    return true;
}

void CanvasCursorSizingTool::stopAdjusting(PointerEvent* event)
{
    mIsAdjusting = false;
    mAdjustPoint = QPointF();

    updateCursor();
}

void CanvasCursorSizingTool::updateCursor()
{
    const qreal cursorRad = mWidth * 0.5;
    const QPointF& circleTopLeft = QPointF(mAdjustPoint.x() - cursorRad, mAdjustPoint.y() - cursorRad);

    CanvasCursorPainterOptions options;
    const qreal cursorWidth = mWidth;
    options.circleRect = QRectF(circleTopLeft, QSizeF(cursorWidth, cursorWidth));

    options.showCursor = true;
    options.showCross = mShowCross;
    options.isAdjusting = mIsAdjusting;

    mCanvasCursorPainter.preparePainter(options);


    const QRect& dirtyRect = mCanvasCursorPainter.dirtyRect();
    const QRect& updateRect = options.circleRect.toAlignedRect();

    emit blitRectChanged(updateRect.united(dirtyRect).adjusted(-2, -2, 2, 2));
}

void CanvasCursorSizingTool::paint(QPainter& painter, const QRect &blitRect)
{
    mCanvasCursorPainter.paint(painter, blitRect);
}
