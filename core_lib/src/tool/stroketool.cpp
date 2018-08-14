/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "stroketool.h"

#include "scribblearea.h"
#include "strokemanager.h"
#include "viewmanager.h"
#include "editor.h"
#include "blitrect.h"

#include <QtMath>

#ifdef Q_OS_MAC
extern "C" {
    void detectWhichOSX();
    void disableCoalescing();
    void enableCoalescing();
}
#else
extern "C" {
    void detectWhichOSX() {}
    void disableCoalescing() {}
    void enableCoalescing() {}
}
#endif

StrokeTool::StrokeTool( QObject *parent ) :
BaseTool( parent )
{
    detectWhichOSX();
}

void StrokeTool::startStroke()
{
    if(emptyFrameActionEnabled())
    {
        mScribbleArea->handleDrawingOnEmptyFrame();
    }

    mFirstDraw = true;
    mLastPixel = getCurrentPixel();
    
    mStrokePoints.clear();

    //Experimental
    QPointF startStrokes =  m_pStrokeManager->interpolateStart(mLastPixel);
    mStrokePoints << mEditor->view()->mapScreenToCanvas( startStrokes );

    mStrokePressures.clear();
    mStrokePressures << m_pStrokeManager->getPressure();

    disableCoalescing();
}

bool StrokeTool::keyPressEvent(QKeyEvent *event)
{
    switch ( event->key() ) {
    case Qt::Key_Alt:
        mScribbleArea->setTemporaryTool( EYEDROPPER );
        return true;
    case Qt::Key_Space:
        mScribbleArea->setTemporaryTool( HAND ); // just call "setTemporaryTool()" to activate temporarily any tool
        return true;
    }
    return false;
}

bool StrokeTool::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    return true;
}

bool StrokeTool::emptyFrameActionEnabled()
{
    return true;
}

void StrokeTool::endStroke()
{
    m_pStrokeManager->interpolateEnd();
    mStrokePressures << m_pStrokeManager->getPressure();
    mStrokePoints.clear();
    mStrokePressures.clear();

    enableCoalescing();
}

void StrokeTool::drawStroke()
{
    QPointF pixel = getCurrentPixel();
    if ( pixel != mLastPixel || !mFirstDraw )
    {

        // get last pixel before interpolation initializes
        QPointF startStrokes =  m_pStrokeManager->interpolateStart(getLastPoint());
        mStrokePoints << startStrokes;
        mStrokePressures << m_pStrokeManager->getPressure();

    }
    else
    {
        mFirstDraw = false;
    }
}

float StrokeTool::missingDabs(float cux, float cuy, float oldX, float oldY)
{
    float distance;
    deltaX = 0;
    deltaY = 0;

    // last - current pos
    // this will determine the slope of the line
    deltaX = oldX - cux;
    deltaY = oldY - cuy;

    qDebug() << "missing dab: deltaX" << deltaX;

    // calculate the distance
    distance = sqrt( deltaX * deltaX + deltaY * deltaY );
    qDebug() << "distance is: " << distance;

    return distance;
}

void StrokeTool::strokeTo(Brush& brush, float lastx, float lasty) {
    qreal opacity = 1.0;
    if (properties.pressure == true) {
        opacity = mCurrentPressure / 2;
    }

    mCurrentWidth = properties.width;
    qreal brushWidth = mCurrentWidth;

    BlitRect rect;

    float leftOverDistance = mLeftOverDabDistance;

    // calculate the euclidean distance
    // to find the distance that we need to cover with dabs
    float distance = missingDabs(lastx, lasty, getCurrentPoint().x(), getCurrentPoint().y());

    float spacing = brushWidth*brush.dabSpacing;

    float stepX = deltaX / distance;
    float stepY = deltaY / distance;

    float offsetX = 0.0;
    float offsetY = 0.0;

    // since we want to stab at a specific interval,
    // add the potentially missing leftover distance to the current distance
    float totalDistance = leftOverDistance + distance;

    // will dap until totalDistance is less than spacing
    while (totalDistance >= spacing)
    {
        // make sure to add potentially missed distance
        // to our offset

        if (leftOverDistance > 0) {
            offsetX += stepX * (spacing - leftOverDistance);
            offsetY += stepY * (spacing - leftOverDistance);

            leftOverDistance -= spacing;
        } else {

            // otherwise just calculate the offset from the
            // direction (stepX, stepY) and spacing.
            offsetX += stepX * spacing;
            offsetY += stepY * spacing;

        }

        for (int i = 0; i < brush.scatterDensity; i++) {
            float randX = qrand() % (int)brushWidth*2 + (-brushWidth);
            float randY = qrand() % (int)brushWidth*2 + (-brushWidth);

//            qDebug() << "rand x" << randX;
//            qDebug() << "rand y" << randY;

            QPoint stampAt(lastx+offsetX+randX,
                           lasty+offsetY+randY);

            rect.extend(stampAt);

            mScribbleArea->drawBrush( brush,stampAt.x(),stampAt.y() );
        }


        // remove the distance we've covered already
        totalDistance -= spacing;
    }

    int rad = qRound( brushWidth  / 2 + 2);

    mScribbleArea->paintBitmapBufferRect( rect );
    mScribbleArea->refreshBitmap( rect, rad );

    // there might still be dabbing we didn't cover
    // so set the remaining to our new left over distance
    mLeftOverDabDistance = totalDistance;
}
