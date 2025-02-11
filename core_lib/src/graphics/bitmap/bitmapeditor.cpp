#include "bitmapeditor.h"

#include "tiledbuffer.h"
#include "tile.h"

#include <QPixmap>
#include <QDebug>

BitmapEditor::BitmapEditor() : KeyFrameEditor()
{

}

BitmapEditor::BitmapEditor(const BitmapEditor& editor) : KeyFrameEditor(editor)
{
    mBounds = editor.mBounds;
    mMinBound = editor.mMinBound;
    mEnableAutoCrop = editor.mEnableAutoCrop;
    mOpacity = editor.mOpacity;
    mImage = editor.mImage;
}

BitmapEditor::BitmapEditor(const QRect& rectangle, const QColor& color)
{
    mBounds = rectangle;
    mImage = QImage(mBounds.size(), QImage::Format_ARGB32_Premultiplied);
    mImage.fill(color.rgba());
    mMinBound = false;
}

BitmapEditor::BitmapEditor(const QPoint& topLeft, const QImage& image)
{
    mBounds = QRect(topLeft, image.size());
    mMinBound = true;
    if (image.format() != QImage::Format_ARGB32_Premultiplied) {
        mImage = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    } else {
        mImage = image;
    }
}

BitmapEditor::BitmapEditor(const QPoint& topLeft, const QString& path)
{
    setFileName(path);
    mImage = QImage();

    mBounds = QRect(topLeft, QSize(-1, 0));
    mMinBound = true;
    setModified(false);
}

BitmapEditor::~BitmapEditor()
{
    qDebug() << "BitmapEditor::deinit";
}

void BitmapEditor::loadFile()
{
    mImage = QImage(fileName()).convertToFormat(QImage::Format_ARGB32_Premultiplied);
    mBounds.setSize(mImage.size());
    mMinBound = false;
}

void BitmapEditor::unloadFile()
{
    mImage = QImage();
}

bool BitmapEditor::isLoaded() 
{
    return mImage.width() == mBounds.width();
}

void BitmapEditor::transform(QRect newBoundaries, bool smoothTransform)
{
    mBounds = newBoundaries;
    newBoundaries.moveTopLeft(QPoint(0, 0));
    QImage newImage(mBounds.size(), QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&newImage);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, smoothTransform);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(newImage.rect(), QColor(0, 0, 0, 0));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(newBoundaries, *image());
    painter.end();
    mImage = newImage;
}

bool BitmapEditor::updateBounds(const QRect& newBoundaries)
{
    // Check to make sure changes actually need to be made
    if (mBounds == newBoundaries) return false;

    QImage newImage(newBoundaries.size(), QImage::Format_ARGB32_Premultiplied);
    newImage.fill(Qt::transparent);
    if (!newImage.isNull())
    {
        QPainter painter(&newImage);
        painter.drawImage(mBounds.topLeft() - newBoundaries.topLeft(), mImage);
        painter.end();
    }
    mImage = newImage;
    mBounds = newBoundaries;
    mMinBound = false;

    return true;
}

bool BitmapEditor::extend(QRect rectangle)
{
    if (rectangle.width() <= 0) rectangle.setWidth(1);
    if (rectangle.height() <= 0) rectangle.setHeight(1);

    if (!mBounds.contains(rectangle)) {
        const QRect& newBoundaries = mBounds.united(rectangle).normalized();
        QImage newImage(newBoundaries.size(), QImage::Format_ARGB32_Premultiplied);
        newImage.fill(Qt::transparent);
        if (!newImage.isNull())
        {
            QPainter painter(&newImage);
            painter.drawImage(mBounds.topLeft() - newBoundaries.topLeft(), *image());
            painter.end();
        }
        mImage = newImage;
        mBounds = newBoundaries;

        return true;
    }

    return false;
}

BitmapEditor BitmapEditor::copyArea(const QRect& rect, const QPolygon& clipToPolygon) const
{
    QRect newRect = rect.normalized();
    BitmapEditor copyEditor = BitmapEditor(newRect, Qt::transparent);

    // // This creates a copy of the current image and adds it to copyEditor
    QPainter painter(copyEditor.image());
    painter.translate(-newRect.topLeft());
    painter.setRenderHint(QPainter::Antialiasing);
    if (clipToPolygon.count() > 0) {
        QPainterPath clipPath;
        clipPath.addPolygon(clipToPolygon);
        painter.setClipPath(clipPath);
        painter.setClipping(true);
    }
    painter.drawImage(mBounds.topLeft(), mImage);
    painter.end();

    return copyEditor;
}

