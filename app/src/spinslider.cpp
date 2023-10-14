/*

Pencil2D - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/

#include "spinslider.h"

#include <cmath>
#include <QLabel>
#include <QSlider>
#include <QGridLayout>
#include <QLocale>
#include <QDebug>
#include <QStyle>
#include <QEvent>


SpinSlider::SpinSlider(QWidget* parent) : QWidget(parent)
{}

void SpinSlider::init(QString text, GROWTH_TYPE type, VALUE_TYPE dataType, qreal min, qreal max)
{
    if (type == LOG)
    {
        // important! dividing by zero is not acceptable.
        Q_ASSERT_X(min > 0.0, "SpinSlider", "Real type value must larger than 0!!");
    }

    mValue = 1.0;
    mGrowthType = type;
    mValueType = dataType;
    mMin = min;
    mMax = max;

    mLabel = new QLabel(text + ": ");

    mSlider = new QSlider(Qt::Horizontal, this);
    mSlider->setMinimum(static_cast<int>(min));
    mSlider->setMaximum(static_cast<int>(max));
    mSlider->setMaximumWidth(500);

    QGridLayout* layout = new QGridLayout();
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);

    layout->addWidget(mLabel, 0, 0, 1, 1);
    layout->addWidget(mSlider, 1, 0, 1, 2);

    setLayout(layout);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    mSlider->installEventFilter(this);

    connect(mSlider, &QSlider::valueChanged, this, &SpinSlider::onSliderValueChanged);
    connect(mSlider, &QSlider::sliderReleased, this, &SpinSlider::onSliderReleased);
}

void SpinSlider::changeValue(qreal value)
{
    mValue = value;
    emit valueChanged(value);
}

void SpinSlider::onSliderValueChanged(int v)
{
    qreal value2 = 0.0;
    if (mGrowthType == LINEAR)
    {
        value2 = static_cast<qreal>(v);
    }
    else if (mGrowthType == LOG)
    {
        value2 = mMin * std::exp(v * std::log(mMax / mMin) / mSlider->maximum());
    }
    else if (mGrowthType == EXPONENT) {
        value2 = mMin + std::pow(v, mExp) * (mMax - mMin) / std::pow(mSlider->maximum(), mExp);
    }

    changeValue(value2);
}

void SpinSlider::onSliderReleased()
{
    emit valueOnRelease(mValue);
}

void SpinSlider::setLabel(QString newText)
{
    mLabel->setText(newText);
}

void SpinSlider::setRange(qreal min, qreal max)
{
    mMin = min;
    mMax = max;
}

void SpinSlider::setValue(qreal v)
{
    int value2 = 0;
    if (mGrowthType == LINEAR)
    {
        value2 = static_cast<int>(v);
    }
    else if (mGrowthType == LOG)
    {
        value2 = qRound(std::log(v / mMin) * mSlider->maximum() / std::log(mMax / mMin));
    }
    else if (mGrowthType == EXPONENT)
    {
        value2 = qRound(std::pow((v - mMin) * std::pow(mSlider->maximum(), mExp) / (mMax - mMin), 1 / mExp));
    }

    changeValue(v);
    mSlider->setSliderPosition(value2);
}

void SpinSlider::setPixelPos(qreal min, qreal max, int val, int space, bool upsideDown)
{
    mSlider->setSliderPosition(QStyle::sliderValueFromPosition(static_cast<int>(min),
                                                               static_cast<int>(max), val, space, upsideDown));
}

void SpinSlider::setExponent(const qreal exp)
{
    mExp = exp;
}

bool SpinSlider::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::Wheel && obj == mSlider)
    {
        // Block wheel events.
        return true;
    }
    return false;
}
