/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "bitmapimage.h"

#include <cmath>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QPainterPath>
#include "util.h"

#include "blitrect.h"
#include "tile.h"
#include "tiledbuffer.h"
#include "selectionbitmapeditor.h"

#include "bitmapeditor.h"

BitmapImage::BitmapImage()
{
    // Do we need to create the key editor here as well?
    mKeyEditor = new BitmapEditor();
}

BitmapImage::BitmapImage(const BitmapImage& a) : KeyFrame(a)
{
    mKeyEditor = new BitmapEditor(*a.editor());
}

BitmapImage::BitmapImage(const QRect& rectangle, const QColor& color)
{
    mKeyEditor = new BitmapEditor(rectangle, color);
}

BitmapImage::BitmapImage(const QPoint& topLeft, const QImage& image)
{
    mKeyEditor = new BitmapEditor(topLeft, image);
}

BitmapImage::BitmapImage(const QPoint& topLeft, const QString& path)
{
    mKeyEditor = new BitmapEditor(topLeft, path);
}

void BitmapImage::attachSelectionEditor(SelectionBitmapEditor* selectionEditor)
{
    qDebug() << "attaching editor to " << pos();
    qDebug() << "editor is" << selectionEditor;
    // mSelectionEditor.reset(selectionEditor);
}

BitmapImage::~BitmapImage()
{
    qDebug() << "BitmapImage::deinit";
}

BitmapImage& BitmapImage::operator=(const BitmapImage& a)
{
    if (this == &a)
    {
        return *this; // a self-assignment
    }

    KeyFrame::operator=(a);
    mKeyEditor = new BitmapEditor(*a.editor());

    // TODO: do we reimplement this here?
    // if (a.mSelectionEditor.get()) {
    //     SelectionBitmapEditor* editor = static_cast<SelectionBitmapEditor*>(a.mSelectionEditor.get());
    //     mSelectionEditor.reset(new SelectionBitmapEditor(*editor));
    // } else {
    //     mSelectionEditor.reset();
    // }
    modification();
    return *this;
}

BitmapImage* BitmapImage::clone() const
{
    BitmapImage* b = new BitmapImage(*this);
    b->setFileName(""); // don't link to the file of the source bitmap image

    const bool validKeyFrame = !fileName().isEmpty();
    if (validKeyFrame && !isLoaded())
    {
        // This bitmapImage is temporarily unloaded.
        // since it's not in the memory, we need to copy the linked png file to prevent data loss.
        QFileInfo finfo(fileName());
        Q_ASSERT(finfo.isAbsolute());
        Q_ASSERT(QFile::exists(fileName()));

        QString newFileName = QString("%1/%2-%3.%4")
            .arg(finfo.canonicalPath())
            .arg(finfo.completeBaseName())
            .arg(uniqueString(12))
            .arg(finfo.suffix());
        b->setFileName(newFileName);

        bool ok = QFile::copy(fileName(), newFileName);
        Q_ASSERT(ok);
        qDebug() << "COPY>" << fileName();
    }
    return b;
}

void BitmapImage::loadFile()
{
    if (!fileName().isEmpty() && !isLoaded())
    {
        editor()->loadFile();
    }
}

void BitmapImage::unloadFile()
{
    if (isModified() == false)
    {
        editor()->unloadFile();
    }
}

bool BitmapImage::isLoaded() const
{
    return editor()->isLoaded();
}

quint64 BitmapImage::memoryUsage()
{
    return editor()->memoryUsage();
}

void BitmapImage::paintImage(QPainter& painter)
{
    editor()->paintImage(painter);
}

void BitmapImage::paintImage(QPainter& painter, QImage& image, QRect sourceRect, QRect destRect)
{
    editor()->paintImage(painter, image, sourceRect, destRect);
}

QImage* BitmapImage::image()
{
    return editor()->image();
}

BitmapImage BitmapImage::copy()
{
    return BitmapImage(editor()->topLeft(), *editor()->image());
}

