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

    free(self->tile_buffer);
    free(self->null_tile);
    free(self);
}

static void onTileRequestStart(MyPaintTiledSurface *tiled_surface, MyPaintTileRequest *request)
{
    MPSurface *self = static_cast<MPSurface*>(tiled_surface);

    const int tx = request->tx;
    const int ty = request->ty;
    uint16_t *tile_pointer = nullptr;

    if (tx > self->getTilesWidth()) {
        self->setTilesWidth(tx);
    }

    if (ty > self->getTilesHeight()) {
        self->setTilesHeight(ty);
    }

    if (self->getTilesWidth() == 0 || self->getTilesHeight() == 0) {

        // Give it a tile which we will ignore writes to
        tile_pointer = self->null_tile;
    } else {

        MPTile* tile = self->getTileFromIdx(QPoint(tx,ty));
        tile_pointer = tile ? tile->bits(false) : nullptr;
    }

    // here we send our buffer data to mypaint
    request->buffer = tile_pointer;
}

static void onTileRequestEnd(MyPaintTiledSurface *tiled_surface, MyPaintTileRequest *request)
{
    MPSurface *self = static_cast<MPSurface*>(tiled_surface);

    const int tx = request->tx;
    const int ty = request->ty;

    MPTile* tile = self->getTileFromIdx(QPoint(tx,ty));
    if (tile) {
        tile->updateCache();
    }

    self->onUpdateTile(self, tile);
}

MPSurface::MPSurface(MPHandler* parentHandler, QSize size)
{
    // MPSurface vfuncs
    this->parent.destroy = freeTiledSurface;
    this->mParentHandler = parentHandler;

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

void MPSurface::onClearTile(MPSurface *surface, MPTile *tile)
{
    emit mParentHandler->tileCleared(surface, tile);
}

void MPSurface::loadTile(const QImage& image, const QPoint& topLeft, MPTile* tile)
{
    QImage paintTo(MYPAINT_TILE_SIZE,MYPAINT_TILE_SIZE, QImage::Format_ARGB32_Premultiplied);

    paintTo.fill(Qt::transparent);
    QPainter painter(&paintTo);

    painter.save();
    painter.translate(-tile->pos());
    painter.drawImage(topLeft, image);
    painter.restore();
    painter.end();

    tile->setImage(paintTo);
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
            this->onUpdateTile(this, tile);
        }
    }
}

void MPSurface::loadImage(const QImage &image, const QPoint topLeft)
{
    QImage paintTo(MYPAINT_TILE_SIZE,MYPAINT_TILE_SIZE, QImage::Format_ARGB32_Premultiplied);
    QList<QPoint> touchedPoints = findCorrespondingTiles(QRect(topLeft, image.size()));

    setSize(image.size());

    for (int point = 0; point < touchedPoints.count(); point++) {

        const QPoint touchedPoint = touchedPoints.at(point);

        paintTo.fill(Qt::transparent);
        QPainter painter(&paintTo);

        painter.save();
        painter.translate(-touchedPoint);
        painter.drawImage(topLeft, image);
        painter.restore();
        painter.end();

        MPTile *tile = getTileFromPos(touchedPoint);
        tile->setImage(paintTo);
        tile->updateMyPaintBuffer(tile->boundingRect().size());
    }
}

/**
 * @brief MPSurface::clearArea
 * Clears surface area at the given rectangle
 * @param bounds
 */
void MPSurface::clearArea(const QRect& bounds)
{
    QImage paintTo(MYPAINT_TILE_SIZE,MYPAINT_TILE_SIZE, QImage::Format_ARGB32_Premultiplied);

    QList<QPoint> touchedPoints = findCorrespondingTiles(bounds);

    for (int point = 0; point < touchedPoints.count(); point++) {

        const QPoint touchedPoint = touchedPoints.at(point);

        MPTile *tile = getTileFromPos(touchedPoint);

        paintTo.fill(Qt::transparent);
        QPainter painter(&paintTo);

        painter.save();
        painter.translate(-touchedPoint);

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(tile->pos(), tile->image());

        painter.setCompositionMode(QPainter::CompositionMode_Clear);
        painter.fillRect(bounds, Qt::transparent);
        painter.restore();
        painter.end();

        if (isFullyTransparent(paintTo)) {
            clearTile(tile);
        } else {
            // tile is updated here and the mypaint buffer is updated
            tile->setImage(paintTo);
            tile->updateMyPaintBuffer(tile->boundingRect().size());
        }
    }
}

