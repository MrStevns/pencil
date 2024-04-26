#include "mappingdistributionwidget.h"

#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QApplication>

#include <QDebug>

#include "mathutils.h"
#include "brushsetting.h"

GridPainter::GridPainter()
{
}

void GridPainter::paint(QPainter& painter, int gridSpacingHorizontal, int gridSpacingVertical)
{
    int gridSizeW = gridSpacingHorizontal;
    int gridSizeH = gridSpacingVertical;

    QRect rect = painter.viewport();

    int left = rect.left();
    int right = rect.right();
    int top = rect.top();
    int bottom = rect.bottom();

    const QPalette palette = QApplication::palette();
    QPen pen(palette.color(QPalette::Highlight));
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setWorldMatrixEnabled(true);
    painter.setBrush(Qt::NoBrush);

    painter.drawRect(rect);

    QColor penColor = pen.color();

    // Slight adjustment in alpha to make the background less noticeable
    pen.setColor(QColor(penColor.red(), penColor.green(), penColor.blue(), penColor.alpha()-150));
    painter.setPen(pen);

    int numberOfLinesX = static_cast<int>(floor(right/gridSizeW));
    int numberOfLinesY = static_cast<int>(floor(bottom/gridSizeH));

    // draw vertical gridlines
    int count = 0;
    QVector<QLineF> lines;
    for (int x = left; x < right; x += gridSizeW) {

        // Prevents the last line from being shown when it's very close to the border
        if (count < numberOfLinesX) {
            lines << QLineF(x, top, x, bottom);
        }
        count++;
    }

    // draw horizontal gridlines
    count = 0;
    for (int y = top; y < bottom; y += gridSizeH) {

        // Prevents the last line from being shown when it's very close to the border
        if (count < numberOfLinesY) {
            lines << QLineF(left, y, right, y);
        }

        count++;
    }

    painter.drawLines(lines);
}

MappingDistributionWidget::MappingDistributionWidget(qreal min, qreal max, QVector<QPointF> points, QWidget *parent)
    : QWidget(parent), mPoints(points)
{
    mPointUniformSize = 3;
    mPointHitSize = 16;

    mActivePoint = -1;
    m_penWidth = 1;
    mMinX = min;
    mMaxX = max;
    mMinY = min;
    mMaxY = max;

    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setMouseTracking(true);
    setTabletTracking(true);

    adjustedRect = this->rect().adjusted(mPointUniformSize,mPointUniformSize-2,-mPointUniformSize+2,-mPointUniformSize+2);
    mGridPainter = GridPainter();

    mMappedPoints = mapPointsToWidget();
}

QVector<QPointF> MappingDistributionWidget::mapPointsToWidget()
{
    if (!mMappedPoints.isEmpty()) {
        mMappedPoints.clear();
    }
    for (QPointF point : mPoints) {
        qreal pointY = point.y();
        qreal pointX = point.x();

        mMinY = qMin(pointY, mMinY);
        mMinX = qMin(pointX, mMinX);
        mMaxY = qMax(pointY, mMaxY);
        mMaxX = qMax(pointX, mMaxX);
    }

    for (QPointF point : mPoints) {

        qreal mappedX = MathUtils::linearMap(point.x(), mMinX, mMaxX, adjustedRect.left(), adjustedRect.right());
        qreal mappedY = MathUtils::linearMap(point.y(), mMinY, mMaxY, adjustedRect.bottom(), adjustedRect.top());

        mMappedPoints << QPointF(mappedX, mappedY);
    }

//    qDebug() << "minX is : " << mMinX << " minY is: " << mMinY;
//    qDebug() << "maxX is : " << mMaxX << " maxY is: " << mMaxY;

    return mMappedPoints;
}

void MappingDistributionWidget::resetMapping()
{
    if (!mMappedPoints.isEmpty()) {
        mMappedPoints.clear();
        initializePoints();
    }

    mPoints = mapPointsFromWidget();
    emit pointsUpdatedFromMappingWidget(mPoints);

    update();
}

QVector<QPointF> MappingDistributionWidget::mapPointsFromWidget()
{
    QVector<QPointF> mapToOrigPoints;
    for (QPointF point : mMappedPoints) {
        qreal mappedX = MathUtils::linearMap(point.x(), adjustedRect.left(), adjustedRect.right(), mMinX, mMaxX);
        qreal mappedY = MathUtils::linearMap(point.y(), adjustedRect.bottom(), adjustedRect.top(), mMinY, mMaxY);

        mapToOrigPoints << QPointF(mappedX, mappedY);
    }
    return mapToOrigPoints;
}

void MappingDistributionWidget::updateInputs(QVector<QPointF> points, qreal min, qreal max)
{
    mPoints = points;
    mMinX = min;
    mMinY = min;
    mMaxX = max;
    mMaxY = max;

    mMappedPoints = mapPointsToWidget();
    update();
}

void MappingDistributionWidget::resizeEvent(QResizeEvent *)
{
    adjustedRect = this->rect().adjusted(mPointUniformSize,mPointUniformSize-2,-mPointUniformSize+2,-mPointUniformSize+2);
    mMappedPoints = mapPointsToWidget();
}

