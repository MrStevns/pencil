#include "timelinelayerlist.h"

#include <QApplication>
#include <QDebug>

#include "editor.h"
#include "layermanager.h"
#include "preferencemanager.h"
#include "timeline.h"

#include "timelinelayercell.h"

TimeLineLayerList::TimeLineLayerList(TimeLine* parent, Editor* editor) : QWidget(parent)
{
    mTimeLine = parent;
    mEditor = editor;
    mPrefs = editor->preference();

    setMinimumSize(500, 4 * mLayerHeight);
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setMouseTracking(true);

    connect(mPrefs, &PreferenceManager::optionChanged, this, &TimeLineLayerList::loadSetting);
}

TimeLineLayerList::~TimeLineLayerList()
{
}

void TimeLineLayerList::loadSetting(SETTING setting)
{
    updateContent();
}

void TimeLineLayerList::loadLayerCells()
{
    if (!mLayerCells.isEmpty()) {
        for (TimeLineLayerCell* cell : qAsConst(mLayerCells)) {
            // The cells might still be in use when this happens, as such
            // make sure they are set for deletion later.
            cell->deleteLater();
        }
        mLayerCells.clear();
    }

    for (int i = 0; i < mEditor->layers()->count(); i++)
    {
        Layer* layeri = mEditor->layers()->getLayer(i);
        const int layerY = getLayerCellY(i);
        TimeLineLayerCell* cell = new TimeLineLayerCell(mTimeLine, mEditor, layeri, QPoint(0, layerY), width() - 1, mLayerHeight);
        mLayerCells.insert(layeri->id(), cell);

        connect(cell, &TimeLineLayerCell::drag, this, &TimeLineLayerList::onCellDragged);
    }

    setMinimumHeight(mEditor->layers()->count() * mLayerHeight);
}

int TimeLineLayerList::getLayerNumber(int y) const
{
    int totalLayerCount = mEditor->layers()->count();
    int layerNumber = (totalLayerCount - 1) - (y / mLayerHeight);
    return layerNumber;
}

int TimeLineLayerList::getLayerCellY(int layerNumber) const
{
    return (mEditor->layers()->count() - 1 - layerNumber) * mLayerHeight;
}

void TimeLineLayerList::updateContent()
{
    mRedrawContent = true;
    update();
}

void TimeLineLayerList::drawContent()
{
    QPainter painter(&mPixmapCache);

    // grey background of the view
    const QPalette palette = QApplication::palette();
    painter.setPen(Qt::NoPen);
    painter.setBrush(palette.color(QPalette::Base));
    painter.drawRect(QRect(0, 0, width(), height()));

    if (mLayerCells.isEmpty()) { return; }

    const QMap<int, TimeLineLayerCell*> layerCells = mLayerCells;
    for (const TimeLineLayerCell* cell : layerCells) {
        if (layerCells.isDetached()) { return; }
        if (mEditor->layers()->selectedLayerId() != cell->layer()->id()) {
            cell->paint(painter, palette);
        }
    }

    int selectedLayerId = mEditor->layers()->selectedLayerId();

    const auto cell = getCell(selectedLayerId);

    if (cell) {
        cell->paint(painter, palette);

        if (cell->didDetach()) {
            paintLayerGutter(painter, palette);
        }
    }
    mRedrawContent = false;
}

void TimeLineLayerList::paintLayerGutter(QPainter& painter, const QPalette& palette) const
{
    painter.setPen(palette.color(QPalette::Mid));
    painter.drawRect(0, mGutterPositionY, width(), 2);
}

void TimeLineLayerList::paintEvent(QPaintEvent*)
{
    QPainter painter(this);

    if (mPixmapCache.isNull() || mRedrawContent)
    {
        drawContent();
    }
    if (!mPixmapCache.isNull())
    {
        painter.drawPixmap(QPoint(0, 0), mPixmapCache);
    }
}

void TimeLineLayerList::resizeEvent(QResizeEvent* event)
{
    if (event->size() != mPixmapCache.size()) {
        mPixmapCache = QPixmap(event->size() * devicePixelRatioF());
        mPixmapCache.fill(Qt::transparent);
        mPixmapCache.setDevicePixelRatio(this->devicePixelRatioF());
    }
    setMinimumHeight(mEditor->layers()->count() * mLayerHeight);;

    const QMap<int, TimeLineLayerCell*> layerCells = mLayerCells;
    for (TimeLineLayerCell* cell : layerCells) {
        if (layerCells.isDetached()) { return; }
        cell->setSize(QSize(event->size().width(), mLayerHeight));
    }

    updateContent();
    event->accept();

}