BitmapEditor BitmapEditor::transformed(const QRect& selection, const QTransform& transform, bool smoothTransform) const
{
    const BitmapEditor& selectedEditor = copyArea(selection, QPolygon());

    const QImage& transformedImage = selectedEditor.constImage()->transformed(transform, smoothTransform ? Qt::SmoothTransformation : Qt::FastTransformation);
    return BitmapEditor(transform.mapRect(selection).normalized().topLeft(), transformedImage);
}

BitmapEditor BitmapEditor::transformed(const QPolygon& selection, const QTransform& transform, bool smoothTransform) const
{
    const QRectF& boundingRect = selection.boundingRect();
    BitmapEditor selectedEditor = copyArea(selection.boundingRect(), selection);

    // QTransform trueMatrix = QImage::trueMatrix(transform, selectedEditor.width(), selectedEditor.height());
    const QRectF& imageRect = transform.mapRect(QRectF(selectedEditor.bounds()));
    const QRect& alignedRect = imageRect.toAlignedRect();

    QImage sizedImage = QImage(alignedRect.size(), QImage::Format_ARGB32_Premultiplied);
    sizedImage.fill(Qt::transparent);

    QPainter imagePainter(&sizedImage);

    const QPointF& oldCenter = QPointF(static_cast<qreal>(selectedEditor.width()*0.5),
                             static_cast<qreal>(selectedEditor.height())*0.5);

    const QPointF& transformedTopLeft = imageRect.topLeft();
    const QPointF& newCenter = imageRect.center() - transformedTopLeft;

    QTransform centeredTransform;

    // Move origin to the center of the initial selection rect
    centeredTransform.translate(boundingRect.width()*0.5, boundingRect.height()*0.5);

    // Apply the selection transform
    centeredTransform *= transform;

    // Translate Back to the oldCenter of the
    centeredTransform.translate(-oldCenter.x(),
                                -oldCenter.y());

    if (smoothTransform) {
        imagePainter.setRenderHint(QPainter::Antialiasing);
        imagePainter.setRenderHint(QPainter::SmoothPixmapTransform);
    }
    imagePainter.setTransform(centeredTransform);

    QPointF imagePoint = QPointF(centeredTransform.inverted().map(newCenter)-oldCenter);
    // QPoint alignedPoint = QPoint(qRound(imagePoint.x()), qRound(imagePoint.y()));
    imagePainter.drawImage(imagePoint, *selectedEditor.image());

    imagePainter.end();
    return BitmapEditor(alignedRect.topLeft(), sizedImage);
}

QImage BitmapEditor::transformed2(const QPolygon& selection, const QTransform& transform, bool smoothTransform) const
{
    const QRectF& boundingRect = selection.boundingRect();
    BitmapEditor selectedEditor = copyArea(selection.boundingRect(), selection);

    // QTransform trueMatrix = QImage::trueMatrix(transform, selectedEditor.width(), selectedEditor.height());
    const QRectF& imageRect = transform.mapRect(QRectF(selectedEditor.bounds()));
    const QRect& alignedRect = imageRect.toAlignedRect();

    QImage sizedImage = QImage(alignedRect.size(), QImage::Format_ARGB32_Premultiplied);
    sizedImage.fill(Qt::transparent);

    QPainter imagePainter(&sizedImage);

    const QPointF& transformedTopLeft = imageRect.topLeft();
    const QPointF& newCenter = imageRect.center() - transformedTopLeft;

    const QPointF& offset = QPointF(static_cast<qreal>(selectedEditor.width()*0.5),
                             static_cast<qreal>(selectedEditor.height())*0.5);

    QTransform centeredTransform;

    // Move origin to the center of the initial selection rect
    centeredTransform.translate(boundingRect.width()*0.5, boundingRect.height()*0.5);

    // Apply the selection transform
    centeredTransform *= transform;

    // Translate Back to the offset of the
    centeredTransform.translate(-offset.x(),
                                -offset.y());

    if (smoothTransform) {
        imagePainter.setRenderHint(QPainter::Antialiasing);
        imagePainter.setRenderHint(QPainter::SmoothPixmapTransform);
    }
    imagePainter.setTransform(centeredTransform);

    QPointF imagePoint = QPointF(centeredTransform.inverted().map(newCenter)-offset);
    // QPoint alignedPoint = QPoint(qRound(imagePoint.x()), qRound(imagePoint.y()));
    imagePainter.drawImage(imagePoint, *selectedEditor.image());

    imagePainter.end();

    // const QImage& transformedImage = selectedEditor.constImage()->transformed(transform, smoothTransform ? Qt::SmoothTransformation : Qt::FastTransformation);
    // qDebug() << "image size: " << transform.mapRect(QRectF(selection.boundingRect())).normalized().size();
    // return BitmapEditor(imageRect.toAlignedRect().topLeft(), sizedImage);
    return sizedImage;
    // qDebug() << "editor size: " << editor.size();
    // return editor;
    // return BitmapEditor(QImage::trueMatrix(transform, selectedEditor.width(), selectedEditor.height()).mapRect(selection.boundingRect()).normalized().toAlignedRect().topLeft(), transformedImage);
}

