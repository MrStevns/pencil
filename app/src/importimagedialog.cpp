#include "importimagedialog.h"
#include "ui_importimagedialog.h"

#include <QImage>
#include <QCheckBox>
#include <QPainter>
#include <QSlider>
#include <QDebug>
#include <QProgressDialog>
#include <QSlider>
#include <QScroller>

#include "bitmapimage.h"
#include "editor.h"

ImportImageDialog::ImportImageDialog(QDialog* parent, enum FileType fileType)
    : QDialog(parent), mFileType(fileType)
{
    ui = new Ui::ImportImageDialog;
    ui->setupUi(this);

    mFileDialog = new FileDialog(this);
    ui->preview_imageLabel->setStyleSheet("QLabel { background-image: url(:background/checkerboard.png); background-repeat: repeat-xy; border: 1px solid lightGrey; }");
    ui->zoomSlider->setEnabled(false);
    ui->sizeToFitCheckbox->setEnabled(false);
    ui->zoomSpinbox->setSuffix("%");
    ui->thresholdSpinbox->setSuffix("%");

    QScroller* scroller = QScroller::scroller(ui->scrollArea_2);

    QScrollerProperties prop = QScroller::scroller(ui->scrollArea_2)->scrollerProperties();

    QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
    prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, overshootPolicy);
    prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, overshootPolicy);
    prop.setScrollMetric(QScrollerProperties::MaximumVelocity, 0);

    QScroller::scroller(ui->scrollArea_2)->setScrollerProperties(prop);
    scroller->setScrollerProperties(prop);
    scroller->grabGesture(ui->scrollArea_2, QScroller::LeftMouseButtonGesture);

    connect(ui->browseButton, &QPushButton::clicked, this, &ImportImageDialog::browse);
    connect(ui->applyChangesButton, &QPushButton::clicked, this, &ImportImageDialog::applyOptions);
    connect(ui->sizeToFitCheckbox, &QCheckBox::clicked, this, &ImportImageDialog::sizeToFit);
    connect(ui->zoomSlider, &QSlider::valueChanged, this, &ImportImageDialog::updateZoom);
    connect(ui->resetPreviewButton, &QPushButton::clicked, this, &ImportImageDialog::resetPreview);
    connect(ui->thresholdSpinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImportImageDialog::mapThreshold);

    Qt::WindowFlags eFlags = Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint;
    setWindowFlags(eFlags);
}

ImportImageDialog::~ImportImageDialog()
{
    delete ui;
}

