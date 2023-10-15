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

#include <QPainter>
#include <QDebug>
#include <QtMath>

#include "mpsurface.h"
#include "mphandler.h"
#include "qrect.h"

static void freeTiledSurface(MyPaintSurface *surface)
{
    MPSurface *self = reinterpret_cast<MPSurface*>(surface);
    mypaint_tiled_surface_destroy(self);

    free(self->tileBuffer);
    free(self->nullTile);
    free(self);
}

static void onTileRequestStart(MyPaintTiledSurface *tiledSurface, MyPaintTileRequest *request)
{
    MPSurface *self = static_cast<MPSurface*>(tiledSurface);

    const int tx = request->tx;
    const int ty = request->ty;
    uint16_t *tilePointer = nullptr;

    if (tx > self->getTilesWidth()) {
        self->setTilesWidth(tx);
    }

    if (ty > self->getTilesHeight()) {
        self->setTilesHeight(ty);
    }

    if (self->getTilesWidth() == 0 || self->getTilesHeight() == 0) {

        // Give it a tile which we will ignore writes to
        tilePointer = self->nullTile;
    } else {

        MPTile* tile = self->getTileFromIdx(tx,ty);
        tilePointer = tile ? tile->bits(false) : nullptr;
    }

    // here we send our buffer data to mypaint
    request->buffer = tilePointer;
}

static void onTileRequestEnd(MyPaintTiledSurface *tiledSurface, MyPaintTileRequest *request)
{
    MPSurface *self = static_cast<MPSurface*>(tiledSurface);

    const int tx = request->tx;
    const int ty = request->ty;

    MPTile* tile = self->getTileFromIdx(tx,ty);
    if (tile) {
        tile->updateCache();
    }

    self->onUpdateTile(self, tile);
}

MPSurface::MPSurface(MPHandler* parentHandler, QSize size)
{
    // MPSurface vfuncs
    parent.destroy = freeTiledSurface;
    mParentHandler = parentHandler;

    resetSurface(size);


    mypaint_tiled_surface_init(static_cast<MyPaintTiledSurface*>(this), onTileRequestStart, onTileRequestEnd);
}

MPSurface::~MPSurface()
{
}

void MPSurface::onUpdateTile(MPSurface *surface, MPTile *tile)
{
    emit mParentHandler->tileUpdated(surface, tile);
}

void MPSurface::onNewTile(MPSurface *surface, MPTile *tile)
{
    emit mParentHandler->tileAdded(surface, tile);
}

void MPSurface::onClearedSurface(MPSurface *surface)
{
    emit mParentHandler->surfaceCleared(surface);
}

void MPSurface::onClearTile(MPSurface *surface, QRect tileRect)
{
    emit mParentHandler->tileCleared(surface, tileRect);
}

void MPSurface::loadTile(const QImage& image, const QPoint& topLeft, MPTile* tile)
{
    QPainter painter(&tile->image());

    painter.translate(-tile->pos());
    painter.drawImage(topLeft, image);
    painter.end();

    tile->updateMyPaintBuffer(tile->boundingRect().size());
}

void MPSurface::loadTiles(const QList<std::shared_ptr<QImage> >& images, const QList<QPoint>& positions)
{
    if (images.isEmpty()) { return; }

    for (int i = 0; i < images.count(); i++) {

        const QImage& image = *images.at(i).get();
        const QPoint pos = positions.at(i);

        // Optimization : Fully transparent (empty) tiles
        // don't need to be created.
        if (!isFullyTransparent(image)) {

            MPTile *tile = getTileFromPos(pos);
            tile->setImage(image);
            onUpdateTile(this, tile);
        }
    }
}

/**
 * @brief MPSurface::saveSurface
 * Saves content of surface to an image and exports it to preferred destinaton
 * @param path
 */
void MPSurface::saveSurface(const QString path)
{
    QImage paintedImage(size(), QImage::Format_ARGB32_Premultiplied);
    paintedImage.fill(Qt::transparent);

    QPainter painter(&paintedImage);
    painter.translate(QPoint(mWidth/2,mHeight/2));

    QHashIterator<TileIndex, MPTile*> cuTiles(mTilesHash);
    while (cuTiles.hasNext()) {
        cuTiles.next();
        const QImage& pix = cuTiles.value()->image();
        const QPoint& pos = cuTiles.value()->pos();
        painter.drawImage(pos, pix);
    }
    painter.end();

    paintedImage.save(path);
}

