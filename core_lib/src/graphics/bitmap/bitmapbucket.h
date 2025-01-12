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
#ifndef BITMAPBUCKET_H
#define BITMAPBUCKET_H

#include "bitmapimage.h"
#include "basetool.h"

#include <functional>

class Layer;
class Editor;

enum class BucketState
{
    WillFillTarget, // Before applying to target image
    DidFillTarget, // After calling floodfill and applied to target
};

class BitmapBucket
{
public:
    explicit BitmapBucket();
    explicit BitmapBucket(Editor* editor, QColor color, QRect maxFillRegion, QPointF fillPoint, Properties properties);

    /** Will paint at the given point, given that it makes sense.. canUse is always called prior to painting
     *
     * @param updatedPoint - the point where to point
     * @param progress - a function that returns the progress of the paint operation,
     * the layer and frame that was affected at the given point.
     */
    void paint(const QPointF& updatedPoint, std::function<void(BucketState, int, int)> progress);

private:

    static bool floodFill(BitmapEditor** replaceImage,
                            const BitmapEditor* targetImage,
                            const QRect& cameraRect,
                            const QPoint& point,
                            const QRgb& fillColor,
                            int tolerance,
                            const int expandValue);
    static bool* floodFillPoints(const BitmapEditor* targetImage,
                                   const QRect& searchBounds,
                                   QPoint point,
                                   const int tolerance,
                                   QRect& newBounds);
    static void expandFill(bool* fillPixels, const QRect& searchBounds, const QRect& maxBounds, int expand);


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
    static inline bool compareColor(QRgb newColor, QRgb oldColor, int tolerance, QHash<QRgb, bool> *cache)
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


    /** Based on the various factors dependant on which tool properties are set,
     *  the result will:
     *
     *  BucketProgress: BeforeFill
     *  to allow filling
     *
     * @param checkPoint
     * @return True if you are allowed to fill, otherwise false
     */
    bool allowFill(const QPoint& checkPoint, const QRgb& checkColor) const;
    bool allowContinuousFill(const QPoint& checkPoint, const QRgb& checkColor) const;

    /** Determines whether fill to drag feature can be used */
    bool canUseDragToFill(const QPoint& fillPoint, const QColor& bucketColor, const BitmapImage& referenceImage);

    BitmapImage flattenBitmapLayersToImage();

    Editor* mEditor = nullptr;
    Layer* mTargetFillToLayer = nullptr;

    QHash<QRgb, bool> *mPixelCache;

    BitmapImage mReferenceImage;
    QRgb mBucketColor = 0;
    QRgb mStartReferenceColor = 0;

    QRect mMaxFillRegion;

    int mTolerance = 0;

    int mTargetFillToLayerIndex = -1;
    bool mFilledOnce = false;
    bool mUseDragToFill = false;

    Properties mProperties;
};

#endif // BITMAPBUCKET_H