void BitmapEditor::paintImage(QPainter& painter)
{
    painter.drawImage(mBounds.topLeft(), *image());
}

void BitmapEditor::paintImage(QPainter& painter, QImage& image, QRect sourceRect, QRect destRect)
{
    painter.drawImage(QRect(mBounds.topLeft(), destRect.size()),
                      image,
                      sourceRect);
}

void BitmapEditor::drawLine(QPointF P1, QPointF P2, QPen pen, QPainter::CompositionMode cm, bool antialiasing)
{
    int width = 2 + pen.width();
    setCompositionModeBounds(QRect(P1.toPoint(), P2.toPoint()).normalized().adjusted(-width, -width, width, width), true, cm);
    if (!image()->isNull())
    {
        QPainter painter(image());
        painter.setCompositionMode(cm);
        painter.setRenderHint(QPainter::Antialiasing, antialiasing);
        painter.setPen(pen);
        painter.drawLine(P1 - mBounds.topLeft(), P2 - mBounds.topLeft());
        painter.end();
    }
}

void BitmapEditor::drawRect(QRectF rectangle, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing)
{
    int width = pen.width();
    setCompositionModeBounds(rectangle.adjusted(-width, -width, width, width).toRect(), true, cm);
    if (brush.style() == Qt::RadialGradientPattern)
    {
        QRadialGradient* gradient = (QRadialGradient*)brush.gradient();
        gradient->setCenter(gradient->center() - mBounds.topLeft());
        gradient->setFocalPoint(gradient->focalPoint() - mBounds.topLeft());
    }
    if (!image()->isNull())
    {
        QPainter painter(image());
        painter.setCompositionMode(cm);
        painter.setRenderHint(QPainter::Antialiasing, antialiasing);
        painter.setPen(pen);
        painter.setBrush(brush);

        // Adjust the brush rectangle to be bigger than the bounds itself,
        // otherwise there will be artifacts shown in some cases when smudging
        painter.drawRect(rectangle.translated(-mBounds.topLeft()).adjusted(-1, -1, 1, 1));
        painter.end();
    }
}

void BitmapEditor::drawEllipse(QRectF rectangle, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing)
{
    int width = pen.width();
    setCompositionModeBounds(rectangle.adjusted(-width, -width, width, width).toRect(), true, cm);
    if (brush.style() == Qt::RadialGradientPattern)
    {
        QRadialGradient* gradient = (QRadialGradient*)brush.gradient();
        gradient->setCenter(gradient->center() - mBounds.topLeft());
        gradient->setFocalPoint(gradient->focalPoint() - mBounds.topLeft());
    }
    if (!image()->isNull())
    {
        QPainter painter(image());

        painter.setRenderHint(QPainter::Antialiasing, antialiasing);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.setCompositionMode(cm);
        painter.drawEllipse(rectangle.translated(-mBounds.topLeft()));
        painter.end();
    }
}