BitmapImage BitmapImage::copy(QRect rectangle)
{
    if (rectangle.isEmpty() || editor()->bounds().isEmpty()) return BitmapImage();

    QRect intersection2 = rectangle.translated(-editor()->bounds().topLeft());

    BitmapImage result(rectangle.topLeft(), editor()->image()->copy(intersection2));
    return result;
}

void BitmapImage::paste(BitmapImage* bitmapImage, QPainter::CompositionMode cm)
{
    editor()->paste(*bitmapImage->editor(), cm);

    modification();
}

void BitmapImage::paste(const TiledBuffer* tiledBuffer, QPainter::CompositionMode cm)
{
    editor()->paste(tiledBuffer, cm);

    modification();
}

void BitmapImage::moveTopLeft(QPoint point)
{
    editor()->moveTopLeft(point);
    // Size is unchanged so there is no need to update mBounds
    modification();
}

void BitmapImage::transform(QRect newBoundaries, bool smoothTransform)
{
    editor()->transform(newBoundaries, smoothTransform);

    modification();
}

BitmapImage BitmapImage::transformed(QRect selection, QTransform transform, bool smoothTransform)
{

    // TODO: Maybe we can move these into the selection editor?
    // Q_ASSERT(!selection.isEmpty());

    // BitmapImage selectedPart = copy(selection);

    // // Get the transformed image
    // QImage transformedImage;
    // if (smoothTransform)
    // {
    //     transformedImage = selectedPart.image()->transformed(transform, Qt::SmoothTransformation);
    // }
    // else
    // {
    //     transformedImage = selectedPart.image()->transformed(transform);
    // }
    // return BitmapImage(transform.mapRect(selection).normalized().topLeft(), transformedImage);
}

BitmapImage BitmapImage::transformed(QRect newBoundaries, bool smoothTransform)
{
    // TODO: Maybe we can move these into the selection editor?
    // BitmapImage transformedImage(newBoundaries, QColor(0, 0, 0, 0));
    // QPainter painter(transformedImage.image());
    // painter.setRenderHint(QPainter::SmoothPixmapTransform, smoothTransform);
    // newBoundaries.moveTopLeft(QPoint(0, 0));
    // painter.drawImage(newBoundaries, *image());
    // painter.end();
    // return transformedImage;
}

/** Update image bounds.
 *
 *  @param[in] newBoundaries the new bounds
 *
 *  Sets this image's bounds to rectangle.
 *  Modifies mBounds and crops mImage.
 */
void BitmapImage::updateBounds(QRect newBoundaries)
{
    if (editor()->updateBounds(newBoundaries)) {
        modification();
    }
}

void BitmapImage::extend(const QPoint &p)
{
    if (!editor()->bounds().contains(p))
    {
        extend(QRect(p, QSize(1, 1)));
    }
}

void BitmapImage::extend(QRect rectangle)
{
    if (editor()->extend(rectangle)) {
        modification();
    }
}

/** Updates the bounds after a drawImage operation with the composition mode cm.
 *
 *  @param[in] source The source image used for the drawImage call.
 *  @param[in] cm The composition mode that will be used for the draw image
 *
 *  @see BitmapImage::setCompositionModeBounds(BitmapImage, QPainter::CompositionMode)
 */
void BitmapImage::setCompositionModeBounds(BitmapImage *source, QPainter::CompositionMode cm)
{
    if (source)
    {
        editor()->setCompositionModeBounds(source->editor()->bounds(), source->isMinimallyBounded(), cm);
    }
}

