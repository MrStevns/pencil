#ifndef SELECTIONPATCHSYSTEM_H
#define SELECTIONPATCHSYSTEM_H

#include <QFuture>
#include <QFutureWatcher>
#include <QImage>
#include <QRect>
#include <QHash>

struct SelectionPatchJob {
    int    jobId;
    QImage backingCopy;
    QImage baseCache;
    QImage transformedImage;
    QRect  baseBounds;
    QRect  transformedBounds;
    QRect  imageBounds;
    int    padding;
};

struct SelectionPatchResult {
    int    jobId;
    QImage patch;
    QRect  bounds;
};

class SelectionPatchSystem : public QObject {
    Q_OBJECT
public:
    int generateId() {
        return mNextId++;
    }

    void requestPatch(const SelectionPatchJob& job);

    void cancelAndRemove(int jobId);

    bool tryGetResult(int jobId, QImage& patch, QRect& bounds);
    void deleteJob(int jobId);

signals:
    void patchReady(int jobId);

private:
    int mNextId = 0;
    QHash<int, QFutureWatcher<SelectionPatchResult>*> mWatchers;
    QHash<int, SelectionPatchResult>                  mResults;
};

#endif // SELECTIONPATCHSYSTEM_H