void BitmapEditor::drawPath(QPainterPath path, QPen pen, QBrush brush, QPainter::CompositionMode cm, bool antialiasing)
{
    int width = pen.width();
    // qreal inc = 1.0 + width / 20.0;

    setCompositionModeBounds(path.controlPointRect().adjusted(-width, -width, width, width).toRect(), true, cm);

    if (!image()->isNull())
    {
        QPainter painter(image());
        painter.setCompositionMode(cm);
        painter.setRenderHint(QPainter::Antialiasing, antialiasing);
        painter.setPen(pen);
        painter.setBrush(brush);
        painter.setTransform(QTransform().translate(-mBounds.left(), -mBounds.top()));
        painter.setWorldMatrixEnabled(true);
        if (path.length() > 0)
        {
            /*
            for (int pt = 0; pt < path.elementCount() - 1; pt++)
            {
                qreal dx = path.elementAt(pt + 1).x - path.elementAt(pt).x;
                qreal dy = path.elementAt(pt + 1).y - path.elementAt(pt).y;
                qreal m = sqrt(dx*dx + dy*dy);
                qreal factorx = dx / m;
                qreal factory = dy / m;
                for (float h = 0.f; h < m; h += inc)
                {
                    qreal x = path.elementAt(pt).x + factorx * h;
                    qreal y = path.elementAt(pt).y + factory * h;
                    painter.drawPoint(QPointF(x, y));
                }
            }
            */
            painter.drawPath( path );
        }
        else
        {
            // forces drawing when points are coincident (mousedown)
            painter.drawPoint(static_cast<int>(path.elementAt(0).x), static_cast<int>(path.elementAt(0).y));
        }
        painter.end();
    }
}

void BitmapEditor::clear()
{
    mImage = QImage(); // null image
    mBounds = QRect(0, 0, 0, 0);
    mMinBound = true;
}

void BitmapEditor::clear(const QPolygon& polygon)
{
    // QRect boundingBox = polygon.boundingRect();
    // QRect clearRectangle = mBounds.intersected(boundingBox);
    // clearRectangle.moveTopLeft(clearRectangle.topLeft() - mBounds.topLeft());

    // setCompositionModeBounds(clearRectangle, true, QPainter::CompositionMode_Clear);

    QPainter painter(image());
    painter.translate(-mBounds.topLeft());
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    QPainterPath fillPath;
    fillPath.addPolygon(polygon);
    painter.setBrush(Qt::white);
    painter.drawPath(fillPath);
    painter.end();
}

void BitmapEditor::setCompositionModeBounds(const QRect& sourceBounds, bool isSourceMinBounds, QPainter::CompositionMode cm)
{
    QRect newBoundaries;
    switch(cm)
    {
    case QPainter::CompositionMode_Destination:
    case QPainter::CompositionMode_SourceAtop:
        // The Destination and SourceAtop modes
        // do not change the bounds from destination.
        newBoundaries = mBounds;
        // mMinBound remains the same
        break;
    case QPainter::CompositionMode_SourceIn:
    case QPainter::CompositionMode_DestinationIn:
    case QPainter::CompositionMode_Clear:
    case QPainter::CompositionMode_DestinationOut: {
        // The bounds of the result of SourceIn, DestinationIn, Clear, and DestinationOut
        // modes are no larger than the destination bounds
        newBoundaries = mBounds;
        mMinBound = false;
        break;
    }
    default:
        // If it's not one of the above cases, create a union of the two bounds.
        // This contains the minimum bounds, if both the destination and source
        // use their respective minimum bounds.
        newBoundaries = mBounds.united(sourceBounds);
        mMinBound = mMinBound && isSourceMinBounds;
    }

    updateBounds(newBoundaries);
}

