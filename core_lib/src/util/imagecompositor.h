#ifndef IMAGECOMPOSITOR_H
#define IMAGECOMPOSITOR_H

#include <QImage>
#include <QPainter>

enum class CompositeEffect {
    Colorize,
    Transformation,
};

class ImageCompositor
{
public:
    ImageCompositor(QRect compositorRect, QPoint origin, QTransform compositorView);

    /** addImage
     * Add an image to the compositor
     * @param image
     */
    void addImage(QImage& image, QPainter::CompositionMode compositionMode = QPainter::CompositionMode_SourceOver);

    /** addEffect
     * Add a colorEffect to the compositor
     * @param effect
     * @param effectColor
     */
    void addEffect(CompositeEffect effect, QColor effectColor);

    /** addEffect
     * Add a transform effect to the compositor
     * @param effect
     * @param effectTransform
     * @param selection
     */
    void addEffect(CompositeEffect effect, QTransform effectTransform, QRect selection);

    /** output
     *
     * @return The output of the composited image
     */
    const QImage& output() const { return mOutputImage; }
    QImage& output() { return mOutputImage; }

private:

    void addColorizeEffect(QPainter& effectPainter, QBrush brush);
    void addTransformationEffect(QPainter& effectPainter, QTransform effectTransform, QRect selection);
    QTransform mCanvasTransform;
    QImage mOutputImage;

    QList<QImage> mComposedImages;
    QPoint mOutputOrigin;
};

#endif // IMAGECOMPOSITOR_H