/**
 * @brief MPSurface::drawImageAt
 * Draw an image on top of the existing surface
 * @param image The image you want to use
 * @param topLeft
 */
void MPSurface::drawImageAt(const QImage& image, const QPoint topLeft)
{
    QImage paintTo(MYPAINT_TILE_SIZE,MYPAINT_TILE_SIZE, QImage::Format_ARGB32_Premultiplied);
    QImage transparenPix(MYPAINT_TILE_SIZE,MYPAINT_TILE_SIZE, QImage::Format_ARGB32_Premultiplied);
    transparenPix.fill(Qt::transparent);

    QList<QPoint> touchedPoints = findCorrespondingTiles(QRect(topLeft, image.size()));

    for (int point = 0; point < touchedPoints.count(); point++) {

        const QPoint touchedPoint = touchedPoints.at(point);

        MPTile *tile = getTileFromPos(touchedPoint);

        paintTo.fill(Qt::transparent);
        QPainter painter(&paintTo);

        painter.save();
        painter.translate(-touchedPoint);

        painter.drawImage(tile->pos(), tile->image());
        painter.drawImage(topLeft, image);
        painter.restore();
        painter.end();

        if (isFullyTransparent(paintTo)) {
            clearTile(tile);
        } else {
            // tile is updated here and the mypaint buffer is updated
            tile->setImage(paintTo);
            tile->updateMyPaintBuffer(tile->boundingRect().size());
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
    painter.translate(QPoint(width/2,height/2));

    QHashIterator<QString, MPTile*> cuTiles(m_Tiles);
    while (cuTiles.hasNext()) {
        cuTiles.next();
        const QImage& pix = cuTiles.value()->image();
        const QPoint& pos = cuTiles.value()->pos();
        painter.drawImage(pos, pix);
    }
    painter.end();

    paintedImage.save(path);
}

/**
 * @brief MPSurface::findCorrespondingTiles
 * Finds corresponding tiles for the given rectangle
 * @param rect
 * @return A list of points that matches the tile coordinate system.
 */
QList<QPoint> MPSurface::findCorrespondingTiles(const QRect& rect)
{
    QRect searchRect = rect;
    const int tileWidth = MYPAINT_TILE_SIZE;
    const int tileHeight = MYPAINT_TILE_SIZE;
    const float imageWidth = searchRect.width();
    const float imageHeight = searchRect.height();
    const int nbTilesOnWidth = static_cast<int>(ceil(imageWidth / tileWidth));
    const int nbTilesOnHeight = static_cast<int>(ceil(imageHeight / tileHeight));

    QList<QPoint> points;

    QList<QPoint> corners;
    const QPoint& cornerOffset = QPoint(tileWidth, tileHeight);

    corners.append({searchRect.topLeft(), searchRect.topRight(), searchRect.bottomLeft(), searchRect.bottomRight()});
    for (int h=0; h < nbTilesOnHeight; h++) {
        for (int w=0; w < nbTilesOnWidth; w++) {

            const QPoint tilePos = getTilePos(QPoint(w,h));
            for (int i = 0; i < corners.count(); i++) {
                QPoint movedPos = getTileIndex(corners[i]-cornerOffset);
                movedPos = getTilePos(movedPos)+tilePos;

                if (points.contains(movedPos)) {
                    continue;
                }

                if (QRect(movedPos, QSize(tileWidth,tileHeight)).intersects(searchRect)) {
                    points.append(movedPos);
                }
            }
        }
    }
    return points;
}

void MPSurface::setSize(QSize size)
{
    free(this->tile_buffer);
    free(this->null_tile);

    resetSurface(size);
}

QSize MPSurface::size()
{
    return QSize(width, height);
}

void MPSurface::clear()
{
    QHashIterator<QString, MPTile*> i(m_Tiles);

    if (!m_Tiles.isEmpty()) {
        m_Tiles.clear();
    }

    while (i.hasNext()) {
        i.next();
        MPTile *tile = i.value();
        if (tile)
        {
            // Clear the content of the tile
            //
            tile->clear();
            m_Tiles.remove(i.key());
        }
    }

    this->onClearedSurface(this);
}

void MPSurface::clearTile(MPTile* tile)
{
    QHashIterator<QString, MPTile*> i(m_Tiles);

    while (i.hasNext()) {
        i.next();
        MPTile *itValue = i.value();
        if (tile == itValue)
        {
            // Clear the content of the tile
            //
            tile->clear();
            m_Tiles.remove(i.key());
        }
    }
    this->onClearTile(this, tile);
}

int MPSurface::getTilesWidth()
{
    return this->tiles_width;
}

int MPSurface::getTilesHeight()
{
    return this->tiles_height;
}

int MPSurface::getWidth()
{
    return this->width;
}

int MPSurface::getHeight()
{
    return this->height;
}

void MPSurface::resetNullTile()
{
    memset(this->null_tile, 0, static_cast<size_t>(this->tile_size));
}

void MPSurface::resetSurface(QSize size)
{
    width = size.width();
    height = size.height();

    assert(width > 0);
    assert(height > 0);

    const int tile_size_pixels = MYPAINT_TILE_SIZE;

    const int tiles_width = static_cast<int>(ceil(static_cast<float>(width) / tile_size_pixels));
    const int tiles_height = static_cast<int>(ceil(static_cast<float>(height) / tile_size_pixels));

    const size_t tile_size = tile_size_pixels * tile_size_pixels * 4 * sizeof(uint16_t);
    const size_t buffer_size = static_cast<size_t>(tiles_width * tiles_height) * tile_size;

    assert(tile_size_pixels*tiles_width >= width);
    assert(tile_size_pixels*tiles_height >= height);
    assert(buffer_size >= static_cast<size_t>(width*height)*4*sizeof(uint16_t));

    uint16_t* buffer = static_cast<uint16_t*>(malloc(buffer_size));
    if (!buffer)
        fprintf(stderr, "CRITICAL: unable to allocate enough memory: %lu bytes", buffer_size);

    memset(buffer, 255, buffer_size);

    this->tile_buffer = buffer;
    this->tile_size = tile_size;
    this->null_tile = static_cast<uint16_t*>(malloc(tile_size));
    this->tiles_width = tiles_width;
    this->tiles_height = tiles_height;

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

void MPSurface::refreshSurface()
{
    for (MPTile* tile : m_Tiles) {
        this->onUpdateTile(this, tile);
    }
}

MPTile* MPSurface::getTileFromPos(const QPoint& pos)
{
    return getTileFromIdx(getTileIndex(pos));
}

MPTile* MPSurface::getTileFromIdx(const QPoint& idx)
{

    MPTile* selectedTile = nullptr;

    // convert point to strign to check index
    // it's faster to iterate qstring than a qpoint...
    QString idxString = QString::number(idx.x())+"_"+QString::number(idx.y());
    selectedTile = m_Tiles.value(idxString, nullptr);

    if (!selectedTile) {
        // Time to allocate it, update table:
        selectedTile = new MPTile();
        m_Tiles.insert(idxString, selectedTile);

        QPoint tilePos (getTilePos(idx));
        selectedTile->setPos(tilePos);

        this->onNewTile(this, selectedTile);
    } else {
        this->onUpdateTile(this, selectedTile);
    }

    return selectedTile;
}

inline QPoint MPSurface::getTilePos(const QPoint& idx)
{
    return QPoint(qRound(MYPAINT_TILE_SIZE*static_cast<qreal>(idx.x())), qRound(MYPAINT_TILE_SIZE*static_cast<qreal>(idx.y())));
}

inline QPoint MPSurface::getTileIndex(const QPoint& pos)
{
    return QPoint(qRound(static_cast<qreal>(pos.x())/MYPAINT_TILE_SIZE), qRound(static_cast<qreal>(pos.y())/MYPAINT_TILE_SIZE));
}

inline QPointF MPSurface::getTileFIndex(const QPoint& pos)
{
    return QPointF(static_cast<qreal>(pos.x())/MYPAINT_TILE_SIZE, static_cast<qreal>(pos.y())/MYPAINT_TILE_SIZE);
}
