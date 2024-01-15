/* brushlib - The MyPaint Brush Library (demonstration project)
 * Copyright (C) 2013 POINTCARRE SARL / Sebastien Leon email: sleon at pointcarre.com
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#include "mptile.h"

MPTile::MPTile():
    mCacheImg(k_tile_dim,k_tile_dim,QImage::Format_ARGB32_Premultiplied)
{
    clear(); //Default tiles are transparent
}

MPTile::MPTile(QImage& image)
{
    mCacheImg = image;
}

MPTile::~MPTile()
{
}

QRect MPTile::boundingRect() const
{
    return mCacheImg.rect();
}

uint16_t* MPTile::bits(bool readOnly)
{
    // Correct C++ way of doing things is using "const" but MyPaint API is not compatible here
    mCacheValid = readOnly ? mCacheValid : false;
    return reinterpret_cast<uint16_t*>(mTPixels);
}

// debug function (simply replace previous value of pixel in t_pixels)
//
void MPTile::drawPoint(uint x, uint y, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
    mCacheValid = false;
    mTPixels[y][x][k_red] = r;
    mTPixels[y][x][k_green] = g;
    mTPixels[y][x][k_blue ] = b;
    mTPixels[y][x][k_alpha] = a;
}


// Time to transfer the pixels from the uint16 to 32 bits cache before repaint...
//
void MPTile::updateCache()
{
    QRgb* dst = (reinterpret_cast<QRgb*>(mCacheImg.bits()));

    const int width = mCacheImg.width();
    const int height = mCacheImg.height();
    for (int y = 0 ; y < height ; y++) {
        for (int x = 0 ; x < width ; x++) {
            const int r = convert_from_mypaint(mTPixels[y][x][k_red]);
            const int g = convert_from_mypaint(mTPixels[y][x][k_green]);
            const int b = convert_from_mypaint(mTPixels[y][x][k_blue]);
            const int a = convert_from_mypaint(mTPixels[y][x][k_alpha]);

            const uint32_t pixel = (static_cast<uint32_t>(a) << 24) |
                                   (static_cast<uint32_t>(r) << 16) |
                                   (static_cast<uint32_t>(g) << 8)  |
                                   static_cast<uint32_t>(b);
            *dst = pixel;
            dst++;
        }
    }

    mCacheValid = true;
}

void MPTile::setImage(const QImage& image)
{
    if (image.isNull()) { return; }

    mCacheImg = image;
    mCacheValid = true;

}

void MPTile::updateMyPaintBuffer(const QSize& tileSize)
{
    const QRgb* dst = (reinterpret_cast<const QRgb*>(mCacheImg.constBits()));
    for (int y = 0 ; y < tileSize.height(); y++) {
        for (int x = 0 ; x < tileSize.width() ; x++) {

            mTPixels[y][x][k_alpha]    = convert_to_mypaint(qAlpha(*dst));
            mTPixels[y][x][k_red]      = convert_to_mypaint(qRed(*dst));
            mTPixels[y][x][k_green]    = convert_to_mypaint(qGreen(*dst));
            mTPixels[y][x][k_blue]     = convert_to_mypaint(qBlue(*dst));
            dst++;
        }
    }
}

void MPTile::clear()
{
    memset(mTPixels, 0, sizeof(mTPixels)); // Tile is transparent
    mCacheImg.fill(Qt::transparent); // image cache is transparent too, and aligned to the pixel table:
    mCacheValid = true;
}
