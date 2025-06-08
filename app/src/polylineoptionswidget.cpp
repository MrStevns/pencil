/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang
Copyright (C) 2025-2099 Oliver S. Larsen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include "polylineoptionswidget.h"
#include "ui_polylineoptionswidget.h"

#include "polylinetool.h"
#include "editor.h"
#include "toolmanager.h"

PolylineOptionsWidget::PolylineOptionsWidget(Editor* editor, QWidget* parent)
    : BaseWidget(parent),
      ui(new Ui::PolylineOptionsWidget),
      mEditor(editor)
{
    ui->setupUi(this);

    initUI();
}

PolylineOptionsWidget::~PolylineOptionsWidget()
{
    delete ui;
}

void PolylineOptionsWidget::initUI()
{
    mPolylineTool = static_cast<PolylineTool*>(mEditor->tools()->getTool(POLYLINE));
    const PolylineSettings* p = static_cast<const PolylineSettings*>(mPolylineTool->settings());

    auto widthInfo = p->getInfo(PolylineSettings::WIDTH_VALUE);
    ui->sizeSlider->init(tr("Width"), SpinSlider::EXPONENT, widthInfo.minReal(), widthInfo.maxReal());

    makeConnectionFromUIToModel();
    makeConnectionFromModelToUI();
}

void PolylineOptionsWidget::updateUI()
{
    const PolylineSettings* p = static_cast<const PolylineSettings*>(mPolylineTool->settings());
    if (mPolylineTool->isPropertyEnabled(PolylineSettings::WIDTH_VALUE))
    {
        PropertyInfo info = p->getInfo(PolylineSettings::WIDTH_VALUE);
        QSignalBlocker b(ui->sizeSlider);
        ui->sizeSlider->setRange(info.minReal(), info.maxReal());
        QSignalBlocker b2(ui->sizeSpinBox);
        ui->sizeSpinBox->setRange(info.minReal(), info.maxReal());

        setWidthValue(info.realValue());
    }

    if (mPolylineTool->isPropertyEnabled(PolylineSettings::ANTI_ALIASING_ENABLED)) {
        setAntiAliasingEnabled(p->AntiAliasingEnabled());
    }

    if (mPolylineTool->isPropertyEnabled(PolylineSettings::BEZIERPATH_ENABLED)) {
        setBezierPathEnabled(p->bezierPathEnabled());
    }

    if (mPolylineTool->isPropertyEnabled(PolylineSettings::CLOSEDPATH_ENABLED)) {
        setClosedPathEnabled(p->closedPathEnabled());
    }
}

void PolylineOptionsWidget::makeConnectionFromModelToUI()
{
    connect(mPolylineTool, &PolylineTool::widthChanged, this, &PolylineOptionsWidget::setWidthValue);
    connect(mPolylineTool, &PolylineTool::antiAliasingEnabledChanged, this, &PolylineOptionsWidget::setAntiAliasingEnabled);
    connect(mPolylineTool, &PolylineTool::bezierPathEnabledChanged, this, &PolylineOptionsWidget::setBezierPathEnabled);
    connect(mPolylineTool, &PolylineTool::closePathChanged, this, &PolylineOptionsWidget::setClosedPathEnabled);
}

void PolylineOptionsWidget::makeConnectionFromUIToModel()
{
    auto spinboxValueChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);
    connect(ui->useBezierBox, &QCheckBox::clicked, [=](bool enabled) {
        mPolylineTool->setUseBezier(enabled);
    });

    connect(ui->useClosedPathBox, &QCheckBox::clicked, [=](bool enabled) {
        mPolylineTool->setClosePath(enabled);
    });

    connect(ui->sizeSlider, &SpinSlider::valueChanged, [=](qreal value) {
        mPolylineTool->setWidth(value);
    });

    connect(ui->sizeSpinBox, spinboxValueChanged, [=](qreal value) {
        mPolylineTool->setWidth(value);
    });

    connect(ui->useAABox, &QCheckBox::clicked, [=](bool enabled) {
        mPolylineTool->setAntiAliasingEnabled(enabled);
    });
}

void PolylineOptionsWidget::setWidthValue(qreal width)
{
    QSignalBlocker b(ui->sizeSlider);
    ui->sizeSlider->setValue(width);

    QSignalBlocker b2(ui->sizeSpinBox);
    ui->sizeSpinBox->setValue(width);
}

void PolylineOptionsWidget::setAntiAliasingEnabled(bool enabled)
{
    QSignalBlocker b(ui->useAABox);
    ui->useAABox->setChecked(enabled);
}

void PolylineOptionsWidget::setBezierPathEnabled(bool enabled)
{
    QSignalBlocker b(ui->useBezierBox);
    ui->useBezierBox->setChecked(enabled);
}

void PolylineOptionsWidget::setClosedPathEnabled(bool enabled)
{
    QSignalBlocker b(ui->useClosedPathBox);
    ui->useClosedPathBox->setChecked(enabled);
}