void ImportImageDialog::sizeToFit()
{
    bool isChecked = ui->sizeToFitCheckbox->isChecked();
    if (isChecked) {
        ui->input_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        ui->preview_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    } else {
        ui->input_imageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        ui->preview_imageLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    ui->input_imageLabel->setScaledContents(isChecked);
    ui->preview_imageLabel->setScaledContents(isChecked);
}

void ImportImageDialog::updateZoom()
{

    Qt::TransformationMode transformMode = Qt::TransformationMode::SmoothTransformation;
    if (mPreviewImage)
    {
        ui->zoomSlider->setEnabled(true);
    } else {
        ui->zoomSlider->setEnabled(false);
    }

    int zoomValue = ui->zoomSlider->value();

    if (zoomValue > 100) {
        transformMode = Qt::TransformationMode::FastTransformation;
    }

    ui->zoomSpinbox->setValue(zoomValue);
    QSize size = mPreviewImage->bounds().size();

    float scaledWidth = zoomValue/100.f;
    float scaledHeight = zoomValue/100.f;
    scaledWidth = scaledWidth * size.width();
    scaledHeight = scaledHeight * size.height();
    QImage scaled = mPreviewImage->image()->scaled(scaledWidth, scaledHeight, Qt::KeepAspectRatio, transformMode);

    QPixmap pix2 = QPixmap(scaled.size());
    pix2.convertFromImage(scaled);

    ui->preview_imageLabel->setPixmap(pix2);
}

void ImportImageDialog::resetPreview()
{
    mPreviewImage = mInputImage->clone();
    QPixmap pix = QPixmap(mPreviewImage->size());
    pix.convertFromImage(*mPreviewImage->image());
    ui->preview_imageLabel->setPixmap(pix);
}

void ImportImageDialog::mapThreshold(int value)
{
    mThresholdValue = value * 2.55;
}

void ImportImageDialog::applyOptions()
{

//    QProgressDialog progress(tr("Applying options..."), tr("Abort"), 0, 100, this);

//    progress.setWindowModality(Qt::WindowModal);
//    progress.setMinimumDuration(500);
//    progress.show();
    BitmapImage* previewConverted = mInputImage->clone();;

    QImage image = previewConverted->image()->convertToFormat(QImage::Format_ARGB32);
    previewConverted->setImage(&image);
    mPreviewImage = previewConverted->applyTransparencyThreshold(previewConverted, mThresholdValue);

    BitmapImage* previewWithTrans = mPreviewImage;
    if (ui->blackWhiteCheckbox->isChecked()) {
        mPreviewImage = previewWithTrans->applyPixelatedLines(previewWithTrans);
    }

    BitmapImage* previewWithPixels = mPreviewImage->clone();
    if (ui->fillAreaSpinbox->value() > 0) {
//        mPreviewImage = previewWithPixels->applyExpandPixels(previewWithPixels, ui->fillAreaSpinbox->value());

        mEditor->fillWhiteAreas(previewWithPixels);
//        mPreviewImage = previewWithPixels;
    }

    QPixmap pix = QPixmap(image.size());
    pix.fill(Qt::transparent);


    QPainter p(&pix);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(0,0, *mPreviewImage->image());
//    p.setCompositionMode(QPainter::CompositionMode_DestinationOut);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(0,0, *previewWithPixels->image());
    p.end();

    pix.save("/Users/CandyFace/Desktop/pix.png");

//    QPixmap pix3 = QPixmap(image.size());
//    pix3.fill(Qt::transparent);

//    previewWithPixels->image()->save("/Users/CandyFace/Desktop/previewWithPixels.png");

//    QPainter p2(&pix3);

//    // TODO: figure out the right combination..
//    // we need to merge an inverted previewWithPixels with our final image
//    p2.setCompositionMode(QPainter::RasterOp_SourceOrDestination );
//    p2.drawImage(0,0, *previewWithPixels->image());
////    p2.setCompositionMode(QPainter::CompositionMode_Destination);
////    p2.drawImage(0,0, *mPreviewImage->image());

////    p2.setCompositionMode(QPainter::CompositionMode_DestinationOut);
//    p2.end();

//    pix3.save("/Users/CandyFace/Desktop/pix3.png");

//    QPainter finalPaint(&pix);
//    finalPaint.setCompositionMode(QPainter::CompositionMode_SourceOver);
//    finalPaint.drawPixmap(0,0, pix3);
////        pfinalPaint2.setCompositionMode(QPainter::CompositionMode_DestinationOut);
////    finalPaint.drawImage(0,0, *previewWithPixels->image());
//    finalPaint.end();

//    mPreviewImage->image()->save("/Users/CandyFace/Desktop/Fisk.png");

    ////    qDebug() << mConvertedPreview->image()->format();
    qDebug() << mPreviewImage->image()->format();

//    mPreviewImage->image()->save("/Users/CandyFace/Desktop/bw.png");
//    progress.setValue(50);
    QPixmap pix2 = pix;
//    pix2.save("/Users/CandyFace/Desktop/hmm.png");
//    pix2.convertFromImage(*mPreviewImage->image());

    ui->preview_imageLabel->setPixmap(pix2);
    ui->preview_imageLabel->update();
//    ui->preview_imageLabel->repaint();
//    progress.setValue(100);
//    progress.close();
}

void ImportImageDialog::browse()
{
    QStringList filePaths;
    filePaths = QStringList(mFileDialog->openFiles(mFileType));

    if (filePaths.isEmpty() || filePaths.first().isEmpty())
    {
        return;
    }

    mFilePaths = filePaths;
    ui->fileEdit->setText("\"" + filePaths.join("\" \"") + "\"");

    if (!filePaths.isEmpty()) {
        loadImageFromPath(filePaths.first());
    }
}

void ImportImageDialog::loadImageFromPath(QString path)
{
    mInputImage = new BitmapImage(ui->input_imageLabel->rect().topLeft(), path);
    mPreviewImage = new BitmapImage(ui->input_imageLabel->rect().topLeft(), path);

//    image = QImage(path);
//    mInputImage->setImage(&image);
//    mConvertedPreview = new BitmapImage(ui->input_imageLabel->rect().topLeft(),path);

    QPixmap pix = QPixmap(mInputImage->size());
    pix.convertFromImage(*mInputImage->image());

    QPixmap pix2 = QPixmap(mPreviewImage->size());
    pix2.convertFromImage(*mPreviewImage->image());

    ui->input_imageLabel->setPixmap(pix);
    ui->preview_imageLabel->setPixmap(pix2);

    ui->sizeToFitCheckbox->setEnabled(true);
    ui->zoomSlider->setEnabled(true);
//    ui->zoomSlider->setValue(50);
}
