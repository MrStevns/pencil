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
#ifndef INLINESLIDER_H
#define INLINESLIDER_H

#include <QWidget>
#include <QLabel>
#include <QPainterPath>

class LineEditWidget;

enum class SliderStartPosType {
    LEFT,
    MIDDLE
};

class InlineSlider : public QWidget
{

    Q_OBJECT
public:

    explicit InlineSlider(QWidget* parent, QString label, qreal min, qreal max, SliderStartPosType type);
    ~InlineSlider() override;

    void setRange(qreal min, qreal max) { mMin = min; mMax = max; }
    void setMin(qreal min) { mMin = min; }
    void setMax(qreal max) { mMax = max; }

    void setValue(qreal value);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    bool event(QEvent *event) override;

signals:
    void valueChanged(qreal value);

private:

    void onLineEditChanged();
    void onScreenChanged();
    void setupPixmap(const QSize& size);

    qreal valueFromMappedRange(qreal value, qreal min, qreal max, qreal oldMin, qreal oldMax) const;
    void setSliderPixelPos(qreal pos);
    void setSliderValueFromPos(int pos);

    void setCornerRadius(qreal percentage);

    void drawSlider();
    void drawLabels(QPainter& painter, const QRectF& borderRect, const QColor& textColor);
    void drawCaret(QPainter& painter, const QRectF& borderRect, const QColor& caretColor);

    void updateLineEditStylesheet();

    QRectF borderRect() const;

    QString mLabel;
    QPixmap mPixmap;

    qreal mMin = 0.0;
    qreal mMax = 0.0;

    int mBorderWidth = 1;

    qreal mCornerRadiusPercentage = 0.2;
    qreal mAbsoluteCornerRadiusX = 0;
    qreal mAbsoluteCornerRadiusY = 0;

    qreal mSliderValue = 0.0;
    qreal mSliderPos = 0.0;

    qreal mCaretWidth = 1.0;
    qreal mTextPadding = 6;

    SliderStartPosType mSliderOrigin = SliderStartPosType::MIDDLE;

    LineEditWidget* mValueLineEditWidget = nullptr;
};

#endif // INLINESLIDER_H
