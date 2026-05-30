#include "selectionpatchsystem.h"

#include <QtConcurrent>
#include <QPainter>

void SelectionPatchSystem::requestPatch(const SelectionPatchJob &job)
{
    deleteJob(job.jobId);

    auto future = QtConcurrent::run([job, this]() -> SelectionPatchResult {

        QRect transformedAlignedRect;
        QRectF transformedPreciseRect;

        computeTransformedImageBounds(job.baseCacheBounds, job.transform, transformedAlignedRect, transformedPreciseRect);

        QRect patchBounds = job.baseCacheBounds
            .united(transformedAlignedRect)
            .adjusted(-job.padding, -job.padding,
                       job.padding,  job.padding);

        QImage patch(patchBounds.size(), QImage::Format_ARGB32_Premultiplied);
        patch.fill(Qt::transparent);

        QPainter p(&patch);
        p.translate(-patchBounds.topLeft());

        // draw the backing image, this needs to be done to avoid showing
        // seams in the rendered result.
        QRect visibleWorld = job.backingImageBounds.intersected(patchBounds);
        if (visibleWorld.isValid()) {
            QRect src = visibleWorld.translated(-job.backingImageBounds.topLeft());
            QRect dst(visibleWorld.topLeft(), visibleWorld.size());
            p.drawImage(dst, job.backingImage, src);
        }

        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.drawImage(job.baseCacheBounds, job.baseCache);

        // TODO: can we cache the transformed image so it only needs to be created when scaling the image?
        QImage transformedImage = subPixelTransformedImage(job.baseCache,
                                                           job.transform,
                                                           transformedAlignedRect,
                                                           transformedPreciseRect,
                                                           job.smoothTransform);

        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.drawImage(transformedAlignedRect, transformedImage);
        p.end();

        return { job.jobId, patch, patchBounds, transformedImage, transformedAlignedRect };
    });

    auto* watcher = new QFutureWatcher<SelectionPatchResult>(this);
    connect(watcher, &QFutureWatcher<SelectionPatchResult>::finished,
            this, [this, watcher]() {
                SelectionPatchResult result = watcher->result();
                mResults[result.jobId] = std::move(result);
                delete mWatchers.take(result.jobId);
                emit patchReady(result.jobId);
            });
    watcher->setFuture(future);
    mWatchers[job.jobId] = watcher;
}

bool SelectionPatchSystem::tryGetResult(int jobId, SelectionPatchResult& result)
{
    if (!mResults.contains(jobId))
        return false;
    result  = mResults[jobId];
    return true;
}

void SelectionPatchSystem::deleteJob(int jobId)
{
    if (mWatchers.contains(jobId)) {
        mWatchers[jobId]->cancel();
        mWatchers[jobId]->waitForFinished();
        delete mWatchers.take(jobId);
    }
}

void SelectionPatchSystem::cancelAndRemove(int jobId) {
    deleteJob(jobId);
    mResults.remove(jobId);
}

QImage SelectionPatchSystem::subPixelTransformedImage(const QImage& src,
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

    painter.setTransform(transform, true);

    // Calculates the sub pixel position offset in order to account for the image being integer based.
    QPointF pixelCorrectionOffset = preciseRect.topLeft() - alignedRect.topLeft();
    QPointF centerInSource = transform.inverted().map(preciseCenter + pixelCorrectionOffset);
    QPointF copiedCenter(src.width() * 0.5, src.height() * 0.5);
    QPointF drawPoint = centerInSource - copiedCenter;

    painter.drawImage(drawPoint, src);
    painter.end();

    return result;
}

void SelectionPatchSystem::computeTransformedImageBounds(const QRect& sourceBounds,
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
