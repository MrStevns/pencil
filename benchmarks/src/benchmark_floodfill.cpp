/*

Pencil2D - Traditional Animation Software
Copyright (C) 2012-2020 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#include <hayai/hayai.hpp>

#include "object.h"
#include "editor.h"
#include "bitmapbucket.h"
#include "bitmapimage.h"
#include "scribblearea.h"
#include "layermanager.h"
#include "layerbitmap.h"

class BitmapImage1080pFixture : public hayai::Fixture {
public:
    virtual void SetUp()
    {
        Object* object = new Object();
        object->init();
        object->addNewBitmapLayer();
        object->data()->setCurrentLayer(0);

        editor = new Editor;
        ScribbleArea* scribbleArea = new ScribbleArea(nullptr);
        editor->setScribbleArea(scribbleArea);
        editor->setObject(object);
        editor->init();

        img = new BitmapImage(QRect(0, 0, 1920, 1080), Qt::white);
        editor->layers()->currentLayer()->removeKeyFrame(1);
        editor->layers()->currentLayer()->addKeyFrame(1, img);
    }

    virtual void TearDown()
    {
        delete editor->getScribbleArea();
        delete editor;
    }

    Editor* editor;
    BitmapImage* img;
};

class BitmapImageEmptyFixture : public hayai::Fixture {
public:
    virtual void SetUp()
    {
        Object* object = new Object();
        object->init();
        object->addNewBitmapLayer();
        object->data()->setCurrentLayer(0);

        editor = new Editor;
        ScribbleArea* scribbleArea = new ScribbleArea(nullptr);
        editor->setScribbleArea(scribbleArea);
        editor->setObject(object);
        editor->init();

        img = static_cast<BitmapImage*>(editor->layers()->currentLayer()->getKeyFrameAt(1));
    }

    virtual void TearDown()
    {
        delete editor->getScribbleArea();
        delete editor;
    }

    Editor* editor;
    BitmapImage* img;
};

BENCHMARK_F(BitmapImage1080pFixture, FloodFill, 10, 5)
{
    Properties properties;
    properties.bucketFillReferenceMode = 0;
    properties.bucketFillToLayerMode = 0;
    properties.bucketFillExpandEnabled = false;
    properties.fillMode = 0;

    QPoint fillPoint = img->bounds().center();

    BitmapBucket bucket = BitmapBucket(editor, Qt::green, img->bounds(), fillPoint, properties);

    bucket.paint(fillPoint, [] (BucketState, int, int ) {});
}

BENCHMARK_F(BitmapImageEmptyFixture, FloodFillTo1080p, 10, 5)
{
    Properties properties;
    properties.bucketFillReferenceMode = 0;
    properties.bucketFillToLayerMode = 0;
    properties.bucketFillExpandEnabled = false;
    properties.fillMode = 0;

    QRect cameraRect(0, 0, 1920, 1080);
    QPoint fillPoint = cameraRect.center();

    BitmapBucket bucket = BitmapBucket(editor, Qt::green, cameraRect, fillPoint, properties);

    bucket.paint(fillPoint, [] (BucketState, int, int ) {});
}
