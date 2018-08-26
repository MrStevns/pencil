/*

Pencil - Traditional Animation Software
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "catch.hpp"

#include <brushfactory.h>

TEST_CASE("Brush Factory")
{
    SECTION("Construct radial image")
    {
        BrushFactory factory;

        // hard brush
//        QImage* pix = factory.createRadialImage(qRgba(0,0,0,0),QColor(0,0,0,255), 40, 5, 1.0);
//        pix->save("/Users/CandyFace/Desktop/brush.png");
        // test noise
//        factory.applySimpleNoise(*pix);

        // softer
//        QImage* pix2 = factory.createRadialImage(QColor(0,0,0, 255), 20, 20, 1.0);
//        pix2->save("/Users/CandyFace/Desktop/brush2.png");

        // very soft
//        QImage* pix3 = factory.createRadialImage(QColor(0,0,0), 40, 60, 1.0);
//        pix3->save("/Users/CandyFace/Desktop/brush3.png");

//        QImage* image = new QImage(256,256, QImage::Format_ARGB32_Premultiplied);
//        image->fill(Qt::white);
//        QImage* pix4 = factory.createRadialImage(QColor(0,0,0,255), 40, 5, 1.0);
//        factory.applyPerlinNoise(*pix4, 5.0, 10.0, 1, 255);

//        pix4->save("/Users/CandyFace/Desktop/perlin.png");

    }

}
