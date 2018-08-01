/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2013-2014 Matt Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation;

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#ifndef SCRIBBLEAREA_H
#define SCRIBBLEAREA_H

#include <cstdint>
#include <ctime>
#include <deque>

#include <QColor>
#include <QTransform>
#include <QImage>
#include <QPoint>
#include <QWidget>
#include <QFrame>
#include <QHash>
#include <QGraphicsView>

#include "log.h"
#include "pencildef.h"
#include "vectorimage.h"
#include "bitmapimage.h"
#include "colourref.h"
#include "vectorselection.h"
#include "colormanager.h"
#include "viewmanager.h"
#include "canvasrenderer.h"
#include "preferencemanager.h"

#include "mphandler.h"
#include "mpbrush.h"
#include "mpsurface.h"
#include "mptile.h"

class Layer;
class Editor;
class BaseTool;
class StrokeManager;
class ColorManager;
//class PopupColorPaletteWidget;


class ScribbleArea : public QGraphicsView
{
    Q_OBJECT

    friend class MoveTool;
    friend class EditTool;
    friend class SmudgeTool;

public:
    ScribbleArea( QWidget *parent );
    ~ScribbleArea();

    bool init();
    void setCore( Editor* pCore ) { mEditor = pCore; }

    void deleteSelection();
    void setSelection( QRectF rect, bool );
    void displaySelectionProperties();
    QRectF getSelection() const { return mySelection; }
    bool somethingSelected;
    QRectF mySelection, myTransformedSelection, myTempTransformedSelection;
    qreal myRotatedAngle;
    qreal myFlipX, myFlipY; // scale -1.0 or +1.0

    bool areLayersSane() const;
    bool isLayerPaintable() const;

    void setEffect( SETTING e, bool isOn );

    int showAllLayers() const { return mShowAllLayers; }
    qreal getCurveSmoothing() const { return mCurveSmoothingLevel; }
    bool usePressure() const { return mUsePressure; }
    bool makeInvisible() const { return mMakeInvisible; }

    enum MoveMode { MIDDLE, TOPLEFT, TOPRIGHT, BOTTOMLEFT, BOTTOMRIGHT, ROTATION, SYMMETRY, NONE };
    MoveMode getMoveMode() const { return mMoveMode; }
    void setMoveMode( MoveMode moveMode ) { mMoveMode = moveMode; }

    QTransform getView();
    QRectF getViewRect();
    QPointF getCentralPoint();

    void showCurrentFrame();

    void updateBackground();
    void updateCurrentFrame();
    void updateFrame( int frame );
    void updateAllFrames();
    void updateAllVectorLayersAtCurrentFrame();
    void updateAllVectorLayersAt( int frame );
    void updateAllVectorLayers();

    bool shouldUpdateAll() const { return mNeedUpdateAll; }
    void setAllDirty() { mNeedUpdateAll = true; }

    BaseTool* currentTool();
    BaseTool* getTool( ToolType eToolMode );
    void setCurrentTool( ToolType eToolMode );
    void setTemporaryTool( ToolType eToolMode );
    void setPrevTool();

    StrokeManager *getStrokeManager() const { return mStrokeManager; }

    //PopupColorPaletteWidget *getPopupPalette() const { return m_popupPaletteWidget; }

    Editor* editor() { return mEditor; }

    void floodFillError( int errorType );

    bool isMouseInUse() { return mMouseInUse; }

    void loadBrush(const QByteArray &content);

signals:
    void modification();
    void modification( int );
    void multiLayerOnionSkinChanged( bool );
    void refreshPreview();

public slots:
    void clearImage();
    void calculateSelectionRect();
    QTransform getSelectionTransformation() { return selectionTransformation; }
    void calculateSelectionTransformation();
    void paintTransformedSelection();
    void applyTransformedSelection();
    void cancelTransformedSelection();
    void setModified( int layerNumber, int frameNumber );

    void selectAll();
    void deselectAll();

    void toggleOnionPrev( bool );
    void toggleOnionNext( bool );
    void toggleOnionBlue( bool );
    void toggleOnionRed( bool );
    void toggleGrid( bool );

    void setCurveSmoothing( int );
    void toggleThinLines();
    void toggleOutlines();
    void toggleShowAllLayers();
    void escape();

    void toggleMultiLayerOnionSkin( bool );
    void togglePopupPalette();

    void updateToolCursor();
    void paletteColorChanged(QColor);

    void onNewTile(MPSurface *surface, MPTile *tile);
    void onUpdateTile(MPSurface *surface, MPTile *tile);
    void updateTile(MPSurface *surface, MPTile *tile);

protected:
    void tabletEvent( QTabletEvent* ) override;
    void wheelEvent( QWheelEvent* ) override;
    void mousePressEvent( QMouseEvent* ) override;
    void mouseMoveEvent( QMouseEvent* ) override;
    void mouseReleaseEvent( QMouseEvent* ) override;
    void mouseDoubleClickEvent( QMouseEvent* ) override;
    void keyPressEvent( QKeyEvent* ) override;
    void keyReleaseEvent( QKeyEvent* ) override;
//    void paintEvent( QPaintEvent* ) override;
    void resizeEvent( QResizeEvent* ) override;

public:
    void startStroke();
    void strokeTo(QPointF point, float pressure, float xtilt, float ytilt);
    void endStroke();

