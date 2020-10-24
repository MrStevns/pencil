//#include "imagepainter.h"

//ImagePainter::ImagePainter(bool showTransformation, QTransform selectionTransform, QRect selectionRect, QTransform viewTransform)
//{
//    mSelectionTransform = selectionTransform;
//    mSelectionRect = selectionRect;
//    mShowTransformation = showTransformation;
//    mViewTransform = viewTransform;
//}

//void ImagePainter::paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QImage& imageToPaint, QPoint origin)
//{
//    paint(painter, visibleArea, viewTransform, imageToPaint, origin, false, 0, 0);
//}

//void ImagePainter::paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QImage& imageToPaint, QPoint origin, bool colorize, int currentFrame, int frameIteration)
//{
//    QPixmap colorDevice = QPixmap(visibleArea.size());
//    colorDevice.fill(Qt::transparent);
//    QPainter areaPainter(&colorDevice);

//    areaPainter.save();
//    areaPainter.setTransform(viewTransform);
//    areaPainter.translate(origin);
//    areaPainter.drawImage(QPoint(), imageToPaint);

//    if (mShowTransformation) {
//        drawTransformation(areaPainter, imageToPaint, origin);
//    }
//    areaPainter.restore();

//    if (colorize) {
//        QBrush colorBrush = Qt::NoBrush; // No color for the current frame

//        if (frameIteration < currentFrame)
//        {
//            colorBrush = QBrush(Qt::red);
//        }
//        else if (frameIteration > currentFrame)
//        {
//            colorBrush = QBrush(Qt::blue);
//        }
//        applyColorize(areaPainter, colorBrush);
//    }

//    QRectF croppedRect = visibleArea;
//    croppedRect = viewTransform.inverted().mapRect(croppedRect);

//    painter.setWorldMatrixEnabled(false);

//    painter.drawPixmap(QPoint(), colorDevice);
//}

//void ImagePainter::paint(QPainter& painter, QRect visibleArea, QTransform viewTransform, QPixmap& pixmap, QPoint origin)
//{
//    QPixmap colorDevice = QPixmap(painter.viewport().size());
//    colorDevice.fill(Qt::transparent);
//    QPainter areaPainter(&colorDevice);

//    areaPainter.setTransform(viewTransform);
//    areaPainter.translate(origin);
//    areaPainter.drawPixmap(QPoint(), pixmap, pixmap.rect());

//    QRectF croppedRect = visibleArea;
//    croppedRect = viewTransform.inverted().mapRect(croppedRect);

//    painter.setTransform(viewTransform);
//    painter.drawPixmap(croppedRect, colorDevice, colorDevice.rect());
//}

//void ImagePainter::applyColorize(QPainter& painter, QBrush brush)
//{
//    painter.setBrush(brush);
//    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
//    painter.drawRect(painter.viewport());
//}

//void ImagePainter::drawTransformation(QPainter& painter, QImage& image, QPoint origin)
//{
//    // Make sure there is something selected
//    if (mSelectionRect.width() == 0 && mSelectionRect.height() == 0)
//        return;

//    QImage tranformedImage = QImage(mSelectionRect.size(), QImage::Format_ARGB32_Premultiplied);
//    tranformedImage.fill(Qt::transparent);

//    QPainter imagePainter(&tranformedImage);
//    imagePainter.translate(-mSelectionRect.topLeft());
//    imagePainter.drawImage(origin, image);
//    imagePainter.end();

//    painter.save();

//    QTransform selectionTransform = mSelectionTransform;

//    QRect fillRect = mSelectionRect;

//    painter.setTransform(mViewTransform);

//    // Clear the painted area to make it look like the content has been erased
//    painter.save();
//    painter.setCompositionMode(QPainter::CompositionMode_Clear);
//    painter.fillRect(fillRect, QColor(255,255,255,255));
//    painter.restore();

//    // Multiply the selection and view matrix to get proper rotation and scale values
//    // Now the image origin will be topleft
//    painter.setTransform(mSelectionTransform*mViewTransform);

//    // Draw the selection image separately and on top
//    painter.drawImage(mSelectionRect, tranformedImage);
//    painter.restore();
//}
