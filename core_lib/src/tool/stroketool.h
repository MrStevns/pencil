#ifndef STROKETOOL_H
#define STROKETOOL_H

#include "basetool.h"
#include "layermanager.h"

#include <QList>
#include <QPointF>
#include <QPair>

typedef QPair<QPointF, QPointF> QSegment;

class StrokeTool : public BaseTool
{
    Q_OBJECT

public:
    explicit StrokeTool(QObject *parent = 0);
    
    void startStroke();
    void drawStroke();
    void endStroke();

    void adjustPressureSensitiveProperties( qreal pressure, bool mouseDevice ) override;
    void adjustTiltProperties(int xTilt, int yTilt, bool mouseDevice ) override;

    bool keyPressEvent(QKeyEvent *event) override;

    bool keyReleaseEvent(QKeyEvent *event) override;

protected:
    bool mFirstDraw = false;

    QList<QPointF> mStrokePoints;
    QList<qreal> mStrokePressures;

    qreal mCurrentWidth;
    qreal mCurrentPressure;
    int mCurrentXTilt;
    int mCurrentYTilt;

private:
	QPointF mLastPixel = { 0, 0 };
};

#endif // STROKETOOL_H
