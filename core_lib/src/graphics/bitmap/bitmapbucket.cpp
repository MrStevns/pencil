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
#include "bitmapbucket.h"

#include <QtMath>
#include <QDebug>

#include "editor.h"
#include "layermanager.h"

#include "layerbitmap.h"
#include "blitrect.h"

BitmapBucket::BitmapBucket()
{
}

BitmapBucket::BitmapBucket(Editor* editor,
                           QColor color,
                           QRect maxFillRegion,
                           QPointF fillPoint,
                           Properties properties):
    mEditor(editor),
    mMaxFillRegion(maxFillRegion),
    mProperties(properties)

{
    Layer* initialLayer = editor->layers()->currentLayer();
    int initialLayerIndex = mEditor->currentLayerIndex();
    int frameIndex = mEditor->currentFrame();

    mBucketColor = qPremultiply(color.rgba());

    mTargetFillToLayer = initialLayer;
    mTargetFillToLayerIndex = initialLayerIndex;

    mTolerance = mProperties.toleranceEnabled ? static_cast<int>(mProperties.tolerance) : 0;
    const QPoint& point = QPoint(qFloor(fillPoint.x()), qFloor(fillPoint.y()));

    Q_ASSERT(mTargetFillToLayer);

    BitmapImage singleLayerImage = *static_cast<BitmapImage*>(initialLayer->getLastKeyFrameAtPosition(frameIndex));
    if (properties.bucketFillReferenceMode == 1) // All layers
    {
        mReferenceImage = flattenBitmapLayersToImage();
    } else {
        mReferenceImage = singleLayerImage;
    }
    mStartReferenceColor = mReferenceImage.constScanLine(point.x(), point.y());
    mUseDragToFill = canUseDragToFill(point, color, singleLayerImage);

    mPixelCache = new QHash<QRgb, bool>();
}

bool BitmapBucket::canUseDragToFill(const QPoint& fillPoint, const QColor& bucketColor, const BitmapImage& referenceImage)
{
    QRgb pressReferenceColorSingleLayer = referenceImage.constScanLine(fillPoint.x(), fillPoint.y());
    QRgb startRef = qUnpremultiply(pressReferenceColorSingleLayer);

    if (mProperties.fillMode == 0 && ((QColor(qRed(startRef), qGreen(startRef), qBlue(startRef)) == bucketColor.rgb() && qAlpha(startRef) == 255) || bucketColor.alpha() == 0)) {
        // In overlay mode: When the reference pixel matches the bucket color and the reference is fully opaque
        // Otherwise when the bucket alpha is zero.
        return false;
    } else if (mProperties.fillMode == 2 && qAlpha(startRef) == 255) {
        // In behind mode: When the reference pixel is already fully opaque, the output will be invisible.
        return false;
    }

    return true;
}

bool BitmapBucket::allowFill(const QPoint& checkPoint, const QRgb& checkColor) const
{
    // A normal click to fill should happen unconditionally, because the alternative is utterly confusing.
    if (!mFilledOnce) {
        return true;
    }

    return allowContinuousFill(checkPoint, checkColor);
}

bool BitmapBucket::allowContinuousFill(const QPoint& checkPoint, const QRgb& checkColor) const
{
    if (!mUseDragToFill) {
        return false;
    }

    const QRgb& colorOfReferenceImage = mReferenceImage.constScanLine(checkPoint.x(), checkPoint.y());

    if (checkColor == mBucketColor && (mProperties.fillMode == 1 || qAlpha(checkColor) == 255))
    {
        // Avoid filling if target pixel color matches fill color
        // to avoid creating numerous seemingly useless undo operations
        return false;
    }

    return compareColor(colorOfReferenceImage, mStartReferenceColor, mTolerance, mPixelCache) &&
           (checkColor == 0 || compareColor(checkColor, mStartReferenceColor, mTolerance, mPixelCache));
}

