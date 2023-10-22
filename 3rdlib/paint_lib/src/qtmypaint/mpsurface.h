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

#ifndef MPSURFACE_H
#define MPSURFACE_H

#include <memory>

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <QImage>
#include <QHash>

extern "C" {
#include "mypaint-glib-compat.h"
#include "mypaint-tiled-surface.h"
}

#include "tileindex.h"
#include "mpbrush.h"
#include "mptile.h"

class MPHandler;

class MPSurface : public MyPaintTiledSurface
{
public:
    MPSurface(MPHandler* parentHandler, QSize size);
    ~MPSurface();

    uint16_t *tileBuffer; // Stores tiles in a linear chunk of memory (16bpc RGBA)
    uint16_t *nullTile; // Single tile that we hand out and ignore writes to

    int getTilesWidth();
    int getTilesHeight();
    int getWidth();
    int getHeight();

    void loadTile(const QImage& image, const QPoint& topLeft, MPTile* tile);
    void loadTiles(const QList<std::shared_ptr<QImage> > &images, const QList<QPoint> &positions);

    void setTilesWidth(int newWidth) { mTilesWidth = newWidth; }
    void setTilesHeight(int newHeight) { mTilesHeight = newHeight; }

    MPTile* getTileFromPos(const QPoint& pos);
    MPTile* getTileFromIdx(int tileX, int tileY);

    inline QPoint getTilePos(const TileIndex& idx) const
    {
        return QPoint(qRound(MYPAINT_TILE_SIZE*static_cast<qreal>(idx.x)), qRound(MYPAINT_TILE_SIZE*static_cast<qreal>(idx.y)));
    }

    TileIndex getTileIndex(const QPoint& pos) const
    {
        return { qRound(static_cast<qreal>(pos.x())/MYPAINT_TILE_SIZE), qRound(static_cast<qreal>(pos.y())/MYPAINT_TILE_SIZE) };
    }

    TileIndex getTileIndex(const int x, const int y) const
    {
        return { qRound(static_cast<qreal>(x)/MYPAINT_TILE_SIZE), qRound(static_cast<qreal>(y)/MYPAINT_TILE_SIZE) };
    }

    void onUpdateTile(MPSurface *surface, MPTile *tile);
    void onNewTile(MPSurface *surface, MPTile *tile);
    void onClearTile(MPSurface *surface, QRect tileRect);
    void onClearedSurface(MPSurface *surface);

    void saveSurface(const QString path);

    void setSize(QSize size);
    QSize size();

    void clear();
    void clearTile(MPTile* tile);

private:
    void resetNullTile();
    void resetSurface(QSize size);
    bool isFullyTransparent(QImage image);

    int mTilesWidth; // width in tiles
    int mTilesHeight; // height in tiles
    int mWidth; // width in pixels
    int mHeight; // height in pixels

protected:
    QHash<TileIndex, MPTile*> mTilesHash;
    MPHandler* mParentHandler;
};

#endif // MPSURFACE_H
