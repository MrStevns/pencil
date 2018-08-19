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
#include "layermanager.h"
#include "editor.h"
#include "blitrect.h"

#include "layerbitmap.h"
#include "brushfactory.h"

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
    mBrushFactory = new BrushFactory();
}

void StrokeTool::startStroke()
{
    if(emptyFrameActionEnabled())
    {
        mScribbleArea->handleDrawingOnEmptyFrame();
    }

    mFirstDraw = true;
    mLastPixel = getCurrentPixel();
    firstStroke = true;
    
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

    mOpacity = 0.0;
    firstStroke = false;

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

//    qDebug() << "missing dab: deltaX" << deltaX;

    // calculate the distance
    distance = sqrt( deltaX * deltaX + deltaY * deltaY );
//    qDebug() << "distance is: " << distance;

    return distance;
}

QRgb StrokeTool::getSurfaceColor(float posX, float posY, QColor brushColor) {

    Layer* layer = mEditor->layers()->currentLayer();
    uint frameNumber = mEditor->currentFrame();
    BitmapImage surfaceImage = *static_cast<LayerBitmap*>(layer)->getBitmapImageAtFrame(frameNumber);

    surfaceImage = surfaceImage.copy(QRect(posX-properties.width/2,posY-properties.width/2,properties.width,properties.width));

    return BrushFactory::colorMeanOfPixels(*surfaceImage.image(), brushColor);

}

void StrokeTool::strokeTo(Brush& brush, float lastx, float lasty) {


//    if (properties.pressure == true) {
        mOpacity += qPow(0.1,0.1);
//    }

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

    // since we want to dab at a specific interval,
    // add the potentially missing leftover distance to the current distance
    float totalDistance = leftOverDistance + distance;

    mSurfaceColor = getSurfaceColor(lastx,lasty, brush.color);
//    if (brush.brushImage) {
    QImage* image = mBrushFactory->createRadialImage(mSurfaceColor,brush.color,
                                                   properties.width,
                                                   properties.feather,
                                                   mOpacity);
        mBrushFactory->applySimpleNoise(*image);
        brush.brushImage = image;
//    }

    bool didPaint = false;

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

        QPoint stampAt;
        if (brush.scatterAmount > 0) {

            if (brush.scatterDensity == 0) {
                brush.scatterDensity = 1;
            }
            for (int i = 0; i < brush.scatterDensity; i++) {
                float randX = qrand() % (int)brushWidth*2 + (-brushWidth);
                float randY = qrand() % (int)brushWidth*2 + (-brushWidth);

                stampAt = QPoint(lastx+offsetX+randX,
                               lasty+offsetY+randY);

                rect.extend(stampAt);

                mScribbleArea->drawBrush(brush, stampAt.x(), stampAt.y() );
            }
        } else {
            stampAt = QPoint(lastx+offsetX, lasty+offsetY);

            rect.extend(stampAt);

            mScribbleArea->drawBrush(brush, stampAt.x(), stampAt.y());
            didPaint = true;

        }

        // remove the distance we've covered already
        totalDistance -= spacing;
    }

    // control the fadeout at the end of a stroke
    mOpacity = 0;

    int rad = qRound(brushWidth);

    mScribbleArea->paintBitmapBuffer();
//    mScribbleArea->paintBitmapBufferRect( QRect(lasty-properties.width/2,lasty-properties.width/2,properties.width,properties.width) );
//    mScribbleArea->refreshBitmap(QRect(lastx-properties.width/2,lasty-properties.width/2,properties.width,properties.width), rad );

    // there might still be dabbing we didn't cover
    // so set the remaining to our new left over distance
    mLeftOverDabDistance = totalDistance;
}
