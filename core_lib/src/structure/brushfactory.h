#ifndef BRUSHFACTORY_H
#define BRUSHFACTORY_H

#include <QRadialGradient>


class QPixmap;
class QImage;

struct Brush {
    QImage* brushImage;

    /**
     * @brief Parameters could take in any kind of floating point value..
     * but it really should be used to map to different values
     * pressure
     *  - min, max, current
     * velocity
     *  - min, max, current
     * radius
     *  - min, max, current
     * scatter
     *  - min, max, current
     *  + density
     *   - min, max, current
     */
//    float parameters[];
    QColor color;
    float softness;
    float opacity;
    float brushWidth;
    float scatterAmount;
    float scatterDensity;
    float dabSpacing;
};


class BrushFactory
{
public:
    BrushFactory();
    ~BrushFactory();

    /**
     * @brief BrushFactory::createRadialImage, constructs a radial image
     * @param colour, takes a QColor object
     * @param radius, determiens the size of the image
     * @param offset, determines the softness of the brush
     * @param opacity, determines how opaque the brush will become
     * @return A QImage image
     */
    QImage* createRadialImage(QColor color, qreal radius, qreal offset, qreal opacity);

    /**
     * @brief BrushFactory::applySimpleNoise, applies simple 1 bit noise to image
     * @return A QImage image
     */
    void applySimpleNoise(QImage& image);

    /**
     * @brief BrushFactory::applyPerlinNoise, applies perlin noise to image
     * @param image, the image you want to apply to
     * @param freqX, gaussian frequency X
     * @param freqY, gaussian frequency Y
     * @param aggresion, how aggresive the noise should be
     * @param opacity, the opacity of the noise image
     */
    void applyPerlinNoise(QImage& image, double freqX, double freqY, uint aggresion, uint opacity);

private:

    void update();

    /**
     * @brief applyAlphaMask radial gradient mask to image
     * @param image, inout QImage that we paint on
     * @param radius, radius of the alpha
     * @param offset, offset of the alpha
     * @param opacity, opacity of the alpha
     */
    void applyAlphaMask(QImage& image, qreal radius, qreal offset, qreal opacity);


    QImage* mBrushImage = nullptr;
    QImage* mNoiseImage = nullptr;
    QImage* mAlphaMask = nullptr;

    QSize mOldBrushSize = QSize(0,0);
    uint mOldRadius = 0;

    qreal mOldOffset = 0;
    qreal mOffset = 0;


};

#endif // BRUSHFACTORY_H
