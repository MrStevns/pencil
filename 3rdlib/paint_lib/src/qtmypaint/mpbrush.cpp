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
#include "mpbrush.h"
#include <QDebug>

MPBrush::MPBrush()
{
    // Init Brush
    initBrush();

    // Set Default color to black
    setColor(QColor(0, 0, 0));
}

MPBrush::~MPBrush()
{
    mypaint_brush_unref(brush);
}

void MPBrush::initBrush()
{
    brush = mypaint_brush_new();
    mypaint_brush_from_defaults(brush);
}

void MPBrush::load(const QByteArray& content)
{
    mypaint_brush_from_defaults(brush);
    if (!mypaint_brush_from_string(brush, content.constData()))
    {
        // Not able to load the selected brush. Let's execute some backup code...
        qDebug("Trouble when reading the selected brush !");
    }
    setColor(mColor);
}

QColor MPBrush::getColor()
{
    return mColor;
}

void MPBrush::setColor(QColor newColor)
{
    mColor = newColor;

    float h = mColor.hue()/360.0;
    float s = mColor.saturation()/255.0;
    float v = mColor.value()/255.0;

    // Opacity is not handled here as it is defined by the brush settings.
    // If you wish to force opacity, use MPHandler::setBrushValue()
    //
//    float opacity = m_color.alpha()/255.0;
//    mypaint_brush_set_base_value(brush, MYPAINT_BRUSH_SETTING_OPAQUE, opacity);

    setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_H, h);
    setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_S, s);
    setBaseValue(MYPAINT_BRUSH_SETTING_COLOR_V, v);
}

void MPBrush::setWidth(const float size)
{
    setBaseValue(MYPAINT_BRUSH_SETTING_RADIUS_LOGARITHMIC, size);
}

float MPBrush::getBaseValue(MyPaintBrushSetting setting)
{
    return mypaint_brush_get_base_value(brush, setting);
}

float MPBrush::getState(MyPaintBrushState state)
{
    return mypaint_brush_get_state(brush, state);
}

void MPBrush::setBaseValue(MyPaintBrushSetting setting, float value)
{
    mypaint_brush_set_base_value(brush, setting, value);
}

void MPBrush::setMappingPoints(QVector<QPointF> points, MyPaintBrushSetting setting, MyPaintBrushInput input)
{
    mypaint_brush_set_mapping_n(brush, setting,input,points.size());
    for (int i = 0; i < points.size(); i++) {
        QPointF point = points[i];
        float x = static_cast<float>(point.x());
        float y = static_cast<float>(point.y());
        mypaint_brush_set_mapping_point(brush, setting, input, i, x,y);
    }
}

const ControlPoints* MPBrush::getMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input)
{
    return mypaint_brush_get_mapping_control_points(brush, setting, input);
}

int MPBrush::getNumberOfMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input)
{
    return mypaint_brush_get_mapping_n(brush, setting, input);
}

void MPBrush::setNumberOfMappingPoints(MyPaintBrushSetting setting, MyPaintBrushInput input, int value)
{
    mypaint_brush_set_mapping_n(brush, setting, input, value);
}

int MPBrush::getNumberOfInputsUsed(MyPaintBrushSetting setting) const
{
    return mypaint_brush_get_inputs_used_n(brush, setting);
}

const MyPaintBrushSettingInfo* MPBrush::getBrushSettingInfo(MyPaintBrushSetting info)
{
    return mypaint_brush_setting_info(info);
}

const MyPaintBrushInputInfo* MPBrush::getBrushInputInfo(MyPaintBrushInput input)
{
    return mypaint_brush_input_info(input);
}
