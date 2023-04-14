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
    ImageCompositor(QRect bounds = QRect());

    /** initialize
     *  Get ready to clear the area again
     *  @param blitRect the area that will be cleared for repainting
     *  @param origin the origin of the painter
     *  @param the transform of the painter
     */
    void initialize(QRect blitRect, QPoint origin, QTransform transform);

    /** addImage
     * Add an image to the compositor
     * @param image
     */
    void addImage(const QImage& image, QPainter::CompositionMode compositionMode = QPainter::CompositionMode_SourceOver);

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
    const QPixmap& output() const { return mOutputPixmap; }
    QPixmap& output() { return mOutputPixmap; }

private:

    void addColorizeEffect(QPainter& effectPainter, QBrush brush);
    void addTransformationEffect(QPainter& effectPainter, QTransform effectTransform, QRect selection);
    QTransform mCanvasTransform;
    QPixmap mOutputPixmap;

    QList<QImage> mComposedImages;
    QPoint mOutputOrigin;
};

#endif // IMAGECOMPOSITOR_H
