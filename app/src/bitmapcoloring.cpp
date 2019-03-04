/*

Pencil - Traditional Animation Software
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "bitmapcoloring.h"
#include "ui_bitmapcoloringwidget.h"
#include "layermanager.h"
#include "scribblearea.h"

BitmapColoring::BitmapColoring(Editor* editor, QWidget *parent) :
    BaseDockWidget(parent)
{
    QWidget* innerWidget = new QWidget;
    setWindowTitle(tr("Bitmap Coloring"));

    ui = new Ui::BitmapColoringWidget;
    ui->setupUi(innerWidget);
    setWidget(innerWidget);

    mEditor = editor;
    if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP)
        mLayerBitmap = static_cast<LayerBitmap*>(mEditor->layers()->currentLayer());
    mBitmapImage = mLayerBitmap->getBitmapImageAtFrame(mEditor->currentFrame());
    ui->btnSelectAreas->setIcon(QIcon(":/icons/select.png"));

    connect(ui->cbLayerSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &BitmapColoring::colorMethodChanged);
    connect(ui->cb2TraceBlack, &QCheckBox::stateChanged, this, &BitmapColoring::checkBlackBoxes);
    connect(ui->cb2TraceRed, &QCheckBox::stateChanged, this, &BitmapColoring::checkRedBoxes);
    connect(ui->cb2TraceGreen, &QCheckBox::stateChanged, this, &BitmapColoring::checkGreenBoxes);
    connect(ui->cb2TraceBlue, &QCheckBox::stateChanged, this, &BitmapColoring::checkBlueBoxes);
    connect(ui->cb3TraceAllKeyframes, &QCheckBox::stateChanged, this, &BitmapColoring::checkAllKeyframes);

    // Prepare
    connect(ui->cb0Trace, &QCheckBox::stateChanged, this, &BitmapColoring::updateTraceBoxes);
    connect(ui->cb1Threshold, &QCheckBox::stateChanged, this, &BitmapColoring::updateBtnSelect);
    connect(ui->sb1Threshold, QOverload<int>::of(&QSpinBox::valueChanged), this, &BitmapColoring::setThreshold);
    connect(ui->btnSelectAreas, &QPushButton::clicked, this, &BitmapColoring::updateSelectButtonIcon);
    connect(ui->btnSelectNext, &QPushButton::clicked, this, &BitmapColoring::selectFromScans);
    connect(ui->btnSelectCancel, &QPushButton::clicked, this, &BitmapColoring::cancelSelectAreas);
    connect(ui->btnApplyPrepare, &QPushButton::clicked, this, &BitmapColoring::traceLines);
    // Thin
    connect(ui->cb1Thin, &QCheckBox::stateChanged, this, &BitmapColoring::updateThinBoxes);
    connect(ui->sbSpotAreas, QOverload<int>::of(&QSpinBox::valueChanged), this, &BitmapColoring::setSpotArea);
    connect(ui->btnApplyThin, &QPushButton::clicked, this, &BitmapColoring::thinLines);
    // Finish
    connect(ui->cb1Finish, &QCheckBox::stateChanged, this, &BitmapColoring::updateFinishBoxes);
    connect(ui->btnApplyFinish, &QPushButton::clicked, this, &BitmapColoring::blendLines);

    updateUI();
}

BitmapColoring::~BitmapColoring()
{
    delete ui;
}

void BitmapColoring::initUI()
{
    updateUI();
}

void BitmapColoring::updateUI()
{
    if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP)
        mLayerBitmap = static_cast<LayerBitmap*>(mEditor->layers()->currentLayer());
    if (mLayerBitmap == nullptr) { return; }

    if (mEditor->layers()->currentLayer()->type() == Layer::BITMAP)
    {
        if (mLayerBitmap->getHasColorLayer())
        {
            ui->tabPrepare->setEnabled(true);
            ui->tabThinLine->setEnabled(false);
            ui->tabFinish->setEnabled(false);
        }
        else if (mLayerBitmap->getIsColorLayer())
        {
            ui->tabPrepare->setEnabled(false);
            ui->tabThinLine->setEnabled(true);
            ui->tabFinish->setEnabled(true);
        }
        else
        {
            ui->tabPrepare->setEnabled(true);
            ui->tabThinLine->setEnabled(true);
            ui->tabFinish->setEnabled(true);
        }
    }
    else
    {   // If it is not a Bitmap Layer - disable
        ui->tabPrepare->setEnabled(false);
        ui->tabThinLine->setEnabled(false);
        ui->tabFinish->setEnabled(false);
    }
}

void BitmapColoring::visibilityChanged(bool visibility)
{
    if (visibility)
        updateUI();
}

void BitmapColoring::colorMethodChanged()
{
    if (ui->cbLayerSelector->currentIndex() == 0)
    {
        ui->cb2TraceBlack->setChecked(false);
        ui->cb2TraceBlack->setEnabled(false);
        ui->cb2ThinBlack->setChecked(false);
        ui->cb2ThinBlack->setEnabled(false);
        ui->cb2FinishBlack->setChecked(false);
        ui->cb2FinishBlack->setEnabled(false);
    }
    else
    {
        ui->cb2TraceBlack->setEnabled(true);
        ui->cb2ThinBlack->setEnabled(true);
        ui->cb2FinishBlack->setEnabled(true);
    }
}

void BitmapColoring::updateSelectButtonIcon()
{
    if (mSelectAreas)
    {
        mSelectAreas = false;
        ui->btnSelectAreas->setIcon(QIcon(":/icons/select.png"));
    }
    else
    {
        mSelectAreas = true;
        ui->btnSelectAreas->setIcon(QIcon(":/icons/select_ok.png"));
    }
}

void BitmapColoring::checkBlackBoxes()
{
    ui->cb2ThinBlack->setChecked(ui->cb2TraceBlack->isChecked());
    ui->cb2FinishBlack->setChecked(ui->cb2TraceBlack->isChecked());
}

void BitmapColoring::checkRedBoxes()
{
    ui->cb2ThinRed->setChecked(ui->cb2TraceRed->isChecked());
    ui->cb2FinishRed->setChecked(ui->cb2TraceRed->isChecked());
}

void BitmapColoring::checkGreenBoxes()
{
    ui->cb2ThinGreen->setChecked(ui->cb2TraceGreen->isChecked());
    ui->cb2FinishGreen->setChecked(ui->cb2TraceGreen->isChecked());
}

void BitmapColoring::checkBlueBoxes()
{
    ui->cb2ThinBlue->setChecked(ui->cb2TraceBlue->isChecked());
    ui->cb2FinishBlue->setChecked(ui->cb2TraceBlue->isChecked());
}

void BitmapColoring::checkAllKeyframes()
{
    ui->cb3ThinAllKeyframes->setChecked(ui->cb3TraceAllKeyframes->isChecked());
    ui->cb3FinishAllKeyframes->setChecked(ui->cb3TraceAllKeyframes->isChecked());
}

void BitmapColoring::updateTraceBoxes()
{
    if (ui->cb0Trace->isChecked())
    {
        ui->gb2Prepare->setEnabled(true);
        if (ui->cbLayerSelector->currentIndex() == 0)
            ui->cb2TraceBlack->setEnabled(false);
    }
    else
    {
        ui->cb2TraceBlack->setChecked(false);
        ui->cb2TraceRed->setChecked(false);
        ui->cb2TraceGreen->setChecked(false);
        ui->cb2TraceBlue->setChecked(false);
        ui->gb2Prepare->setEnabled(false);
    }
}

void BitmapColoring::updateBtnSelect()
{
    if (ui->cb1Threshold->isChecked())
    {
        ui->btnSelectAreas->setEnabled(true);
    }
    else
    {
        ui->btnSelectAreas->setEnabled(false);
    }
}

void BitmapColoring::setThreshold(int threshold)
{
    mBitmapImage->setThreshold(threshold);
}

void BitmapColoring::selectAreas()
{
    ui->gb0Prepare->setEnabled(false);
    ui->gb1Prepare->setEnabled(false);
    ui->gb2Prepare->setEnabled(false);
    ui->gb3Prepare->setEnabled(false);
    ui->btnApplyPrepare->setEnabled(false);
    ui->btnSelectNext->setEnabled(true);
    ui->btnSelectCancel->setEnabled(true);
    mEditor->scrubTo(1);
}

void BitmapColoring::cancelSelectAreas()
{
    ui->gb0Prepare->setEnabled(true);
    ui->gb1Prepare->setEnabled(true);
    ui->gb2Prepare->setEnabled(true);
    ui->gb3Prepare->setEnabled(true);
    ui->btnApplyPrepare->setEnabled(true);
    ui->btnSelectNext->setEnabled(false);
    ui->btnSelectCancel->setEnabled(false);
}

void BitmapColoring::traceLines()
{
    if (ui->cb1Threshold->isChecked())
    {
        selectAreas();
    }
    else
    {
        prepareLines();
    }
}

void BitmapColoring::selectFromScans()
{
    ScribbleArea* scribble = mEditor->getScribbleArea();
    if (scribble->isSomethingSelected())
    {
        mEditor->copy();
        mEditor->layers()->currentLayer()->removeKeyFrame(mEditor->currentFrame());
        mEditor->layers()->currentLayer()->addNewKeyFrameAt(mEditor->currentFrame());
        mEditor->paste();
        mBitmapImage = mLayerBitmap->getBitmapImageAtFrame(mEditor->currentFrame());
        mBitmapImage = mBitmapImage->scanToTransparent(mBitmapImage,
                                                       true,
                                                       ui->cb2TraceRed->isChecked(),
                                                       ui->cb2TraceGreen->isChecked(),
                                                       ui->cb2TraceBlue->isChecked());
    }
    if (ui->cb0Trace->isChecked())
    {
        prepareLines();
    }
    mEditor->backup(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame(), tr("Select from scan"));
    if (mEditor->currentFrame() < mEditor->layers()->currentLayer()->getMaxKeyFramePosition())
    {
        mEditor->scrubNextKeyFrame();
    }
    else
    {
        cancelSelectAreas();
        ui->cb1Threshold->setChecked(false);
    }
}

void BitmapColoring::prepareLines()
{
    if (mLayerBitmap == nullptr) { return; }

    if (!ui->cb1Threshold->isChecked())
        ui->tabPrepare->setEnabled(false);

    // if a separate layer is needed, we make one
    LayerBitmap* colorLayer = nullptr;
    if (ui->cbLayerSelector->currentIndex() == 0)
    {           // if coloring is on same layer...
        colorLayer = mLayerBitmap;
    }
    else
    {           // if coloring is on separate layer...
        if (!mLayerBitmap->getHasColorLayer())
        {
            colorLayer = mEditor->layers()->createBitmapLayer(mLayerBitmap->name() + "_C");
            mLayerBitmap->setHasColorLayer(true);
            colorLayer->setIsColorLayer(true);
        }
        else {
            colorLayer = static_cast<LayerBitmap*>(mEditor->layers()->findLayerByName(mLayerBitmap->name() + "_C"));
        }
    }

    if (ui->cbLayerSelector->currentIndex() == 1)
        colorLayer->setVisible(false);
    int firstFrame = 1, lastFrame = 1;
    if (!ui->cb3TraceAllKeyframes->isChecked() || ui->cb1Threshold->isChecked())
    {
        firstFrame = mEditor->currentFrame();
        lastFrame = firstFrame;
    }
    else
    {
        lastFrame = mLayerBitmap->getMaxKeyFramePosition();
    }
    for (int i = firstFrame; i <= lastFrame; i++)
    {
        if (mLayerBitmap->keyExists(i))
        {
            mEditor->scrubTo(i);
            emit mEditor->updateTimeLine();
            if (ui->cbLayerSelector->currentIndex() == 1)
                mLayerBitmap->copyFrame(mLayerBitmap, colorLayer, i);
            colorLayer->getBitmapImageAtFrame(i)->traceLine(colorLayer->getBitmapImageAtFrame(i),
                                                            ui->cb2TraceBlack->isChecked(),
                                                            ui->cb2TraceRed->isChecked(),
                                                            ui->cb2TraceGreen->isChecked(),
                                                            ui->cb2TraceBlue->isChecked());
            mEditor->backup(mEditor->layers()->currentLayerIndex() ,mEditor->currentFrame(), tr("Preparelines"));
        }
    }
//    updateUI();
}

void BitmapColoring::updateThinBoxes()
{
    if (ui->cb1Thin->isChecked())
    {
        ui->gb2Thin->setEnabled(true);
        if (ui->cbLayerSelector->currentIndex() == 0)
            ui->cb2ThinBlack->setEnabled(false);
    }
    else
    {
        ui->cb2ThinBlack->setChecked(false);
        ui->cb2ThinRed->setChecked(false);
        ui->cb2ThinGreen->setChecked(false);
        ui->cb2ThinBlue->setChecked(false);
        ui->gb2Thin->setEnabled(false);
    }
}

void BitmapColoring::setSpotArea(int size)
{
    mBitmapImage->setSpotArea(size);
}

void BitmapColoring::thinLines()
{
    if (mLayerBitmap == nullptr) { return; }
    mBitmapImage = mLayerBitmap->getBitmapImageAtFrame(mEditor->layers()->currentLayer()->getMaxKeyFramePosition());

    ui->tabThinLine->setEnabled(false);

    int firstFrame = 1, lastFrame = 1;
    if (ui->cb3ThinAllKeyframes->isChecked())
    {
        lastFrame = mLayerBitmap->getMaxKeyFramePosition();
    }
    else
    {
        firstFrame = mEditor->currentFrame();
        lastFrame = firstFrame;
    }
    mBitmapImage->setSpotArea(ui->sbSpotAreas->value());
    for (int i = firstFrame; i <= lastFrame; i++)
    {
        if (mLayerBitmap->keyExists(i))
        {
            mEditor->scrubTo(i);
            mBitmapImage = mLayerBitmap->getBitmapImageAtFrame(i);
            if (ui->cbSpotAreas->isChecked())
            {
                mBitmapImage->fillSpotAreas(mLayerBitmap->getBitmapImageAtFrame(i));
            }

            if (ui->cb1Thin->isChecked())
            {
                mBitmapImage->toThinLine(mBitmapImage,
                                              ui->cb2ThinBlack->isChecked(),
                                              ui->cb2ThinRed->isChecked(),
                                              ui->cb2ThinGreen->isChecked(),
                                              ui->cb2ThinBlue->isChecked());
            }
            mEditor->backup(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame(), tr("Thin lines"));
        }
    }
    updateUI();
}

void BitmapColoring::updateFinishBoxes()
{
    if (ui->cb1Finish->isChecked())
    {
        ui->gb2Finish->setEnabled(true);
        if (ui->cbLayerSelector->currentIndex() == 0)
            ui->cb2FinishBlack->setEnabled(false);
    }
    else
    {
        ui->cb2FinishBlack->setChecked(false);
        ui->cb2FinishRed->setChecked(false);
        ui->cb2FinishGreen->setChecked(false);
        ui->cb2FinishBlue->setChecked(false);
        ui->gb2Finish->setEnabled(false);
    }
}

void BitmapColoring::blendLines()
{
    if (mLayerBitmap == nullptr) { return; }
    mBitmapImage = mLayerBitmap->getBitmapImageAtFrame(mEditor->layers()->currentLayer()->getMaxKeyFramePosition());

    QString orgName = mLayerBitmap->name();
    orgName.chop(2);
    LayerBitmap* artLayer = static_cast<LayerBitmap*>(mEditor->layers()->findLayerByName(orgName));

    ui->tabFinish->setEnabled(false);

    int firstFrame = 1, lastFrame = 1;
    if (ui->cb3FinishAllKeyframes->isChecked())
    {
        lastFrame = mLayerBitmap->getMaxKeyFramePosition();
    }
    else
    {
        firstFrame = mEditor->currentFrame();
        lastFrame = firstFrame;
    }
    for (int i = firstFrame; i <= lastFrame; i++)
    {
        if (mLayerBitmap->keyExists(i))
        {
            mEditor->scrubTo(i);
            if (ui->cb1Finish->isChecked())
            {
                mLayerBitmap->getBitmapImageAtFrame(i)->blendLines(mLayerBitmap->getBitmapImageAtFrame(i),
                                              ui->cb2FinishBlack->isChecked(),
                                              ui->cb2FinishRed->isChecked(),
                                              ui->cb2FinishGreen->isChecked(),
                                              ui->cb2FinishBlue->isChecked());
                if (ui->cbLayerSelector->currentIndex() == 1 && artLayer != nullptr)
                {
                    artLayer->getBitmapImageAtFrame(i)->blendLines(artLayer->getBitmapImageAtFrame(i),
                                                                   false, // don't mess with the original
                                                                   ui->cb2FinishRed->isChecked(),
                                                                   ui->cb2FinishGreen->isChecked(),
                                                                   ui->cb2FinishBlue->isChecked());
                }
                mEditor->backup(mEditor->layers()->currentLayerIndex(), mEditor->currentFrame(), tr("Blend lines"));
            }
        }
    }
}