void MPSurface::setSize(QSize size)
{
    free(tileBuffer);
    free(nullTile);

    resetSurface(size);
}

QSize MPSurface::size()
{
    return QSize(mWidth, mHeight);
}

void MPSurface::clear()
{
    QHashIterator<TileIndex, MPTile*> i(mTilesHash);

    while (i.hasNext()) {
        i.next();
        MPTile *tile = i.value();
        if (tile)
        {
            mTilesHash.remove(i.key());
            delete tile;
        }
    }

    onClearedSurface(this);
}

void MPSurface::clearTile(MPTile* tile)
{
    QRect tileRect = QRect(tile->pos(), tile->boundingRect().size());
    tile->clear();
    delete tile;

    onClearTile(this, tileRect);
}

int MPSurface::getTilesWidth()
{
    return mTilesWidth;
}

int MPSurface::getTilesHeight()
{
    return mTilesHeight;
}

int MPSurface::getWidth()
{
    return mWidth;
}

int MPSurface::getHeight()
{
    return mHeight;
}

void MPSurface::resetNullTile()
{
    memset(nullTile, 0, static_cast<size_t>(tile_size));
}

void MPSurface::resetSurface(QSize size)
{
    mWidth = size.width();
    mHeight = size.height();

    assert(mWidth > 0);
    assert(mHeight > 0);

    const int tileSizePixels = MYPAINT_TILE_SIZE;

    const int tilesWidth = static_cast<int>(ceil(static_cast<float>(mWidth) / tileSizePixels));
    const int tilesHeight = static_cast<int>(ceil(static_cast<float>(mHeight) / tileSizePixels));

    const size_t tileSize = tileSizePixels * tileSizePixels * 4 * sizeof(uint16_t);
    const size_t bufferSize = static_cast<size_t>(tilesWidth * tilesHeight) * tileSize;

    assert(tileSizePixels*tilesWidth >= mWidth);
    assert(tileSizePixels*tilesHeight >= mHeight);
    assert(bufferSize >= static_cast<size_t>(mWidth*mHeight)*4*sizeof(uint16_t));

    uint16_t* buffer = static_cast<uint16_t*>(malloc(bufferSize));
    if (!buffer)
        fprintf(stderr, "CRITICAL: unable to allocate enough memory: %lu bytes", bufferSize);

    memset(buffer, 255, bufferSize);

    this->tile_size = tileSize;
    tileBuffer = buffer;
    nullTile = static_cast<uint16_t*>(malloc(tile_size));
    mTilesWidth = tilesWidth;
    mTilesHeight = tilesHeight;

    resetNullTile();
}

bool MPSurface::isFullyTransparent(QImage image)
{
    if (!image.hasAlphaChannel()) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    for (int x = 0 ; x < image.width() ; x++) {
        for (int y = 0 ; y < image.height() ; y++) {
            const QRgb pixelColor = *(reinterpret_cast<const QRgb*>(image.constScanLine(x))+y);

            if (qAlpha(pixelColor) != 0) {
                return false;
            }
        }
    }
    return true;
}

MPTile* MPSurface::getTileFromPos(const QPoint& pos)
{
    TileIndex tileIndex = getTileIndex(pos);
    return getTileFromIdx(tileIndex.x, tileIndex.y);
}

MPTile* MPSurface::getTileFromIdx(int tileX, int tileY)
{

    TileIndex tileIndex;
    tileIndex.x = tileX;
    tileIndex.y = tileY;

    MPTile* selectedTile = nullptr;

    selectedTile = mTilesHash.value(tileIndex, nullptr);

    if (!selectedTile) {
        // Time to allocate it, update table:
        selectedTile = new MPTile();
        mTilesHash.insert(tileIndex, selectedTile);

        QPoint tilePos (getTilePos(tileIndex));
        selectedTile->setPos(tilePos);

        onNewTile(this, selectedTile);
    } else {
        onUpdateTile(this, selectedTile);
    }

    return selectedTile;
}

inline QPoint MPSurface::getTilePos(const TileIndex& idx) const
{
    return QPoint(qRound(MYPAINT_TILE_SIZE*static_cast<qreal>(idx.x)), qRound(MYPAINT_TILE_SIZE*static_cast<qreal>(idx.y)));
}

inline TileIndex MPSurface::getTileIndex(const QPoint& pos) const
{
    return { qRound(static_cast<qreal>(pos.x())/MYPAINT_TILE_SIZE), qRound(static_cast<qreal>(pos.y())/MYPAINT_TILE_SIZE) };
}
