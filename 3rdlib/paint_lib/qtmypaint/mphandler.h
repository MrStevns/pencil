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
#ifndef MPHANDLER_H
#define MPHANDLER_H

#include <QColor>
#include <memory>

extern "C" {
#include "mypaint-brush.h"
#include "mypaint-surface.h"
}

#include "mpbrush.h"
#include "mpsurface.h"

#ifndef QTMYPAINT_SURFACE_WIDTH
#define QTMYPAINT_SURFACE_WIDTH 100
#endif

#ifndef QTMYPAINT_SURFACE_HEIGHT
#define QTMYPAINT_SURFACE_HEIGHT 100
#endif

class QPixmap;

class MPHandler : public QObject
{
    Q_OBJECT

public:
    MPHandler();
    ~MPHandler();

    static MPHandler *handler();

    void startStroke();
    void strokeTo(float x, float y, float pressure, float xtilt, float ytilt, double dtime);
    void strokeTo(float x, float y);
    void endStroke();

    float getBrushSettingBaseValue(MyPaintBrushSetting setting);
    float getBrushState(MyPaintBrushState state);

    QColor getSurfaceColor(float x, float y, int radius);

    void setBrushColor(QColor newColor);
    void setBrushBaseValue(MyPaintBrushSetting setting, float value);
    void setBrushWidth(float width);

    int getBrushNumberOfMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input);
    void setBrushNumberOfMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input, int value);

    int getBrushInputsUsed(MyPaintBrushSetting setting);

    const MyPaintBrushSettingInfo* getBrushSettingInfo(MyPaintBrushSetting setting);
    const MyPaintBrushInputInfo* getBrushInputInfo(MyPaintBrushInput input);

    void setBrushInputMappingPoints(QVector<QPointF> points, MyPaintBrushSetting settingType, MyPaintBrushInput inputType);
    const ControlPoints* getBrushInputMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input);

    void refreshSurface();
    void saveSurface(const QString path);

    void setSurfaceSize(QSize size);
    QSize surfaceSize();

    void clearSurface();

    void drawImageAt(const QImage& image, const QPoint topLeft);
    void loadImage(const QImage &image, const QPoint pos);
    void loadTile(const QImage& image, const QPoint& topLeft, MPTile* tile);

signals:
    void tileUpdated(MPSurface *surface, MPTile *tile);
    void tileAdded(MPSurface *surface, MPTile *tile);
    void tileCleared(MPSurface* surface, QRect tileRect);
    void surfaceCleared(MPSurface *surface);

public slots:
    void loadBrush(const QByteArray& content);

private:
    MPBrush *   m_brush;
    MPSurface * m_surface;

};

#endif // MPHANDLER_H