void BitmapBucket::paint(const QPointF& updatedPoint, std::function<void(BucketState, int, int)> state)
{
    const int currentFrameIndex = mEditor->currentFrame();

    BitmapImage* targetImage = static_cast<LayerBitmap*>(mTargetFillToLayer)->getLastBitmapImageAtFrame(currentFrameIndex, 0);
    if (targetImage == nullptr || !targetImage->isLoaded()) { return; } // Can happen if the first frame is deleted while drawing

    QPoint point = QPoint(qFloor(updatedPoint.x()), qFloor(updatedPoint.y()));
    if (!mReferenceImage.contains(point))
    {
        // If point is outside the our max known fill area, move the fill point anywhere within the bounds
        point = mReferenceImage.topLeft();
    }

    const QRgb& targetPixelColor = targetImage->constScanLine(point.x(), point.y());

    if (!allowFill(point, targetPixelColor)) {
        return;
    }

    QRgb fillColor = mBucketColor;
    if (mProperties.fillMode == 1)
    {
        // Pass a fully opaque version of the new color to floodFill
        // This is required so we can fully mask out the existing data before
        // writing the new color.
        QColor tempColor;
        tempColor.setRgba(fillColor);
        tempColor.setAlphaF(1);
        fillColor = tempColor.rgba();
    }

    BitmapEditor* targetBitmapEditor = targetImage->editor();
    BitmapEditor* replaceBitmapEditor = nullptr;

    int expandValue = mProperties.bucketFillExpandEnabled ? mProperties.bucketFillExpand : 0;
    bool didFloodFill = floodFill(&replaceBitmapEditor,
                           mReferenceImage.editor(),
                           mMaxFillRegion,
                           point,
                           fillColor,
                           mTolerance,
                           expandValue);

    if (!didFloodFill) {
        delete replaceBitmapEditor;
        return;
    }
    Q_ASSERT(replaceBitmapEditor != nullptr);

    state(BucketState::WillFillTarget, mTargetFillToLayerIndex, currentFrameIndex);

    if (mProperties.fillMode == 0)
    {
        targetBitmapEditor->paste(*replaceBitmapEditor);
    }
    else if (mProperties.fillMode == 2)
    {
        targetBitmapEditor->paste(*replaceBitmapEditor, QPainter::CompositionMode_DestinationOver);
    }
    else
    {
        // fill mode replace
        targetBitmapEditor->paste(*replaceBitmapEditor, QPainter::CompositionMode_DestinationOut);
        // Reduce the opacity of the fill to match the new color
        BitmapEditor properColorEditor(replaceBitmapEditor->bounds(), QColor::fromRgba(mBucketColor));
        properColorEditor.paste(*replaceBitmapEditor, QPainter::CompositionMode_DestinationIn);
        // Write reduced-opacity fill image on top of target image
        targetBitmapEditor->paste(properColorEditor);
    }

    targetImage->modification();
    delete replaceBitmapEditor;

    state(BucketState::DidFillTarget, mTargetFillToLayerIndex, currentFrameIndex);
    mFilledOnce = true;
}

BitmapImage BitmapBucket::flattenBitmapLayersToImage()
{
    BitmapImage flattenImage = BitmapImage();
    int currentFrame = mEditor->currentFrame();
    auto layerMan = mEditor->layers();
    for (int i = 0; i < layerMan->count(); i++)
    {
        Layer* layer = layerMan->getLayer(i);
        Q_ASSERT(layer);
        if (layer->type() == Layer::BITMAP && layer->visible())
        {
            BitmapImage* image = static_cast<LayerBitmap*>(layer)->getLastBitmapImageAtFrame(currentFrame);
            if (image) {
                flattenImage.paste(image);
            }
        }
    }
    return flattenImage;
}