/** Updates the bounds after a draw operation with the composition mode cm.
 *
 * @param[in] sourceBounds The bounds of the source used for drawcall.
 * @param[in] isSourceMinBounds Is sourceBounds the minimal bounds for the source image
 * @param[in] cm The composition mode that will be used for the draw image
 *
 * For a call to draw image of a QPainter (initialized with mImage) with an argument
 * of source, this function intelligently calculates the bounds. It will attempt to
 * preserve minimum bounds based on the composition mode.
 *
 * This works baed on the principle that some minimal bounds can be determined
 * solely by the minimal bounds of this and source, depending on the value of cm.
 * Some composition modes only expand, or have no affect on the bounds.
 *
 * @warning The draw operation described by the arguments of this
 *          function needs to be called after this function is run,
 *          or the bounds will be out of sync. If mBounds is null,
 *          no draw operation needs to be performed.
 */
void BitmapImage::setCompositionModeBounds(QRect sourceBounds, bool isSourceMinBounds, QPainter::CompositionMode cm)
{
    editor()->setCompositionModeBounds(sourceBounds, isSourceMinBounds, cm);

    modification();
}

/** Removes any transparent borders by reducing the boundaries.
 *
 *  This function reduces the bounds of an image until the top and
 *  bottom rows, and the left and right columns of pixels each
 *  contain at least one pixel with a non-zero alpha value
 *  (i.e. non-transparent pixel). Both mBounds and
 *  the size of #mImage are updated.
 *
 *  @pre mBounds.size() == mImage->size()
 *  @post Either the first and last rows and columns all contain a
 *        pixel with alpha > 0 or mBounds.isEmpty() == true
 *  @post isMinimallyBounded() == true
 */
void BitmapImage::autoCrop()
{
    editor()->autoCrop();

    modification();
}

QRgb BitmapImage::pixel(int x, int y)
{
    return pixel(QPoint(x, y));
}

QRgb BitmapImage::pixel(QPoint p)
{
    return editor()->pixel(p);
}

void BitmapImage::setPixel(int x, int y, QRgb color)
{
    setPixel(QPoint(x, y), color);
}

void BitmapImage::setPixel(QPoint p, QRgb color)
{
    editor()->setPixel(p, color);
    modification();
}

void BitmapImage::fillNonAlphaPixels(const QRgb color)
{
    if (editor()->bounds().isEmpty()) { return; }

    BitmapImage fill(editor()->bounds(), color);
    paste(&fill, QPainter::CompositionMode_SourceIn);
}

void BitmapImage::drawLine(QPointF P1, QPointF P2, QPen pen, QPainter::CompositionMode cm, bool antialiasing)
{
    editor()->drawLine(P1, P2, pen, cm, antialiasing);
    modification();
}

void BitmapImage::drawRect(QRectF rectangle, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing)
{
    editor()->drawRect(rectangle, pen, brush, cm, antialiasing);
    modification();
}

void BitmapImage::drawEllipse(QRectF rectangle, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing)
{
    editor()->drawEllipse(rectangle, pen, brush, cm, antialiasing);
    modification();
}

void BitmapImage::drawPath(QPainterPath path, QPen pen, QBrush brush,
                           QPainter::CompositionMode cm, bool antialiasing)
{
    editor()->drawPath(path, pen, brush, cm, antialiasing);
    modification();
}

Status BitmapImage::writeFile(const QString& filename)
{
    if (!editor()->isLoaded())
    {
        bool b = editor()->image()->save(filename);
        return (b) ? Status::OK : Status::FAIL;
    }

    if (editor()->bounds().isEmpty())
    {
        QFile f(filename);
        if(f.exists())
        {
            bool b = f.remove();
            return (b) ? Status::OK : Status::FAIL;
        }
        return Status::SAFE;
    }
    return Status::SAFE;
}

void BitmapImage::clear()
{
    editor()->clear();
    modification();
}

QRgb BitmapImage::constScanLine(int x, int y) const
{
    editor()->constScanLine(x, y);
}

void BitmapImage::scanLine(int x, int y, QRgb color)
{
    editor()->scanLine(x, y, color);
}

void BitmapImage::clear(QRect rectangle)
{
    editor()->clear(rectangle);
    modification();
}