    void drawPolyline( QList<QPointF> points, QPointF lastPoint );
    void endPolyline( QList<QPointF> points );

    void drawLine( QPointF P1, QPointF P2, QPen pen, QPainter::CompositionMode cm );
    void drawPath( QPainterPath path, QPen pen, QBrush brush, QPainter::CompositionMode cm );
    void drawPen( QPointF thePoint, qreal brushWidth, QColor fillColour, qreal opacity );
    void drawPencil( QPointF thePoint, qreal brushWidth, QColor fillColour, qreal opacity );
    void drawBrush( QPointF thePoint, qreal brushWidth, qreal offset, QColor fillColour, qreal opacity, bool usingFeather = true );
    void drawEraser( QPointF thePoint, qreal brushWidth, qreal offset, QColor fillColour, qreal opacity );
    void blurBrush( BitmapImage *bmiSource_, QPointF srcPoint_, QPointF thePoint_, qreal brushWidth_, qreal offset_, qreal opacity_ );
    void liquifyBrush( BitmapImage *bmiSource_, QPointF srcPoint_, QPointF thePoint_, qreal brushWidth_, qreal offset_, qreal opacity_ );

    void paintBitmapBuffer();
    void paintBitmapBufferRect( QRect rect );
    void refreshBitmap( const QRectF& rect, int rad );
    void refreshVector( const QRectF& rect, int rad );
    void setGaussianGradient( QGradient &gradient, QColor colour, qreal opacity, qreal offset );

    void switchToPreviewMode();
    void switchToDrawingMode();

    BitmapImage* mBufferImg = nullptr; // used to pre-draw vector modifications
    BitmapImage* mStrokeImg = nullptr; // used for brush strokes before they are finalized


    void onTabletEnterProximity(const QTabletEvent *event);
    void onTabletLeaveProximity(const QTabletEvent *event);

private:
    RenderOptions getRenderOptions();

    void drawCanvas(int frame, QRect rect );
    void drawCanvasBack(int frame, QRect rect);
    void drawCanvasLayer(int frame, QRect rect);
    void drawCanvasTop(int frame, QRect rect);

    QString getCachedFrameKey(int frame);

    void loadFullCanvas();
    void loadBackCanvas();
    void loadTiles();
    void loadTopCanvas();

    QGraphicsPixmapItem* getTileFromPosition(QPoint point);

    void drawAxis( QPainter& );
    void drawGrid( QPainter& );

    void settingUpdated(SETTING setting);
    void applyBackgroundShadow(QPainter &painter);

    MoveMode mMoveMode = MIDDLE;
    ToolType mPrevTemporalToolType;
    ToolType mPrevToolType = PEN; // previous tool (except temporal)

    BitmapImage mBitmapSelection; // used to temporary store a transformed portion of a bitmap image
    bool isTransforming = false;

    StrokeManager* mStrokeManager = nullptr;

    Editor* mEditor = nullptr;

    //PopupColorPaletteWidget* m_popupPaletteWidget; // color palette popup (may be enhanced with tools)

    bool mIsSimplified  = false;
    bool mShowThinLines = false;
    bool mQuickSizing = true;
    int  mShowAllLayers;
    bool mUsePressure   = true;
    bool mMakeInvisible = false;
    bool mToolCursors   = true;
    qreal mCurveSmoothingLevel = 0.0;
    bool mMultiLayerOnionSkin; // future use. If required, just add a checkbox to updated it.
    QColor mOnionColor;

    bool mNeedUpdateAll = false;
    bool mNeedQuickUpdate = false;

    bool mKeyboardInUse = false;
    bool mMouseInUse    = false;
    QPointF mLastPixel;
    QPointF mCurrentPixel;
    QPointF mLastPoint;
    QPointF mCurrentPoint;

    qreal tol;
    QList<int> mClosestCurves;
    QList<VertexRef> mClosestVertices;
    QPointF mOffset;

    //instant tool (temporal eg. eraser)
    bool instantTool = false; //whether or not using temporal tool

    VectorSelection vectorSelection;
    QTransform selectionTransformation;

    PreferenceManager *mPrefs = nullptr;

    QPixmap mCanvas;
    QPixmap mCanvasBack;
    QPixmap mCanvasLayer;
    QPixmap mCanvasTop;

    CanvasRenderer mCanvasRenderer;


    // debug
    QRectF mDebugRect;
    QLoggingCategory mLog;
    std::deque< clock_t > mDebugTimeQue;

    QGraphicsPixmapItem* mCanvasItem;

    QGraphicsPixmapItem* mBackgroundItem;
    QGraphicsPixmapItem* mCanvasBackItem;
    QGraphicsPixmapItem* mCanvasTopItem;

    QGraphicsScene mScene;
    MPHandler *mMypaint;

    QHash<QString, QGraphicsPixmapItem*> mTiles;

    bool isInPreviewMode = false;

    qreal mH;



};

#endif
