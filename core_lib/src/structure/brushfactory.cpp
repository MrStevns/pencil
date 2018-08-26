#include "brushfactory.h"

#include <QPainter>
#include <QPixmap>
#include <QImage>
#include <QRgb>
#include <qmath.h>

#include "bitmapimage.h"

#include <util.h>

#include <QDebug>

BrushFactory::BrushFactory()
{
}

BrushFactory::~BrushFactory()
{
    mBrushImage = nullptr;
    mNoiseImage = nullptr;
    mAlphaMask = nullptr;
}

void BrushFactory::update()
{
    mOldBrushSize = mBrushImage->size();
    mOldOffset = mOffset;
}

QImage* BrushFactory::createRadialImage(Brush* brush, QRgb surfaceColor)
{

    mOffset = brush->softness;
    mBrushImage = new QImage(brush->brushWidth,brush->brushWidth, QImage::Format_ARGB32_Premultiplied);

    // fill with transparency to avoid artifcats
    mBrushImage->fill(QColor(0,0,0,0));

    QRgb brushColor = brush->fillColor.rgba();
    QColor rgbBlended = blendBetween(brushColor, surfaceColor, brush->fgBlendFac, brush->bgBlendFac);

    mBrushImage->fill(rgbBlended);

//    qDebug() << "mbrushImage is: " << mBrushImage->isNull();
    applyAlphaMask(*mBrushImage, brush->brushWidth, brush->softness, brush->opacity);

    return mBrushImage;
}

QRgb BrushFactory::getAverageColorOfImage(QImage& surfaceImage, QColor brushColor) {

    int red = 0;
    int blue = 0;
    int green = 0;
    int alpha = 0;

    // finds average color value...
    int pixelCount = surfaceImage.width()*surfaceImage.height();

    const QRgb* end = (const QRgb*)surfaceImage.bits() + surfaceImage.width()*surfaceImage.height();

    for (QRgb* rgb = (QRgb*)surfaceImage.bits(); rgb != end; rgb++){
//        *rgb = qRgb(rgb[0],rgb[1],rgb[2]);

        if (qAlpha(rgb[0]) == 0 && qRed(rgb[1]) == 0 && qGreen(rgb[2]) == 0 && qBlue(rgb[3]) == 0) {

            red += 255*255;
            green += 255*255;
            blue += 255*255;
            alpha += 255*255;
        } else  {
            red += qRed(rgb[0]) * qRed(rgb[0]);
            green += qGreen(rgb[1]) * qGreen(rgb[1]);
            blue += qBlue(rgb[2]) * qBlue(rgb[2]);
            alpha += qAlpha(rgb[3]) * qAlpha(rgb[3]);

        }
    }

    QColor rgb = qRgba(qSqrt(red/pixelCount),qSqrt(green/pixelCount),qSqrt(blue/pixelCount),qSqrt(alpha/pixelCount));


    // we're only interested in the saturation here...
//    rgb = rgb.fromHsv(brushColor.hue(), rgb.hsvSaturation(), brushColor.value());

    return rgb.rgba();
}

QRgb BrushFactory::colorMean(QRgb color1, QRgb color2)
{
    uint red = 0;
    uint green = 0;
    uint blue = 0;

    red = qRed(color1) * qRed(color1) + qRed(color2) * qRed(color2);
    green = qGreen(color1) * qGreen(color1) + qGreen(color2) * qGreen(color2);
    blue = qBlue(color1) * qBlue(color1) + qBlue(color2) * qBlue(color2);


    return qRgb(qSqrt(red/2),qSqrt(green/2),qSqrt(blue/2));
}

QColor BrushFactory::blendBetween(QRgb foreground, QRgb background, float fac1, float fac2)
{

    // α blend value
    // rgb1 - foreground color
    // (1-x) - midway blend value
    // rgb2 - background color
    //
    // formula
    // α * rgb1 + (1-x) * rgb2 = rgb3

    int redBlend;
    int greenBlend;
    int blueBlend;
    int alphaBlend;

    redBlend = fac1*qRed(foreground)+qRed(background)*(1-fac2);
    greenBlend = fac1*qGreen(foreground)+qGreen(background)*(1-fac2);
    blueBlend =  fac1*qBlue(foreground)+qBlue(background)*(1-fac2);
    alphaBlend = fac1*qAlpha(foreground)+qAlpha(background)*(1-fac2);

    if (redBlend > 255) {
        redBlend = 255;
    } else if (redBlend < 0) {
        redBlend = 0;
    }

    if (greenBlend > 255) {
        greenBlend = 255;
    } else if (greenBlend < 0) {
        greenBlend = 0;
    }

    if (blueBlend > 255) {
        blueBlend = 255;
    } else if (blueBlend < 0 ) {
        blueBlend = 0;
    }

    if (alphaBlend > 255) {
        alphaBlend = 255;
    } else if (alphaBlend < 0 ) {
        alphaBlend = 0;
    }

    return qRgba(redBlend, greenBlend, blueBlend, alphaBlend);
}

