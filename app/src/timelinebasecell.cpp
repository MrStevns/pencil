#include "timelinebasecell.h"

#include <QMouseEvent>

#include "editor.h"
#include "layer.h"
#include "preferencemanager.h"
#include "timeline.h"

TimeLineBaseCell::TimeLineBaseCell(TimeLine* parent,
                                    Editor* editor,
                                    const QPoint& origin,
                                    int width,
                                    int height)
{
    mTimeLine = parent;
    mEditor = editor;
    mPrefs = mEditor->preference();
    mGlobalBounds = QRect(origin, QSize(width,height));
}

TimeLineBaseCell::~TimeLineBaseCell()
{
}

bool TimeLineBaseCell::contains(const QPoint& point) const
{
    return mGlobalBounds.contains(point);
}

void TimeLineBaseCell::move(int x, int y)
{
    mGlobalBounds.translate(x, y);
}

QPoint TimeLineBaseCell::localPosition(QMouseEvent* event) const
{
    return QPoint(mGlobalBounds.x() + event->pos().x(), mGlobalBounds.y() + event->pos().y());
}
