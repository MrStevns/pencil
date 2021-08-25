#ifndef BITMAPUTILS_H
#define BITMAPUTILS_H

#include "bitmapimage.h"
#include "qmath.h"
#include <QHash>

struct BitmapUtils {

 /** constScanline
 * Recommended method to search pixels in an image and return the rgb at the given position.
 * @param inImage
 * @param bounds
 * @param x
 * @param y
 * @return the color at the given position
 */
static QRgb constScanLine(const BitmapImage& bitmapImage, const int x, const int y)
{
    QRgb result = qRgba(0, 0, 0, 0);
    if (bitmapImage.bounds().contains(QPoint(x,y))) {
        result = *(reinterpret_cast<const QRgb*>(bitmapImage.constImage()->constScanLine(y - bitmapImage.top())) + x - bitmapImage.left() );
    }
    return result;
}

/** scanLine the image and replace the pixel at the given pos
 * outImage will contain the modification
 * @param outImage
 * @param bounds
 * @param x
 * @param y
 * @param colour
 */
static void scanLine(BitmapImage* bitmapImage, const int x, const int y, const QRgb& colour)
{
    // Make sure color is premultiplied before calling
    if (bitmapImage->bounds().contains(QPoint(x,y))) {
        QRgb toColor = qRgba(qRed(colour),qGreen(colour), qBlue(colour), qAlpha(colour));
        QRgb* sourceColor = (reinterpret_cast<QRgb*>(bitmapImage->image()->scanLine(y - bitmapImage->topLeft().y())) + x - bitmapImage->topLeft().x());
        *sourceColor = toColor;
    }
}

static QRgb pixel(const BitmapImage& bitmapImage, int x, int y)
{
    return constScanLine(bitmapImage, x, y);
}

static QRgb pixel(const BitmapImage& bitmapImage, QPoint point)
{
    return constScanLine(bitmapImage, point.x(), point.y());
}

static void setPixel(BitmapImage& bitmapImage, const int x, const int y, const QRgb& colour)
{
    bitmapImage.image()->setPixel(QPoint(x, y)-bitmapImage.topLeft(), colour);
}

/** Compare colors for the purposes of flood filling
 *
 *  Calculates the Eulcidian difference of the RGB channels
 *  of the image and compares it to the tolerance
 *
 *  @param[in] newColor The first color to compare
 *  @param[in] oldColor The second color to compare
 *  @param[in] tolerance The threshold limit between a matching and non-matching color
 *  @param[in,out] cache Contains a mapping of previous results of compareColor with rule that
 *                 cache[someColor] = compareColor(someColor, oldColor, tolerance)
 *
 *  @return Returns true if the colors have a similarity below the tolerance level
 *          (i.e. if Eulcidian distance squared is <= tolerance)
 */
static bool compareColor(QRgb newColor, QRgb oldColor, int tolerance, QHash<QRgb, bool> *cache)
{
    // Handle trivial case
    if (newColor == oldColor) return true;

    if(cache && cache->contains(newColor)) return cache->value(newColor);

    // Get Eulcidian distance between colors
    // Not an accurate representation of human perception,
    // but it's the best any image editing program ever does
    int diffRed = static_cast<int>(qPow(qRed(oldColor) - qRed(newColor), 2));
    int diffGreen = static_cast<int>(qPow(qGreen(oldColor) - qGreen(newColor), 2));
    int diffBlue = static_cast<int>(qPow(qBlue(oldColor) - qBlue(newColor), 2));
    // This may not be the best way to handle alpha since the other channels become less relevant as
    // the alpha is reduces (ex. QColor(0,0,0,0) is the same as QColor(255,255,255,0))
    int diffAlpha = static_cast<int>(qPow(qAlpha(oldColor) - qAlpha(newColor), 2));

    bool isSimilar = (diffRed + diffGreen + diffBlue + diffAlpha) <= tolerance;

    if(cache)
    {
        Q_ASSERT(cache->contains(isSimilar) ? isSimilar == (*cache)[newColor] : true);
        (*cache)[newColor] = isSimilar;
    }

    return isSimilar;
}

// Flood fill
// ----- http://lodev.org/cgtutor/floodfill.html
static bool floodFill(BitmapImage* replaceImage,
                            BitmapImage* targetImage,
                            QPoint point,
                            QRgb fillColor,
                            int tolerance)
{
    // Square tolerance for use with compareColor
    tolerance = static_cast<int>(qPow(tolerance, 2));

    QRgb oldColor = pixel(*targetImage, point);
    oldColor = qRgba(qRed(oldColor), qGreen(oldColor), qBlue(oldColor), qAlpha(oldColor));

    // Preparations
    QList<QPoint> queue; // queue all the pixels of the filled area (as they are found)

    QPoint tempPoint;
    QRgb newPlacedColor = 0;
    QScopedPointer< QHash<QRgb, bool> > cache(new QHash<QRgb, bool>());

    int xTemp = 0;
    bool spanLeft = false;
    bool spanRight = false;

    queue.append(point);
    // Preparations END

    while (!queue.empty())
    {
        tempPoint = queue.takeFirst();

        point.setX(tempPoint.x());
        point.setY(tempPoint.y());

        xTemp = point.x();

        newPlacedColor = constScanLine(*replaceImage, xTemp, point.y());
        while (xTemp >= targetImage->left() &&
               compareColor(constScanLine(*targetImage, xTemp, point.y()), oldColor, tolerance, cache.data())) xTemp--;
        xTemp++;

        spanLeft = spanRight = false;
        while (xTemp <= targetImage->right() &&
               compareColor(constScanLine(*targetImage, xTemp, point.y()), oldColor, tolerance, cache.data()) &&
               newPlacedColor != fillColor)
        {

            // Set pixel color
            scanLine(replaceImage, xTemp, point.y(), fillColor);

            if (!spanLeft && (point.y() > targetImage->top()) &&
                compareColor(constScanLine(*targetImage, xTemp, point.y() - 1), oldColor, tolerance, cache.data())) {
                queue.append(QPoint(xTemp, point.y() - 1));
                spanLeft = true;
            }
            else if (spanLeft && (point.y() > targetImage->top()) &&
                     !compareColor(constScanLine(*targetImage, xTemp, point.y() - 1), oldColor, tolerance, cache.data())) {
                spanLeft = false;
            }

            if (!spanRight && point.y() < targetImage->bottom() &&
                compareColor(constScanLine(*targetImage, xTemp, point.y() + 1), oldColor, tolerance, cache.data())) {
                queue.append(QPoint(xTemp, point.y() + 1));
                spanRight = true;

            }
            else if (spanRight && point.y() < targetImage->bottom() &&
                     !compareColor(constScanLine(*targetImage, xTemp, point.y() + 1), oldColor, tolerance, cache.data())) {
                spanRight = false;
            }

            Q_ASSERT(queue.count() < (targetImage->width() * targetImage->height()));
            xTemp++;
        }
    }

    return true;
}

/** Fills the target image with a given color in a radius of the expansion value
 *
 * @param targetImage
 * @param newColor
 * @param expand
 */
static void expandFill(BitmapImage* targetImage, QRgb newColor, int expand)
{
    QList<QPoint> expandPoints;

    QRect expandRect = QRect(targetImage->topLeft() - QPoint(expand, expand), targetImage->bottomRight() + QPoint(expand, expand));
    targetImage->extendBoundaries(expandRect);

    auto twoDVectorList = manhattanDistance(targetImage, newColor);

    for (int y = 0; y < expandRect.height(); y++)
    {
        for (int x = 0; x < expandRect.width(); x++)
        {
            if (twoDVectorList[y][x] <= expand && twoDVectorList[y][x] != 0) {
                *(reinterpret_cast<QRgb*>(targetImage->image()->scanLine(y)) + x) = newColor;
            }
        }
    }
}

/** Finds all pixels closest to the input color and returns the result as a 2D array
 *  matching the size of the image
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
 * @param bitmapImage: Image to search
 * @param searchColor: Color to find
 * @return Return a 2D array of pixels closes to the inputColor
 */
static QVector<QVector<int>> manhattanDistance(BitmapImage* bitmapImage, QRgb& searchColor) {

    // Allocate with size of image size
    QVector<QVector<int>> manhattanPoints(bitmapImage->height(), QVector<int>(bitmapImage->width()));

    // traverse from top left to bottom right
    for (int y = 0; y < manhattanPoints.length(); y++) {
        for (int x = 0; x < manhattanPoints[y].length(); x++) {

            const QRgb& colorAtPixel = *(reinterpret_cast<const QRgb*>(bitmapImage->image()->constScanLine(y)) + x);
            if (colorAtPixel == searchColor) {
                manhattanPoints[y][x] = 0;
            } else {
                manhattanPoints[y][x] = manhattanPoints.length() + manhattanPoints[y].length();

                if (y > 0) {
                    // the value will be the num of pixels away from y - 1 of the next position
                    manhattanPoints[y][x] = qMin(manhattanPoints[y][x],
                                                 manhattanPoints[y - 1][x] + 1);
                }
                if (x > 0) {
                    // the value will be the num of pixels away from x - 1 of the next position
                    manhattanPoints[y][x] = qMin(manhattanPoints[y][x],
                                                 manhattanPoints[y][x - 1] + 1);
                }
            }
        }
    }

    // traverse from bottom right to top left
    for (int y = manhattanPoints.length() - 1; y >= 0; y--) {
        for (int x = manhattanPoints[y].length() - 1; x >= 0; x--) {

            if (y + 1 < manhattanPoints.length()) {
                manhattanPoints[y][x] = qMin(manhattanPoints[y][x], manhattanPoints[y + 1][x] + 1);
            }
            if (x + 1 < manhattanPoints[y].length()) {
                manhattanPoints[y][x] = qMin(manhattanPoints[y][x], manhattanPoints[y][x + 1] + 1);
            }
        }
    }

    return manhattanPoints;
}

};


#endif // BITMAPUTILS_H
