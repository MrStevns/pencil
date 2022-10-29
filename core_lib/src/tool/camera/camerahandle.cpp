#include "camerahandle.h"

CameraHandle::CameraHandle()
{

}

void CameraHandle::update(const QTransform& localCamT, const QTransform& viewT, const QRect& cameraRect, const QPointF& localRotationPoint)
{
    QTransform invertedLocalCamT = localCamT.inverted();
    QTransform invertedViewT = viewT.inverted();

    mCameraRect = invertedLocalCamT.mapRect(cameraRect);
    mCameraPolygon =  invertedLocalCamT.map(QPolygon(cameraRect));
    mRotationHandle = invertedLocalCamT.map(localRotationPoint);

    mWorldCameraRect = invertedViewT.mapRect(mCameraRect);
    mWorldCameraPolygon =  invertedViewT.map(mCameraPolygon);
    mWorldRotationHandle = invertedViewT.map(mRotationHandle);
}