void BrushFactory::applyAlphaMask(QImage& image, qreal radius, qreal offset, qreal opacity)
{


//    opacity = 1.0;
    // invalidate the image every time we modify our brush
    if (mBrushImage->size() != mOldBrushSize || offset != mOldOffset) {
        mAlphaMask = nullptr;
    }

    QPainter painter;
    if (mAlphaMask == nullptr) {

//        qDebug() << "size was updated, creating new mask";
        qDebug() << "alpha is null, creating new mask";
        mAlphaMask = new QImage(radius,radius, QImage::Format_Alpha8);
        mAlphaMask->fill(0);
        QRadialGradient gradient(QPoint(0.5*radius,0.5*radius), 0.5 * radius);

        if (offset < 0) { offset = 0; }
        if (offset > 100) { offset = 100; }

        uint a = 255;

        uint mainColorAlpha = qRound(a * opacity);

        uint alphaAdded = qRound((mainColorAlpha * offset) / 100);

        gradient.setColorAt(0.0, QColor(255, 255, 255, mainColorAlpha - alphaAdded));
        gradient.setColorAt(1.0, QColor(255, 255, 255, 0));
        gradient.setColorAt(1.0 - (offset / 100.0), QColor(255, 255, 255, mainColorAlpha - alphaAdded));

        painter.begin(mAlphaMask);

        painter.setBrush(QBrush(gradient));
        painter.setOpacity(opacity);
        painter.setPen(Qt::NoPen);
        painter.drawRect(QRect(0,0,radius,radius));
        painter.end();
    }

//    qDebug() << "alphaMask is: " << mAlphaMask->isNull();

//    QPainter painter;
//    painter.begin(mAlphaMask);

//    painter.setBrush(QBrush(gradient));
//    painter.setPen(Qt::NoPen);
//    painter.drawRect(QRect(0,0,radius,radius));
//    painter.end();

//    alphaMask.save("/Users/CandyFace/desktop/alphaMask.png");

    painter.begin(&image);

    painter.setPen(Qt::NoPen);
    painter.setCompositionMode(QPainter::CompositionMode_DestinationAtop);
    painter.drawImage(0,0, *mAlphaMask);
    painter.end();

    update();
}

void BrushFactory::applySimpleNoise(QImage& image)
{
    srand(time(0));
    int w = mBrushImage->width();
    int h = mBrushImage->height();

    if (mBrushImage->size() != mOldBrushSize) {
        mNoiseImage = nullptr;
    }

    if (mNoiseImage == nullptr) {


        mNoiseImage = new QImage(w,h,QImage::Format_ARGB32_Premultiplied);
        mNoiseImage->fill(Qt::black);
        QRgb* pixel;

        for(int j=0; j<h; j++) {
            pixel = (QRgb*)mNoiseImage->constScanLine(j);
            for(int i=0; i<w; i++) {
                if((rand()%2) == 0) {
                    pixel[i] = qRgba(0,0,0,0);
                }
            }
        }
    }

    QPainter painter(&image);

    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.drawImage(QRect(0,0,w,h),*mNoiseImage);

    update();
}

void BrushFactory::applyPerlinNoise(QImage& image, double freqX, double freqY, uint aggresion = 1, uint opacity = 255)
{

    Q_UNUSED(opacity);
    PerlinNoise perlin;

    const double fx = image.width() / freqX;
    const double fy = image.height() / freqY;

    const int w = image.width();
    const int h = image.height();

    if (mBrushImage->size() != mOldBrushSize) {
        mNoiseImage = nullptr;
    }

    if (mNoiseImage == nullptr) {
        qDebug() << "new perlin noise";
        mNoiseImage = new QImage(w,h,QImage::Format_ARGB32_Premultiplied);
        mNoiseImage->fill(0);

        QRgb* pixel;
        for (int j = 0; j < h; ++j) { // y
            for (int i = 0; i < w; ++i) { // x
                pixel = (QRgb*)mNoiseImage->constScanLine(j)+i;

    //            double x = (double)i/((double)image.width());
    //            double y = (double)j/((double)image.height());

                double n = aggresion * perlin.noise(j/fx, i/fy);
//                n = n - qFloor(n);

                if (n <= 0) {
                    n = 0;
                }

                if (n >= 1) {
                    n = 1;
                }

                *pixel = qRgba(
                            0,
                            0,
                            0,
                            qFloor(255*n));
            }
        }
    }

    QPainter painter(&image);

    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);

    painter.drawImage(QRect(0,0,w,h), *mNoiseImage);
    painter.end();

    update();

}
