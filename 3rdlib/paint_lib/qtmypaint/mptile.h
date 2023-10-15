/* brushlib - The MyPaint Brush Library (demonstration project)
 * Copyright (C) 2013 POINTCARRE SARL / Sebastien Leon email: sleon at pointcarre.com
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#ifndef TILE_H
#define TILE_H

#include <QImage>
#include <stdint.h>

//-------------------------------------------------------------------------
// This basic class store a tile info & display it. the ushort table is the
// real info modified by libMyPaint. Before any screen refresh, we transfer
// it to a QImage acting as a cache. this QImage is only necessary to paint.
// NOTE that the uint16_t data (premul RGB 16 bits) is transfered in premul
// format. This is only useful if you plan to have several layers.
// if it is not the case, you could simply convert to RGBA (not premul)

#define CONV_16_8(x) ((x*255)/(1<<15))
#define CONV_8_16(x) ((x*(1<<15))/255)

class MPTile
{
public:

    explicit MPTile ();
    explicit MPTile (QImage& image);
    ~MPTile();

    enum { k_tile_dim = 64 };
    enum { k_red = 0, k_green = 1, k_blue = 2, k_alpha = 3 }; // Index to access RGBA values in myPaint

    const QImage& image() const { return mCacheImg; }
    QImage& image() { return mCacheImg; }

    QRect boundingRect() const;

    uint16_t* bits (bool readOnly);
    void drawPoint ( uint x, uint y, uint16_t r, uint16_t g, uint16_t b, uint16_t a );
    void updateCache();
    void clear();
    void setImage(const QImage& image);

    /**
     * @brief updateMyPaintBuffer
     * This method is used to update buffer data for mypaint backend, which will be applied later
     * @param tileSize
     * @param pixmap
     */
    void updateMyPaintBuffer(const QSize& tileSize);

    bool isDirty() { return mDirty; }
    void setDirty(bool dirty) { mDirty = dirty; }
    void setPos(const QPoint& pos) { mPos = pos; }
    QPoint pos() const { return mPos; }

private:

    /// Convert 16 bit pixel format to 32 bit
    inline uint convert_from_mypaint(uint16_t value) const { return ((value*255)/(1<<15)); }

    /// Convert 32 bit pixel format to 16 bit
    inline uint16_t convert_to_mypaint(int value) const { return ((value*(1<<15))/255); }

    uint16_t  mTPixels [k_tile_dim][k_tile_dim][4];
    QImage    mCacheImg;
    bool      mCacheValid;
    bool mDirty = false;
    QPoint mPos;
};

#endif // TILE_H
