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
#ifndef LAYERVECTOR_H
#define LAYERVECTOR_H

#include <QImage>
#include "layer.h"

class VectorImage;

class LayerVector : public Layer
{
    Q_DECLARE_TR_FUNCTIONS(LayerVector)

public:
    explicit LayerVector(int id);
    ~LayerVector();

    // method from layerImage
    void loadImageAtFrame(QString strFileName, int);

    QDomElement createDomElement(QDomDocument& doc) const override;
    void loadDomElement(const QDomElement& element, QString dataDirPath, ProgressCallback progressStep) override;

    VectorImage* getVectorImageAtFrame(int frameNumber) const;
    VectorImage* getLastVectorImageAtFrame(int frameNumber, int increment) const;
    void replaceKeyFrame(const KeyFrame* vectorImage) override;

    bool usesColor(int index);
    void removeColor(int index);
    void moveColor(int start, int end);

protected:
    Status saveKeyFrameFile(KeyFrame*, QString path) override;
    KeyFrame* createKeyFrame(int position) override;

private:
    QString fileName(KeyFrame* key) const;
    bool needSaveFrame(KeyFrame* key, const QString& strSavePath);
};

#endif