bool BitmapBucket::floodFill(BitmapEditor** replaceImage,
                            const BitmapEditor* targetImage,
                            const QRect& cameraRect,
                            const QPoint& point,
                            const QRgb& fillColor,
                            int tolerance,
                            const int expandValue)
{
    // Fill region must be 1 pixel larger than the target image to fill regions on the edge connected only by transparent pixels
    const QRect& fillBounds = targetImage->bounds().adjusted(-1, -1, 1, 1);
    QRect maxBounds = cameraRect.united(fillBounds).adjusted(-expandValue, -expandValue, expandValue, expandValue);
    const int maxWidth = maxBounds.width(), left = maxBounds.left(), top = maxBounds.top();

    // Square tolerance for use with compareColor
    tolerance = static_cast<int>(qPow(tolerance, 2));

    QRect newBounds;
    bool *filledPixels = floodFillPoints(targetImage, maxBounds, point, tolerance, newBounds);

    QRect translatedSearchBounds = newBounds.translated(-maxBounds.topLeft());

    // The scanned bounds should take the expansion into account
    const QRect& expandRect = newBounds.adjusted(-expandValue, -expandValue, expandValue, expandValue);
    if (expandValue > 0) {
        newBounds = expandRect;
    }
    if (!maxBounds.contains(newBounds)) {
        newBounds = maxBounds;
    }
    translatedSearchBounds = newBounds.translated(-maxBounds.topLeft());

    if (expandValue > 0) {
        expandFill(filledPixels, translatedSearchBounds, maxBounds, expandValue);
    }

    *replaceImage = new BitmapEditor(newBounds, Qt::transparent);

    // Fill all the found pixels
    for (int y = translatedSearchBounds.top(); y <= translatedSearchBounds.bottom(); y++)
    {
        for (int x = translatedSearchBounds.left(); x <= translatedSearchBounds.right(); x++)
        {
            const int index = y * maxWidth + x;
            if (!filledPixels[index])
            {
                continue;
            }
            (*replaceImage)->scanLine(x + left, y + top, fillColor);
        }
    }

    delete[] filledPixels;

    return true;
}


// Flood filling based on this scanline algorithm
// ----- http://lodev.org/cgtutor/floodfill.html
bool* BitmapBucket::floodFillPoints(const BitmapEditor* targetImage,
                                   const QRect& searchBounds,
                                   QPoint point,
                                   const int tolerance,
                                   QRect& newBounds)
{
    QRgb oldColor = targetImage->constScanLine(point.x(), point.y());
    oldColor = qRgba(qRed(oldColor), qGreen(oldColor), qBlue(oldColor), qAlpha(oldColor));

    // Preparations
    QList<QPoint> queue; // queue all the pixels of the filled area (as they are found)

    QPoint tempPoint;
    QScopedPointer< QHash<QRgb, bool> > cache(new QHash<QRgb, bool>());

    int xTemp = 0;
    bool spanLeft = false;
    bool spanRight = false;

    queue.append(point);
    // Preparations END

    bool *filledPixels = new bool[searchBounds.height()*searchBounds.width()]{};

    BlitRect blitBounds(point);
    while (!queue.empty())
    {
        tempPoint = queue.takeFirst();

        point.setX(tempPoint.x());
        point.setY(tempPoint.y());

        xTemp = point.x();

        int xCoord = xTemp - searchBounds.left();
        int yCoord = point.y() - searchBounds.top();

        if (filledPixels[yCoord*searchBounds.width()+xCoord]) continue;

        while (xTemp >= searchBounds.left() &&
               compareColor(targetImage->constScanLine(xTemp, point.y()), oldColor, tolerance, cache.data())) xTemp--;
        xTemp++;

        spanLeft = spanRight = false;
        while (xTemp <= searchBounds.right() &&
               compareColor(targetImage->constScanLine(xTemp, point.y()), oldColor, tolerance, cache.data()))
        {

            QPoint floodPoint = QPoint(xTemp, point.y());
            if (!blitBounds.contains(floodPoint)) {
                blitBounds.extend(floodPoint);
            }

            xCoord = xTemp - searchBounds.left();
            // This pixel is what we're going to fill later
            filledPixels[yCoord*searchBounds.width()+xCoord] = true;

            if (!spanLeft && (point.y() > searchBounds.top()) &&
                compareColor(targetImage->constScanLine(xTemp, point.y() - 1), oldColor, tolerance, cache.data())) {
                queue.append(QPoint(xTemp, point.y() - 1));
                spanLeft = true;
            }
            else if (spanLeft && (point.y() > searchBounds.top()) &&
                     !compareColor(targetImage->constScanLine(xTemp, point.y() - 1), oldColor, tolerance, cache.data())) {
                spanLeft = false;
            }

            if (!spanRight && point.y() < searchBounds.bottom() &&
                compareColor(targetImage->constScanLine(xTemp, point.y() + 1), oldColor, tolerance, cache.data())) {
                queue.append(QPoint(xTemp, point.y() + 1));
                spanRight = true;
            }
            else if (spanRight && point.y() < searchBounds.bottom() &&
                     !compareColor(targetImage->constScanLine(xTemp, point.y() + 1), oldColor, tolerance, cache.data())) {
                spanRight = false;
            }

            Q_ASSERT(queue.count() < (searchBounds.width() * searchBounds.height()));
            xTemp++;
        }
    }

    newBounds = blitBounds;

    return filledPixels;
}

