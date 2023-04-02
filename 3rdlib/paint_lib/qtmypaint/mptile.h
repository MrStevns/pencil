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
// NOTE that the uint16_t data (premul RGB 15 bits) is transfered in premul
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

    const QImage& image() const { return m_cache_img; }

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

    bool isDirty() { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }
    void setPos(const QPoint& pos) { m_pos = pos; }
    QPoint pos() const { return m_pos; }

private:

    uint16_t  t_pixels [k_tile_dim][k_tile_dim][4];
    QImage    m_cache_img;
    bool      m_cache_valid;
    bool m_dirty = false;
    QPoint m_pos;
};

#endif // TILE_H
