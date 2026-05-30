#include "bitmapselectionrenderworker.h"

#include <QtConcurrent>
#include <QPainter>

void BitmapSelectionRenderWorker::requestJob(const BitmapSelectionRenderJob& job)
{
    // Rather than discarding the current job, wait for the existing one to finish,
    // so we can render the intermediate result while we wait
    if (mWatchers.contains(job.jobId)) {
        mPendingJobs[job.jobId] = job;
        return;
    }

    startJob(job);
}

void BitmapSelectionRenderWorker::startJob(const BitmapSelectionRenderJob& job)
{
    auto future = QtConcurrent::run([job, this]() -> BitmapSelectionRenderResult {

        QRect transformedAlignedRect;
        QRectF transformedPreciseRect;

        computeTransformedImageBounds(job.baseCacheBounds, job.transform, transformedAlignedRect, transformedPreciseRect);

        QImage transformedImage = subPixelTransformedImage(job.baseCache,
                                                           job.transform,
                                                           transformedAlignedRect,
                                                           transformedPreciseRect,
                                                           job.smoothTransform);

        return { job.jobId, transformedImage, transformedAlignedRect };
    });

    auto* watcher = new QFutureWatcher<BitmapSelectionRenderResult>(this);
    connect(watcher, &QFutureWatcher<BitmapSelectionRenderResult>::finished,
            this, [this, watcher]() {
                BitmapSelectionRenderResult result = watcher->result();
                mResults[result.jobId] = std::move(result);
                delete mWatchers.take(result.jobId);
                emit jobDone(result.jobId);

                // Always ensure we render the latest result
                if (mPendingJobs.contains(result.jobId)) {
                    startJob(mPendingJobs.take(result.jobId));
                }

            });
    watcher->setFuture(future);
    mWatchers[job.jobId] = watcher;
}

bool BitmapSelectionRenderWorker::tryGetResult(int jobId, BitmapSelectionRenderResult& result)
{
    if (!mResults.contains(jobId)) { return false; }

    result  = mResults[jobId];
    return true;
}

void BitmapSelectionRenderWorker::deleteJob(int jobId)
{
    mPendingJobs.remove(jobId);
    if (mWatchers.contains(jobId)) {
        mWatchers[jobId]->cancel();
        mWatchers[jobId]->waitForFinished();
        delete mWatchers.take(jobId);
    }
}

void BitmapSelectionRenderWorker::cancelAndRemove(int jobId)
{
    deleteJob(jobId);
    mResults.remove(jobId);
}

QImage BitmapSelectionRenderWorker::subPixelTransformedImage(const QImage& src,
                                               const QTransform& transform,
                                               const QRect& alignedRect,
                                               const QRectF& preciseRect,
                                               bool smooth) const
{
    QImage result(QSize(alignedRect.width(), alignedRect.height()),
                  QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    if (smooth) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);
    }

    QPointF preciseCenter(preciseRect.width() * 0.5, preciseRect.height() * 0.5);

    painter.setTransform(transform);

    // Calculates the sub pixel position offset in order to account for the image being integer based.
    QPointF pixelCorrectionOffset = preciseRect.topLeft() - alignedRect.topLeft();
    QPointF centerInSource = transform.inverted().map(preciseCenter + pixelCorrectionOffset);
    QPointF copiedCenter(src.width() * 0.5, src.height() * 0.5);
    QPointF drawPoint = centerInSource - copiedCenter;

    painter.drawImage(drawPoint, src);
    painter.end();

    return result;
}

void BitmapSelectionRenderWorker::computeTransformedImageBounds(const QRect& sourceBounds,
                                   const QTransform& transform,
                                   QRect& outAlignedRect, QRectF& outPreciseRect) const
{
    QPolygon boundsPolygon(sourceBounds);

    QRectF boundingRect = transform.map(boundsPolygon).boundingRect();

    outPreciseRect = transform.map(QPolygonF(boundsPolygon)).boundingRect();

    // The aligned rect is calculated such that it accounts for transformations
    // where the image might otherwise have been slightly larger.
    outAlignedRect = QRect(
        qFloor(boundingRect.x() - 1),
        qFloor(boundingRect.y() - 1),
        qCeil(boundingRect.width() + 1),
        qCeil(boundingRect.height() + 1)
    );
}
