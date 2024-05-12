/*
Copyright (c) 2016, François Téchené
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "mphandler.h"

#include <QVector>
#include <QPixmap>

#include <qdebug.h>

extern "C" {
#include "mypaint-brush.h"
#include "mypaint-surface.h"
}

MPHandler::MPHandler()
{
    QSize defaultSize = QSize(QTMYPAINT_SURFACE_WIDTH, QTMYPAINT_SURFACE_HEIGHT);

    mBrush = new MPBrush();
    mSurface = new MPSurface(this, defaultSize);
}

MPHandler::~MPHandler()
{
    mypaint_surface_unref(reinterpret_cast<MyPaintSurface*>(mSurface));
}

void MPHandler::setSurfaceSize(QSize size)
{
    mSurface->setSize(size);
}

QSize MPHandler::surfaceSize()
{
    return mSurface->size();
}

void MPHandler::clearSurface()
{
    mSurface->clear();
}

/**
 * @brief MPHandler::saveSurface
 * Debug method to check the content of surface
 * @param path
 */
void MPHandler::saveSurface(const QString path)
{
    mSurface->saveSurface(path);
}

void MPHandler::loadTile(const QImage& image, const QPoint& topLeft, MPTile* tile)
{
    mSurface->loadTile(image, topLeft, tile);
}

void MPHandler::loadBrush(const QByteArray &content)
{
    mBrush->load(content);
}

void MPHandler::setBrushWidth(float width)
{
    mBrush->setWidth(width);
}

void
MPHandler::
strokeTo(float x, float y, float pressure, float xtilt, float ytilt, double dtime)
{
    auto surface = reinterpret_cast<MyPaintSurface*>(mSurface);

    mypaint_surface_begin_atomic(surface);
    mypaint_brush_stroke_to(mBrush->brush, surface,
                            x,
                            y,
                            pressure,
                            xtilt,
                            ytilt,
                            dtime/*, 1.0, 1.0, .0*/);


    this->x = x;
    this->y = y;
    this->pressure = pressure;
    this->tiltX = xtilt;
    this->tiltY = ytilt;
    this->dTime = dtime;

    MyPaintRectangle roi;
    mypaint_surface_end_atomic(surface, &roi);
}

QColor
MPHandler::getSurfaceColor(float x, float y, int radius)
{
    auto surface = reinterpret_cast<MyPaintSurface*>(mSurface);
    float r, g, b, a;
    mypaint_surface_get_color(surface, x, y, radius, &r,&g,&b,&a);


    r = r * 255;
    g = g * 255;
    b = b * 255;
    a = a * 255;
    return QColor(static_cast<int>(r),
                  static_cast<int>(g),
                  static_cast<int>(b),
                  static_cast<int>(a));
}

void
MPHandler::startStroke()
{
    mypaint_brush_reset (mBrush->brush);
    mypaint_brush_new_stroke(mBrush->brush);
}

void
MPHandler::strokeTo(float x, float y)
{
    float pressure = 1.0;
    float xtilt = 0.0;
    float ytilt = 0.0;

    strokeTo(x, y, pressure, xtilt, ytilt, 1.0);

    this->x = x;
    this->y = y;
    this->pressure = pressure;
    this->tiltX = xtilt;
    this->tiltY = ytilt;
    this->dTime = 1.0;
}

void MPHandler::strokeTo()
{
    strokeTo(this->x, this->y, this->pressure, this->tiltX, this->tiltY, this->dTime);
}

void
MPHandler::endStroke()
{
    mypaint_brush_reset(mBrush->brush);
}

float MPHandler::getBrushSettingBaseValue(MyPaintBrushSetting setting)
{
    return mBrush->getBaseValue(setting);
}

float MPHandler::getBrushState(MyPaintBrushState state)
{
    return mBrush->getState(state);
}

void
MPHandler::setBrushColor(QColor newColor)
{
    mBrush->setColor(newColor);
}

void MPHandler::setBrushBaseValue(MyPaintBrushSetting setting, float value)
{
    mBrush->setBaseValue(setting, value);
}

int  MPHandler::getBrushNumberOfMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input)
{
    return mBrush->getNumberOfMappingPoints(setting, input);
}

void MPHandler::setBrushNumberOfMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input, int value)
{
    mBrush->setNumberOfMappingPoints(setting, input, value);
}

int MPHandler::getBrushInputsUsed(MyPaintBrushSetting setting)
{
    return mBrush->getNumberOfInputsUsed(setting);
}

const ControlPoints* MPHandler::getBrushInputMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input)
{
    return mBrush->getMappingPoints(setting, input);
}

void MPHandler::setBrushInputMappingPoints(QVector<QPointF> points, MyPaintBrushSetting settingType, MyPaintBrushInput inputType)
{
    mBrush->setMappingPoints(points, settingType, inputType);
}

const MyPaintBrushSettingInfo* MPHandler::getBrushSettingInfo(MyPaintBrushSetting setting)
{
    return mBrush->getBrushSettingInfo(setting);
}

const MyPaintBrushInputInfo* MPHandler::getBrushInputInfo(MyPaintBrushInput input)
{
    return mBrush->getBrushInputInfo(input);
}
