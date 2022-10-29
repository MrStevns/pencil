#ifndef CAMERAHANDLE_H
#define CAMERAHANDLE_H

#include <QTransform>
#include <QPolygonF>

class CameraHandle
{
public:
    CameraHandle();

    QPolygonF worldCorners() const { return mCameraPolygon; }
    QPointF worldRotationPoint() const { return mRotationHandle; }
    QRectF worldRect() const { return mWorldCameraRect; }

    QPolygon cameraProjectedCorners() const { return mCameraPolygon; }
    QPointF cameraProjectedRotationPoint() const { return mRotationHandle; }
    QRectF cameraProjectedRect() { return mCameraRect; }

    void update(const QTransform& localCamT, const QTransform& viewT, const QRect& cameraRect, const QPointF& localRotationPoint);

private:
    QPolygonF mWorldCameraPolygon;
    QPointF mWorldRotationHandle;
    QRectF mWorldCameraRect;

    QRect mCameraRect;
    QPolygon mCameraPolygon;
    QPointF mRotationHandle;
};

#endif // CAMERAHANDLE_H