void MappingDistributionWidget::paintEvent(QPaintEvent*)
{
    if (mMappedPoints.isEmpty())
        initializePoints();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setViewport(adjustedRect);

    mGridPainter.paint(painter, adjustedRect.width()/4,adjustedRect.height()/4);

    QVector<QPointF> mappedPoints = mMappedPoints;

    mappedPoints << QPoint(mMappedPoints.last().x(),adjustedRect.height());
    mappedPoints << QPoint(adjustedRect.left(),adjustedRect.height());
    mappedPoints << mappedPoints[0];

    const QPalette palette = QApplication::palette();
    painter.setBrush(palette.color(QPalette::Highlight));

    painter.drawPolygon(mappedPoints);

    for (int i=0; i < mMappedPoints.size(); ++i) {
        QPointF pos = mMappedPoints.at(i);

        if (i == mActivePoint) {
            painter.setPen(palette.color(QPalette::HighlightedText));
            painter.setBrush(palette.color(QPalette::Highlight));
        } else {
            painter.setPen(palette.color(QPalette::AlternateBase));
            painter.setBrush(palette.color(QPalette::Base));
        }
        painter.drawText(QPointF(pos.x()+20,pos.y()), QString::number(i));
        painter.setBrush(palette.color(QPalette::Button));
        painter.drawRect(QRectF(pos.x() - mPointUniformSize,
                                pos.y() - mPointUniformSize,
                                mPointUniformSize*2, mPointUniformSize*2));
    }
}


/// Creates three initial points
/// [y]
/// |          x
/// |
/// |     x
/// |
/// |x
/// |---------- [x]
void MappingDistributionWidget::initializePoints()
{
    mPoints.clear();
    QPointF center(adjustedRect.width() / 2, adjustedRect.height() / 2);
    QVector<QPointF> initPoints = { QPointF(adjustedRect.left(),adjustedRect.bottom()),
                                   center,
                                    QPointF(adjustedRect.right(),adjustedRect.top())
                                  };

    for (QPointF point : initPoints) {
        mMappedPoints << point;
    }

    mPoints = mapPointsFromWidget();
    emit pointsUpdatedFromMappingWidget(mPoints);
}

void MappingDistributionWidget::mousePressEvent(QMouseEvent *e)
{
    mActivePoint = -1;
    qreal distance = -1;

    for (int i=0; i<mMappedPoints.size(); ++i) {
        qreal d = QLineF(e->pos(), mMappedPoints.at(i)).length();
        if ((distance < 0 && d < mPointHitSize) || d < distance) {
            distance = d;
            mActivePoint = i;
            break;
        }
    }

    if (mActivePoint == -1) {

        // Ensure not adding more points than allowed
        if (mMaxPoints > 0) {
            if (mMappedPoints.size() >= mMaxPoints) {
                return;
            }
        }

        for (int i=0; i < mMappedPoints.size(); ++i) {
            if (i+1 < mMappedPoints.size() && i >= 0) {
                QPointF nextPoint = mMappedPoints[i+1];
                QPointF prevPoint = mMappedPoints[i];

                if ((e->pos().x() <= nextPoint.x() && e->pos().x() >= prevPoint.x())) {
                    const int activePoint = i+1;
                    mMappedPoints.insert(activePoint, e->pos());
                    mActivePoint = activePoint;
                    break;
                }
            }
        }
    }

    mCanDrag = true;

    if (mActivePoint != -1) {
        mouseMoveEvent(e);
    }

    m_mousePress = e->pos();
}

void MappingDistributionWidget::mouseMoveEvent(QMouseEvent *e)
{
    // If we've moved more then 25 pixels, assume user is dragging
    if (mCanDrag) {
        if (!m_mouseDrag && QPoint(m_mousePress - e->pos()).manhattanLength() > 25) {
            m_mouseDrag = true;
        }
    }

    QPointF pos = e->pos();
    if (mCanDrag) {
        if (m_mouseDrag && mActivePoint >= 0 && mActivePoint < mMappedPoints.size()) {

            // boundary check
            if (pos.x() <= adjustedRect.left()) {
                pos.setX(adjustedRect.left());
            }
            if (pos.x() >= adjustedRect.right()) {
                pos.setX(adjustedRect.right());
            }
            if (pos.y() <= adjustedRect.top()) {
                pos.setY(adjustedRect.top());
            }
            if (pos.y() >= adjustedRect.bottom()) {
                pos.setY(adjustedRect.bottom());
            }


            // dragged point cannot pass previous or next point horizontally
            if (mActivePoint > 0 && mActivePoint < mMappedPoints.size()-1) {
                auto lastPoint = mMappedPoints[mActivePoint-1];
                auto nextPoint = mMappedPoints[mActivePoint+1];
                auto currentPoint = pos;

                if (currentPoint.x() < lastPoint.x()) {
                    pos.setX(lastPoint.x());
                } else if (currentPoint.x() > nextPoint.x()) {
                    pos.setX(nextPoint.x());
                }
                mMappedPoints[mActivePoint] = pos;
            }

            // Can only move first and last point vertically
            if (mActivePoint == mMappedPoints.size()-1 || mActivePoint == 0) {
                mMappedPoints[mActivePoint].setY(pos.y());
            }
            emit pointsUpdatedFromMappingWidget(mapPointsFromWidget());
        }

        // Only update when dragging a point
        update();
    }
}

void MappingDistributionWidget::mouseReleaseEvent(QMouseEvent *)
{
    mCanDrag = false;

    mPoints = mapPointsFromWidget();
    emit pointsUpdatedFromMappingWidget(mPoints);
}

void MappingDistributionWidget::keyPressEvent(QKeyEvent *event)
{
    if (mMappedPoints.size() <= 2) {
        return;
    }

    // Avoid deleting the select the first and last point
    if (mActivePoint == mMappedPoints.count()-1 || mActivePoint == 0) {
        return;
    }

    if (event->key() == Qt::Key_Backspace) {
        mMappedPoints.removeAt(mActivePoint);
    }
    update();
}
