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
#include "util.h"
#include "stdint.h"


QTransform RectMapTransform( QRectF source, QRectF target )
{
    qreal x1 = source.left();
    qreal y1 = source.top();
    qreal x2 = source.right();
    qreal y2 = source.bottom();
    qreal x1P = target.left();
    qreal y1P = target.top();
    qreal x2P = target.right();
    qreal y2P = target.bottom();

    QTransform matrix;
    if ( ( x1 != x2 ) && ( y1 != y2 ) )
    {
        matrix = QTransform( ( x2P - x1P ) / ( x2 - x1 ), // scale x
                             0,
                             0,
                             ( y2P - y1P ) / ( y2 - y1 ), // scale y
                             ( x1P * x2 - x2P * x1 ) / ( x2 - x1 ),    // dx
                             ( y1P * y2 - y2P * y1 ) / ( y2 - y1 ) );  // dy
    }
    else
    {
        matrix.reset();
    }
    return matrix;
}

SignalBlocker::SignalBlocker( QObject* o )
    : mObject( o ),
    mBlocked( o && o->blockSignals( true ) )
{}

SignalBlocker::~SignalBlocker()
{
    if ( mObject )
        mObject->blockSignals( mBlocked );
}

/**
 * @brief Perlin noise generator
 * modified source from https://github.com/Reputeless/PerlinNoise
 */
PerlinNoise::PerlinNoise(std::uint32_t seed)
{
    reseed(seed);
}

void PerlinNoise::reseed(std::uint32_t seed)
{
    for (size_t i = 0; i < 256; ++i)
    {
        p[i] = i;
    }

    std::shuffle(std::begin(p), std::begin(p) + 256, std::default_random_engine(seed));

    for (size_t i = 0; i < 256; ++i)
    {
        p[256 + i] = p[i];
    }
}

double PerlinNoise::noise(double x) const
{
    return noise(x, 0.0, 0.0);
}

double PerlinNoise::noise(double x, double y) const
{
    return noise(x, y, 0.0);
}

double PerlinNoise::noise(double x, double y, double z) const
{
    const std::int32_t X = static_cast<std::int32_t>(std::floor(x)) & 255;
    const std::int32_t Y = static_cast<std::int32_t>(std::floor(y)) & 255;
    const std::int32_t Z = static_cast<std::int32_t>(std::floor(z)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);

    const double u = Fade(x);
    const double v = Fade(y);
    const double w = Fade(z);

    const std::int32_t A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
    const std::int32_t B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return Lerp(w, Lerp(v, Lerp(u, Grad(p[AA], x, y, z),
        Grad(p[BA], x - 1, y, z)),
        Lerp(u, Grad(p[AB], x, y - 1, z),
        Grad(p[BB], x - 1, y - 1, z))),
        Lerp(v, Lerp(u, Grad(p[AA + 1], x, y, z - 1),
        Grad(p[BA + 1], x - 1, y, z - 1)),
        Lerp(u, Grad(p[AB + 1], x, y - 1, z - 1),
        Grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

double PerlinNoise::Fade(double t) noexcept
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double PerlinNoise::Lerp(double t, double a, double b) noexcept
{
    return a + t * (b - a);
}

double PerlinNoise::Grad(std::int32_t hash, double x, double y, double z) noexcept
{
    const std::int32_t h = hash & 15;
    const double u = h < 8 ? x : y;
    const double v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

