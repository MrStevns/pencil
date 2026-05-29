#ifndef SELECTIONPATCHSYSTEM_H
#define SELECTIONPATCHSYSTEM_H

#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QRect>
#include <QHash>

struct SelectionPatchJob {
    int    jobId;
    QImage backingImage;
    QRect  backingImageBounds;
    QImage baseCache;
    QRect  baseCacheBounds;
    QTransform transform;
    int    padding;
    bool smoothTransform;
};

struct SelectionPatchResult {
    int    jobId;

    // Patch of the composited result
    QImage patch;
    QRect  bounds;

    // The transformed image which is used for pasting onto a keyframe
    QImage transformedImage;
    QRect transformedBounds;
};

class SelectionPatchSystem : public QObject {
    Q_OBJECT
public:
    int generateId() {
        return mNextId++;
    }

    void requestPatch(const SelectionPatchJob& job);

    void cancelAndRemove(int jobId);

    bool tryGetResult(int jobId, SelectionPatchResult& result);
    void deleteJob(int jobId);

signals:
    void patchReady(int jobId);

private:

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
    QHash<int, QFutureWatcher<SelectionPatchResult>*> mWatchers;
    QHash<int, SelectionPatchResult>                  mResults;
};

#endif // SELECTIONPATCHSYSTEM_H