void BitmapEditor::autoCrop()
{
    if (!mEnableAutoCrop) return;
    if (mBounds.isEmpty()) return; // Exit if current bounds are null
    if (mImage.isNull()) return;

    Q_ASSERT(mBounds.size() == mImage.size());

    // Exit if already min bounded
    if (mMinBound) return;

    // Get image properties
    const int width = mImage.width();

    // Relative top and bottom row indices (inclusive)
    int relTop = 0;
    int relBottom = mBounds.height() - 1;

    // Check top row
    bool isEmpty = true; // Used to track if a non-transparent pixel has been found
    while (isEmpty && relTop <= relBottom) // Loop through rows
    {
        // Point cursor to the first pixel in the current top row
        const QRgb* cursor = reinterpret_cast<const QRgb*>(mImage.constScanLine(relTop));
        for (int col = 0; col < width; col++) // Loop through pixels in row
        {
            // If the pixel is not transparent
            // (i.e. alpha channel > 0)
            if (qAlpha(*cursor) != 0)
            {
                // We've found a non-transparent pixel in row relTop,
                // so we can stop looking for one
                isEmpty = false;
                break;
            }
            // Move cursor to point to the next pixel in the row
            cursor++;
        }
        if (isEmpty)
        {
            // If the row we just checked was empty, increase relTop
            // to remove the empty row from the top of the bounding box
            ++relTop;
        }
    }

    // Check bottom row
    isEmpty = true; // Reset isEmpty
    while (isEmpty && relBottom >= relTop) // Loop through rows
    {
        // Point cursor to the first pixel in the current bottom row
        const QRgb* cursor = reinterpret_cast<const QRgb*>(mImage.constScanLine(relBottom));
        for (int col = 0; col < width; col++) // Loop through pixels in row
        {
            // If the pixel is not transparent
            // (i.e. alpha channel > 0)
            if(qAlpha(*cursor) != 0)
            {
                // We've found a non-transparent pixel in row relBottom,
                // so we can stop looking for one
                isEmpty = false;
                break;
            }
            // Move cursor to point to the next pixel in the row
            ++cursor;
        }
        if (isEmpty)
        {
            // If the row we just checked was empty, decrease relBottom
            // to remove the empty row from the bottom of the bounding box
            --relBottom;
        }
    }

    // Relative left and right column indices (inclusive)
    int relLeft = 0;
    int relRight = mBounds.width()-1;

    // Check left row
    isEmpty = (relBottom >= relTop); // Check left only when
    while (isEmpty && relBottom >= relTop && relLeft <= relRight) // Loop through columns
    {
        // Point cursor to the pixel at row relTop and column relLeft
        const QRgb* cursor = reinterpret_cast<const QRgb*>(mImage.constScanLine(relTop)) + relLeft;
        // Loop through pixels in column
        // Note: we only need to loop from relTop to relBottom (inclusive)
        //       not the full image height, because rows 0 to relTop-1 and
        //       relBottom+1 to mBounds.height() have already been
        //       confirmed to contain only transparent pixels
        for (int row = relTop; row <= relBottom; row++)
        {
            // If the pixel is not transparent
            // (i.e. alpha channel > 0)
            if(qAlpha(*cursor) != 0)
            {
                // We've found a non-transparent pixel in column relLeft,
                // so we can stop looking for one
                isEmpty = false;
                break;
            }
            // Move cursor to point to next pixel in the column
            // Increment by width because the data is in row-major order
            cursor += width;
        }
        if (isEmpty)
        {
            // If the column we just checked was empty, increase relLeft
            // to remove the empty column from the left of the bounding box
            ++relLeft;
        }
    }

    // Check right row
    isEmpty = (relBottom >= relTop); // Reset isEmpty
    while (isEmpty && relRight >= relLeft) // Loop through columns
    {
        // Point cursor to the pixel at row relTop and column relRight
        const QRgb* cursor = reinterpret_cast<const QRgb*>(mImage.constScanLine(relTop)) + relRight;
        // Loop through pixels in column
        // Note: we only need to loop from relTop to relBottom (inclusive)
        //       not the full image height, because rows 0 to relTop-1 and
        //       relBottom+1 to mBounds.height()-1 have already been
        //       confirmed to contain only transparent pixels
        for (int row = relTop; row <= relBottom; row++)
        {
            // If the pixel is not transparent
            // (i.e. alpha channel > 0)
            if(qAlpha(*cursor) != 0)
            {
                // We've found a non-transparent pixel in column relRight,
                // so we can stop looking for one
                isEmpty = false;
                break;
            }
            // Move cursor to point to next pixel in the column
            // Increment by width because the data is in row-major order
            cursor += width;
        }
        if (isEmpty)
        {
            // If the column we just checked was empty, increase relRight
            // to remove the empty column from the left of the bounding box
            --relRight;
        }
    }

    //qDebug() << "Original" << mBounds;
    //qDebug() << "Autocrop" << relLeft << relTop << relRight - mBounds.width() + 1 << relBottom - mBounds.height() + 1;
    // Update mBounds and mImage if necessary
    updateBounds(mBounds.adjusted(relLeft, relTop, relRight - mBounds.width() + 1, relBottom - mBounds.height() + 1));

    mMinBound = true;
}