void TimeLineLayerList::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);

    mPrimaryButton = event->button();

    if (!mLayerCells.isEmpty()) {
        const QMap<int, TimeLineLayerCell*> layerCells = mLayerCells;
        for (TimeLineLayerCell* cell : layerCells) {
            if (layerCells.isDetached()) { return; }
            if (!cell->contains(event->pos())) { continue; }
            cell->mousePressEvent(event);
        }
    }
}

void TimeLineLayerList::mouseMoveEvent(QMouseEvent* event)
{
    QWidget::mouseMoveEvent(event);
    if (!mLayerCells.isEmpty()) {
        const QMap<int, TimeLineLayerCell*> layerCells = mLayerCells;
        for (TimeLineLayerCell* cell : layerCells) {
            if (layerCells.isDetached()) { return; }
            if (cell->didDetach()) {
                cell->mouseMoveEvent(event);
                break;
            } else if (cell->contains(event->pos())) {
                cell->mouseMoveEvent(event);
            }
        }
    }
}

void TimeLineLayerList::mouseReleaseEvent(QMouseEvent* event)
{
    QWidget::mouseReleaseEvent(event);
    if (event->button() != mPrimaryButton) return;

    if (!mLayerCells.isEmpty()) {
        const QMap<int, TimeLineLayerCell*> layerCells = mLayerCells;
        for (TimeLineLayerCell* cell : layerCells) {
            if (layerCells.isDetached()) { return; }
            if (cell->didDetach()) {
                cell->mouseReleaseEvent(event);
                break;
            } else if (cell->contains(event->pos())) {
                cell->mouseReleaseEvent(event);
            }
        }
    }

    mPrimaryButton = Qt::NoButton;
    mTimeLine->scrubbing = false;
}

void TimeLineLayerList::mouseDoubleClickEvent(QMouseEvent* event)
{
    QWidget::mouseDoubleClickEvent(event);

    const QMap<int, TimeLineLayerCell*> layerCells = mLayerCells;
    for (TimeLineLayerCell* cell : layerCells) {
        if (layerCells.isDetached()) { return; }
        if (!cell->contains(event->pos())) {
            continue;
        }
        cell->mouseDoubleClickEvent(event);
    }
}

int TimeLineLayerList::getLayerGutterYPosition(int posY) const
{
    int layerNumber = getLayerNumber(posY - (mLayerHeight * 0.5));
    if(posY > getLayerCellY(layerNumber)) {
        layerNumber--;
    }
    int maxLayerNum = mEditor->layers()->count() - 1;
    if (layerNumber > maxLayerNum) {
        layerNumber = maxLayerNum;
    } else if (layerNumber < 0) {
        layerNumber = 0;
    }
    return getLayerCellY(layerNumber);
}

void TimeLineLayerList::vScrollChange(int x)
{
    Q_UNUSED(x)
    mScrollingVertically = true;
    updateContent();
}

void TimeLineLayerList::onScrollingVerticallyStopped()
{
    mScrollingVertically = false;
}

void TimeLineLayerList::onCellDragged(const DragEvent& event, const TimeLineLayerCell* /*cell*/, int x, int y)
{
    switch (event)
    {
        case DragEvent::STARTED: {
            mGutterPositionY = getLayerGutterYPosition(y);
            mFromLayer = getLayerNumber(y);
            emit cellDraggedY(event, y);
            break;
        }
            // mToLayer = mFromLayer;
        case DragEvent::DRAGGING: {
            mGutterPositionY = getLayerGutterYPosition(y);
            emit cellDraggedY(event, y);
            break;
        }
        case DragEvent::ENDED: {
            int dragToNumber = getLayerNumber(mGutterPositionY - (mLayerHeight * 0.5));
            if (!mScrollingVertically && dragToNumber != mFromLayer && dragToNumber > -1)
            {
                if (dragToNumber < mEditor->layers()->count())
                {
                    // Bubble the from layer up or down to the to layer
                    if (dragToNumber < mFromLayer) // bubble up
                    {
                        for (int i = mFromLayer - 1; i >= dragToNumber; i--)
                            mEditor->layers()->swapLayers(i, i + 1);
                    }
                    else // bubble down
                    {
                        for (int i = mFromLayer + 1; i <= dragToNumber; i++)
                            mEditor->layers()->swapLayers(i, i - 1);
                    }
                }
            }
            emit cellDraggedY(event, y);
            mGutterPositionY = -1;
            mFromLayer = -1;
            break;
        }
    }

    updateContent();
}

TimeLineLayerCell* TimeLineLayerList::getCell(int id) const
{
    if (mLayerCells.isDetached()) {
        return nullptr;
    }

    return mLayerCells.find(id).value();
}
