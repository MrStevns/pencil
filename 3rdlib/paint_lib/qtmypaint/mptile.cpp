/* brushlib - The MyPaint Brush Library (demonstration project)
 * Copyright (C) 2013 POINTCARRE SARL / Sebastien Leon email: sleon at pointcarre.com
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "mptile.h"

MPTile::MPTile():
    m_cache_img(k_tile_dim,k_tile_dim,QImage::Format_ARGB32_Premultiplied)
{
    clear(); //Default tiles are transparent
}

MPTile::MPTile(QImage& image)
{
    m_cache_img = image;
}

MPTile::~MPTile()
{
}

QRect MPTile::boundingRect() const
{
    return m_cache_img.rect();
}

uint16_t* MPTile::bits(bool readOnly)
{
    // Correct C++ way of doing things is using "const" but MyPaint API is not compatible here
    m_cache_valid = readOnly ? m_cache_valid : false;
    return reinterpret_cast<uint16_t*>(t_pixels);
}

// debug function (simply replace previous value of pixel in t_pixels)
//
void MPTile::drawPoint(uint x, uint y, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
    m_cache_valid = false;
    t_pixels[y][x][k_red] = r;
    t_pixels[y][x][k_green] = g;
    t_pixels[y][x][k_blue ] = b;
    t_pixels[y][x][k_alpha] = a;
}


// Time to transfer the pixels from the uint16 to 32 bits cache before repaint...
//
void MPTile::updateCache()
{
    QRgb* dst = (reinterpret_cast<QRgb*>(m_cache_img.bits()));

    for (int y = 0 ; y < k_tile_dim ; y++) {
         for (int x = 0 ; x < k_tile_dim ; x++) {
              uint16_t& alpha = t_pixels[y][x][k_alpha];
              *dst = alpha ? qRgba(
              convert_from_mypaint(t_pixels[y][x][k_red]),
              convert_from_mypaint(t_pixels[y][x][k_green]),
              convert_from_mypaint(t_pixels[y][x][k_blue]),
              convert_from_mypaint(alpha)) : 0; // aplha is 0 => all is zero (little optimization)
              dst++; // next image pixel...
         }
    }

    m_cache_valid = true;
}

void MPTile::setImage(const QImage& image)
{
    if (image.isNull()) { return; }

    m_cache_img = image;
    m_cache_valid = true;

}

void MPTile::updateMyPaintBuffer(const QSize& tileSize)
{
    const QRgb* dst = (reinterpret_cast<const QRgb*>(m_cache_img.constBits()));
    for (int y = 0 ; y < tileSize.height(); y++) {
         for (int x = 0 ; x < tileSize.width() ; x++) {

            t_pixels[y][x][k_alpha]    = convert_to_mypaint(qAlpha(*dst));
            t_pixels[y][x][k_red]      = convert_to_mypaint(qRed(*dst));
            t_pixels[y][x][k_green]    = convert_to_mypaint(qGreen(*dst));
            t_pixels[y][x][k_blue]     = convert_to_mypaint(qBlue(*dst));
            dst++;
         }
    }
}

void MPTile::clear()
{
    memset(t_pixels, 0, sizeof(t_pixels)); // Tile is transparent
    m_cache_img.fill(Qt::transparent); // image cache is transparent too, and aligned to the pixel table:
    m_cache_valid = true;
    m_dirty = false;
}
