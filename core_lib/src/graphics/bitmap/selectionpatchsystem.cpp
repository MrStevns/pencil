#include "selectionpatchsystem.h"

#include <QtConcurrent>
#include <QPainter>

void SelectionPatchSystem::requestPatch(const SelectionPatchJob &job)
{
    deleteJob(job.jobId);

    auto future = QtConcurrent::run([job]() -> SelectionPatchResult {
        QRect patchBounds = job.baseBounds
            .united(job.transformedBounds)
            .adjusted(-job.padding, -job.padding,
                       job.padding,  job.padding);

        QImage patch(patchBounds.size(), QImage::Format_ARGB32_Premultiplied);
        patch.fill(Qt::transparent);
        QPoint offset = -patchBounds.topLeft();

        QPainter p(&patch);

        // draw the backing image, this needs to be done to avoid showing
        // seams in the rendered result.
        QRect visibleWorld = job.imageBounds.intersected(patchBounds);
        if (visibleWorld.isValid()) {
            QRect src = visibleWorld.translated(-job.imageBounds.topLeft());
            QRect dst(visibleWorld.topLeft() + offset, visibleWorld.size());
            p.drawImage(dst, job.backingCopy, src);
        }

        p.setCompositionMode(QPainter::CompositionMode_Clear);
        p.drawImage(job.baseBounds.topLeft() + offset, job.baseCache);

        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.drawImage(job.transformedBounds.topLeft() + offset, job.transformedImage);
        p.end();

        return { job.jobId, patch, patchBounds };
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

bool SelectionPatchSystem::tryGetResult(int jobId, QImage &patch, QRect &bounds)
{
    if (!mResults.contains(jobId))
        return false;
    patch  = mResults[jobId].patch;
    bounds = mResults[jobId].bounds;
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
