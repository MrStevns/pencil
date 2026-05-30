#ifndef BITMAPSELECTIONRENDERWORKER_H
#define BITMAPSELECTIONRENDERWORKER_H

#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QRect>
#include <QHash>

struct BitmapSelectionRenderJob {
    int    jobId;
    QImage baseCache;
    QRect  baseCacheBounds;
    QTransform transform;
    int    padding;
    bool smoothTransform;
};

struct BitmapSelectionRenderResult {
    int    jobId;

    QImage transformedImage;
    QRect transformedBounds;
};

class BitmapSelectionRenderWorker : public QObject {
    Q_OBJECT
public:
    int generateId() {
        return mNextId++;
    }

    void requestJob(const BitmapSelectionRenderJob& job);

    void cancelAndRemove(int jobId);

    bool tryGetResult(int jobId, BitmapSelectionRenderResult& result);
    void deleteJob(int jobId);

signals:
    void jobDone(int jobId);

private:

    /// Creates a sub pixel transformed image based on the input image
    /// @param src The input image to transform
    /// @param transform The transform that alters the input image
    /// @param alignedRect The rectangle used for the image bounds
    /// @param preciseRect The rectangle used for the sub pixel transformations
    /// @param smooth A boolean indicating whether to transform the image using SmoothPixmapTransform
    QImage subPixelTransformedImage(const QImage& src,
                                   const QTransform& transform,
                                   const QRect& alignedRect,
                                   const QRectF& preciseRect,
                                   bool smooth) const;

    /// Computes two rectangles, a rectangle for the aligned bounds of the image used to create the image
    /// and a second bound used to allow smooth sub pixel transformation
    ///
    /// @param sourceBounds The bound of the source, ie. the image boundingbox
    /// @param tranform The transform used to transform the image
    /// @param outAlignedRect the bound required to store the transformed image.
    /// @param outPreciseRect the bound required to calculate the sub pixel position of the image
    void computeTransformedImageBounds(const QRect& sourceBounds,
                                       const QTransform& transform,
                                       QRect& outAlignedRect, QRectF& outPreciseRect) const;

    int mNextId = 0;
    QHash<int, QFutureWatcher<BitmapSelectionRenderResult>*> mWatchers;
    QHash<int, BitmapSelectionRenderResult>                  mResults;
};

#endif // BITMAPSELECTIONRENDERWORKER_H
