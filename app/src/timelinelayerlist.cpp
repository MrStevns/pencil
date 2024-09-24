#include "timelinelayerlist.h"

#include <QApplication>
#include <QDebug>

#include "editor.h"
#include "layermanager.h"
#include "preferencemanager.h"
#include "timeline.h"

#include "timelinelayercell.h"
#include "timelinelayercelleditorwidget.h"
#include "timelinelayercellgutterwidget.h"

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

    mGutterWidget = new TimeLineLayerCellGutterWidget(width(), this);
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
        TimeLineLayerCell* cell = new TimeLineLayerCell(mTimeLine, this, mEditor, layeri, QPoint(0, layerY), width(), mLayerHeight);
        mLayerCells.insert(layeri->id(), cell);

        connect(cell->editorWidget(), &TimeLineLayerCellEditorWidget::drag, this, &TimeLineLayerList::onCellDragged);
        connect(cell->editorWidget(), &TimeLineLayerCellEditorWidget::layerVisibilityChanged, mTimeLine, &TimeLine::updateContent);
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

    mRedrawContent = false;
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

    mGutterWidget->updateWidth(event->size().width());

    updateContent();
    event->accept();

}

int TimeLineLayerList::getLayerGutterYPosition(int posY) const
{
    int layerNumber = getLayerNumber(posY + (mLayerHeight * 0.5));

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

void TimeLineLayerList::onCellDragged(const DragEvent& event, TimeLineLayerCellEditorWidget* editorWidget, int /*x*/, int y)
{
    switch (event)
    {
        case DragEvent::STARTED: {
            mGutterPositionY = getLayerGutterYPosition(y);
            mFromLayer = getLayerNumber(y);
            editorWidget->raise();
            mGutterWidget->hide();
            mGutterWidget->raise();
            emit cellDraggedY(event, y);
            break;
        }
        case DragEvent::DRAGGING: {

            // The camera layer is at the bottom and must not be moved
            if (mFromLayer <= 0) {
                break;
            }

            editorWidget->move(0, y);
            mGutterPositionY = getLayerGutterYPosition(y);

            if (editorWidget->didDetach()) {

                int dragToNumber = getDragToLayerNumber(getLayerCellY(mFromLayer), mGutterPositionY);

                if (dragToNumber != mFromLayer && mFromLayer > 0) {
                    mGutterWidget->show();
                } else {
                    mGutterWidget->hide();
                }
            }

            mGutterWidget->move(0, mGutterPositionY - mGutterWidget->rect().center().y());
            emit cellDraggedY(event, y);
            break;
        }
        case DragEvent::ENDED: {
            int fromLayerDragY = getLayerCellY(mFromLayer);
            int dragToNumber = getDragToLayerNumber(getLayerCellY(mFromLayer), mGutterPositionY);

            if (!mScrollingVertically && dragToNumber != mFromLayer && dragToNumber > -1 && mGutterWidget->isVisible())
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
            mGutterWidget->hide();
            editorWidget->move(0, fromLayerDragY);
            emit cellDraggedY(event, y);
            mGutterPositionY = -1;
            mFromLayer = -1;
            break;
        }
    }

    updateContent();
}

int TimeLineLayerList::getDragToLayerNumber(int fromLayerDragY, int gutterPositionY) const
{
    if (fromLayerDragY > gutterPositionY) {
        // If we're starting from below, adjust the drag number so we're one cell below
        return getLayerNumber(gutterPositionY + (mLayerHeight * 0.5));
    } else if (fromLayerDragY < gutterPositionY) {
        // If we're starting from above, adjust the drag number so we're one cell above
        return getLayerNumber(gutterPositionY - (mLayerHeight * 0.5));
    }

    return getLayerNumber(fromLayerDragY);
}
