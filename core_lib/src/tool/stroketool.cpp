#include "stroketool.h"

#include "scribblearea.h"
#include "strokemanager.h"
#include "editor.h"

#ifdef Q_OS_MAC
extern "C" {
    void detectElCapitan();
    void disableCoalescing();
    void enableCoalescing();
}
#else
extern "C" {
    void detectElCapitan() {}
    void disableCoalescing() {}
    void enableCoalescing() {}
}
#endif

StrokeTool::StrokeTool( QObject *parent ) :
BaseTool( parent )
{
    detectElCapitan();
}

void StrokeTool::adjustPressureSensitiveProperties( qreal pressure, bool mouseDevice )
{
    if ( properties.pressure && !mouseDevice )
    {
        mCurrentPressure = pressure;
    }
    else
    {
        mCurrentPressure = 1.0;
    }
}

void StrokeTool::adjustTiltProperties(int xTilt, int yTilt, bool mouseDevice)
{
    if ( properties.pressure && !mouseDevice )
    {
        mCurrentXTilt = xTilt;
        mCurrentYTilt = yTilt;
    }
    else
    {
        mCurrentXTilt = 0;
        mCurrentYTilt = 0;
    }
}

void StrokeTool::startStroke()
{
    mScribbleArea->startStroke();

    mFirstDraw = true;
    mLastPixel = getCurrentPixel();
    
	mStrokePoints.clear();
    mStrokePoints << mEditor->view()->mapScreenToCanvas( mLastPixel );

    mStrokePressures.clear();
    mStrokePressures << m_pStrokeManager->getPressure();

    mCurrentLayerType = mEditor->layers()->currentLayer()->type();

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
    return true;
}


void StrokeTool::endStroke()
{
    mStrokePoints.clear();
    mStrokePressures.clear();

    enableCoalescing();

    mScribbleArea->endStroke();
}

void StrokeTool::drawStroke()
{
    QPointF pixel = getCurrentPixel();

    mScribbleArea->strokeTo(pixel, mCurrentPressure, mCurrentXTilt,  mCurrentYTilt);

    if ( pixel != mLastPixel || !mFirstDraw )
    {
        mLastPixel = pixel;
        mStrokePoints << mEditor->view()->mapScreenToCanvas( pixel );
        mStrokePressures << m_pStrokeManager->getPressure();
    }
    else
    {
        mFirstDraw = false;
    }
}