QRgb BitmapEditor::constScanLine(int x, int y) const
{
    QRgb result = QRgb();
    if (mBounds.contains(x, y)) {
        result = *(reinterpret_cast<const QRgb*>(mImage.constScanLine(y - mBounds.top())) + x - mBounds.left());
    }
    return result;
}

void BitmapEditor::scanLine(int x, int y, QRgb color)
{
    if (!mBounds.contains(x, y)) {
        return;
    }
    // Make sure color is premultiplied before calling
    *(reinterpret_cast<QRgb*>(image()->scanLine(y - mBounds.top())) + x - mBounds.left()) = color;
}

QRgb BitmapEditor::pixel(const QPoint& p) const
{
    QRgb result = qRgba(0, 0, 0, 0); // black
    if (mBounds.contains(p))
        result = mImage.pixel(p - mBounds.topLeft());
    return result;
}

void BitmapEditor::setPixel(const QPoint& p, QRgb color)
{
    setCompositionModeBounds(QRect(p, QSize(1,1)), true, QPainter::CompositionMode_SourceOver);
    if (mBounds.contains(p))
    {
        image()->setPixel(p - mBounds.topLeft(), color);
    }
}

void BitmapEditor::paste(const BitmapEditor& bitmapEditor, const QPointF& topLeft, QPainter::CompositionMode mode)
{
    if(!bitmapEditor.bounds().isValid())
    {
        return;
    }


    setCompositionModeBounds(bitmapEditor.bounds(), bitmapEditor.isMinimallyBounded(), mode);

    const QImage* imageToPaste = bitmapEditor.constImage();

    QPainter painter(&mImage);
    // painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setCompositionMode(mode);
    painter.translate(-mBounds.topLeft());
    painter.drawImage(topLeft, *imageToPaste);
    painter.end();
}

void BitmapEditor::paste(const BitmapEditor& bitmapEditor, QPainter::CompositionMode mode)
{
    if(!bitmapEditor.bounds().isValid())
    {
        return;
    }

    setCompositionModeBounds(bitmapEditor.bounds(), bitmapEditor.isMinimallyBounded(), mode);

    const QImage* imageToPaste = bitmapEditor.constImage();

    QPainter painter(&mImage);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setCompositionMode(mode);
    painter.drawImage(bitmapEditor.bounds().topLeft() - mBounds.topLeft(), *imageToPaste);
    painter.end();
}

void BitmapEditor::paste(const TiledBuffer* tiledBuffer, const QPolygon& clipPolygon, const QTransform& transform, QPainter::CompositionMode cm)
{
    if(tiledBuffer->bounds().width() <= 0 || tiledBuffer->bounds().height() <= 0)
    {
        return;
    }
    extend(tiledBuffer->bounds());

    QPainter painter(image());

    painter.setCompositionMode(cm);
    QTransform t = QTransform::fromTranslate(-mBounds.topLeft().x(), -mBounds.topLeft().y());
    if (clipPolygon.count() > 0) {
        // When clipping is enabled, the clipping shape
        // needs to be transformed so it matches the orientation of the image
        painter.setTransform(t);
        QPainterPath clipPath;
        clipPath.addPolygon(clipPolygon);
        painter.setClipPath(clipPath);
        painter.setClipping(true);
    }

    // The tiledbuffer image is never pre-transformed, as such we need to invert the transform to avoid it being
    // placed incorrectly on the image
    painter.setTransform(transform.inverted() * t);
    auto const tiles = tiledBuffer->tiles();
    for (const Tile* item : tiles) {
        const QPixmap& tilePixmap = item->pixmap();
        const QPoint& tilePos = item->pos();
        painter.drawPixmap(tilePos, tilePixmap);
    }
    painter.end();
}

QImage* BitmapEditor::image()
{
    if (!isLoaded()) {
        loadFile();
    }
    return &mImage;
}

const QImage* BitmapEditor::constImage() const
{
    return &mImage;
}

quint64 BitmapEditor::memoryUsage() const
{
    if (!mImage.isNull())
    {
        #if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
            return mImage.sizeInBytes();
        #else
            return mImage.byteCount();
        #endif
    }
    
    return 0;
}
