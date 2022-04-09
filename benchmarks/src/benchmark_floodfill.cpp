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
#include <hayai.hpp>

#include "object.h"
#include "editor.h"
#include "bitmapbucket.h"
#include "bitmapimage.h"
#include "scribblearea.h"
#include "layermanager.h"
#include "filemanager.h"
#include "layerbitmap.h"

#include <QTemporaryDir>

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
    FileManager* fileManager;
    BitmapImage* img;
};

class BitmapImageContourFixture : public hayai::Fixture {
public:
    virtual void SetUp()
    {
        fileManager = new FileManager();
        QTemporaryDir tempDir;

        const QString path = tempDir.path() + "/contour-fill.pclx";
        QFile::copy(":/contour-fill.pclx", path);
        Object* loadedObject = fileManager->load(path);

        editor = new Editor;
        ScribbleArea* scribbleArea = new ScribbleArea(nullptr);
        editor->setScribbleArea(scribbleArea);
        editor->setObject(loadedObject);
        editor->init();

        img = static_cast<BitmapImage*>(editor->layers()->currentLayer()->getKeyFrameAt(1));
    }

    virtual void TearDown()
    {
        delete editor->getScribbleArea();
        delete editor;
        delete fileManager;
    }

    FileManager* fileManager;
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

BENCHMARK_F(BitmapImage1080pFixture, ExpandFill, 10, 5)
{
    Properties properties;
    properties.bucketFillReferenceMode = 0;
    properties.bucketFillToLayerMode = 0;
    properties.bucketFillExpandEnabled = true;
    properties.fillMode = 0;

    QPoint fillPoint = img->bounds().center();

    BitmapBucket bucket = BitmapBucket(editor, Qt::green, img->bounds(), fillPoint, properties);

    bucket.paint(fillPoint, [] (BucketState, int, int ) {});
}

BENCHMARK_F(BitmapImageEmptyFixture, ExpandFillTo1080p, 10, 5)
{
    Properties properties;
    properties.bucketFillReferenceMode = 0;
    properties.bucketFillToLayerMode = 0;
    properties.bucketFillExpandEnabled = true;
    properties.fillMode = 0;

    QRect cameraRect(0, 0, 1920, 1080);
    QPoint fillPoint = cameraRect.center();

    BitmapBucket bucket = BitmapBucket(editor, Qt::green, cameraRect, fillPoint, properties);

    bucket.paint(fillPoint, [] (BucketState, int, int ) {});
}

BENCHMARK_F(BitmapImageContourFixture, ExpandFillTo1080p, 10, 5)
{
    Properties properties;
    properties.bucketFillReferenceMode = 0;
    properties.bucketFillToLayerMode = 0;
    properties.bucketFillExpandEnabled = true;
    properties.bucketFillExpand = 5;
    properties.fillMode = 0;

    QRect cameraRect(0, 0, 1920, 1080);
    QPoint fillPoint1 = QPoint(84,365);
    QPoint fillPoint2 = QPoint(91,89);
    QPoint fillPoint3 = QPoint(590,98);
    QPoint fillPoint4 = QPoint(595,378);

    BitmapBucket bucket = BitmapBucket(editor, Qt::green, cameraRect, fillPoint1, properties);
    bucket.paint(fillPoint1, [] (BucketState, int, int ) {});


    bucket.paint(fillPoint2, [] (BucketState, int, int ) {});
    bucket.paint(fillPoint3, [] (BucketState, int, int ) {});
    bucket.paint(fillPoint4, [] (BucketState, int, int ) {});
}
