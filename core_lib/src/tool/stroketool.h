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

#ifndef STROKETOOL_H
#define STROKETOOL_H

#include "basetool.h"

#include <QList>
#include <QPointF>
#include <QMap>

#include "brushfactory.h"

class BrushFactory;

class StrokeTool : public BaseTool
{
    Q_OBJECT

public:
    explicit StrokeTool(QObject* parent);
    /**
     * @brief startStroke, this prepares a bunch of settings before we initialize our stroke
     * it is mainly used by vector
     */
    void startStroke();


    /**
     * @brief drawStroke, Prepares the next stroke for stroke and pressure interpolation
     */
    void drawStroke();

    /**
     * @brief endStroke
     */
    void endStroke();

    bool keyPressEvent(QKeyEvent* event) override;
    bool keyReleaseEvent(QKeyEvent* event) override;

    /**
     * @brief StrokeTool::missingDabs, calculates the euclidean distance to figure out
     * the distance we need to cover with dabs
     * @param x, x pos of next dab
     * @param y, y pos of next dab
     * @return float, how many dabs will be drawn between the last and current pos
     */
    float missingDabs(float cux, float cuy, float oldX, float oldY);


    /**
     * @brief strokeTo, Performs the actual stroking from a to b.
     * @param brushObject, the struct containing all relevant information about our brush
     * @param x, last x position
     * @param y, last y position
     */
    void strokeTo(Brush& brush, float lastx, float lasty);


    /**
     * @brief getSurfaceColor, gets the average surface color by applying squares mean to every pixel in the image
     * @param brush, the brush struct that contains position information, image etc that is required.
     * @return the average color of the surface image
     */
    QRgb getSurfaceColor(float posX, float posY, QColor brushColor);
protected:
    bool mFirstDraw = false;

    QList<QPointF> mStrokePoints;
    QList<qreal> mStrokePressures;

    qreal mCurrentWidth    = 0.0;
    qreal mCurrentPressure = 0.5;

    /// Whether to enable the "drawing on empty frame" preference.
    /// If true, then the user preference is honored.
    /// If false, then the stroke is drawn on the previous key-frame (i.e. the
    /// "old" Pencil behaviour).
    /// Returns true by default.
    virtual bool emptyFrameActionEnabled();

    float deltaX;
    float deltaY;

    float mLeftOverDabDistance;
    float mOpacity = 0;

    bool firstStroke = true;

    BrushFactory* mBrushFactory;

private:
	QPointF mLastPixel = { 0, 0 };
};

#endif // STROKETOOL_H