/** Finds all pixels closest to the input color and applies the input color to the image
 *
 * An example:
 *
 * 0 is where the color was found
 * 1 is the distance from the nearest pixel of that color
 *
 * 211112
 * 100001
 * 100001
 * 211112
 *
 * @param fillPixels: pixels to fill
 * @param searchBounds: the bounds to search
 * @param maxBounds: The maximum bound
 * @param expand: the amount of pixels to expand
 */
void BitmapBucket::expandFill(bool* fillPixels, const QRect& searchBounds, const QRect& maxBounds, int expand) {

    const int maxWidth = maxBounds.width();
    const int length = maxBounds.height() * maxBounds.width();

    int* manhattanPoints = new int[length]{};

    // Fill points with max length, this is important because otherwise the filled pixels will include a border of the expanded area
    std::fill_n(manhattanPoints, length, searchBounds.width()+searchBounds.height());

    for (int y = searchBounds.top(); y <= searchBounds.bottom(); y++)
    {
        for (int x = searchBounds.left(); x <= searchBounds.right(); x++)
        {
            const int index = y*maxWidth+x;

            if (fillPixels[index]) {
                manhattanPoints[index] = 0;
                continue;
            }

            if (y > searchBounds.top()) {
                // the value will be the num of pixels away from y - 1 of the next position
                manhattanPoints[index] = qMin(manhattanPoints[index],
                                             manhattanPoints[(y - 1) * maxWidth+x] + 1);

                int distance = manhattanPoints[index];
                if (distance <= expand) {
                    fillPixels[index] = true;
                }
            }
            if (x > searchBounds.left()) {
                // the value will be the num of pixels away from x - 1 of the next position
                manhattanPoints[index] = qMin(manhattanPoints[index],
                                             manhattanPoints[y*maxWidth+(x - 1)] + 1);

                int distance = manhattanPoints[index];
                if (distance <= expand) {
                    fillPixels[index] = true;
                }
            }
        }
    }

    // traverse from bottom right to top left
    for (int y = searchBounds.bottom(); y >= searchBounds.top(); y--)
    {
        for (int x = searchBounds.right(); x >= searchBounds.left(); x--)
        {
            const int index = y*maxWidth+x;

            if (y + 1 < searchBounds.bottom()) {
                manhattanPoints[index] = qMin(manhattanPoints[index], manhattanPoints[(y + 1)*maxWidth+x] + 1);

                int distance = manhattanPoints[index];
                if (distance <= expand) {
                    fillPixels[index] = true;
                }
            }
            if (x + 1 < searchBounds.right()) {
                manhattanPoints[index] = qMin(manhattanPoints[index], manhattanPoints[y*maxWidth+(x + 1)] + 1);

                int distance = manhattanPoints[index];
                if (distance <= expand) {
                    fillPixels[index] = true;
                }
            }
        }
    }

    delete[] manhattanPoints;
}
