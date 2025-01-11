#include "selectionmanager.h"

#include "object.h"
#include "selectionbitmapeditor.h"
#include "selectionvectoreditor.h"
#include "bitmapimage.h"

#include "layerbitmap.h"
#include "editor.h"

SelectionManager::SelectionManager(Editor* editor) : BaseManager(editor, __FUNCTION__)
{
}

SelectionManager::~SelectionManager()
{
}

bool SelectionManager::init()
{
    return true;
}

Status SelectionManager::load(Object* obj)
{
    int count = obj->getLayerCount();
    for (int i = 0; i < count; ++i)
    {
        Layer* layer = obj->getLayer(i);
        if (!layer->isPaintable())
        {
            continue;
        }

        if (layer->type() == Layer::BITMAP) {
            layer->foreachKeyFrame([this](KeyFrame* key)
            {
                createEditor(static_cast<BitmapImage*>(key));
            });
        } else {
            layer->foreachKeyFrame([this](KeyFrame* key)
            {
                // TODO: implement for vector
            });
        }
    }
    return Status::OK;
}

Status SelectionManager::save(Object*)
{
    return Status::OK;
}

void SelectionManager::workingLayerChanged(Layer* layer)
{
    mWorkingLayer = layer;
}

void SelectionManager::createEditor(BitmapImage* bitmapImage)
{
    SelectionBitmapEditor* selectionEditor = new SelectionBitmapEditor();
    bitmapImage->attachSelectionEditor(selectionEditor);

    selectionEditor->selectionReset = [this] (void) {
        emit selectionReset();
    };

    selectionEditor->selectionChanged = [this] (void) {
        emit selectionChanged();
    };
}

KeyFrame* SelectionManager::getCurrentKeyFrame()
{
    if (!editor()) { return nullptr; }

    if (mWorkingLayer->type() == Layer::BITMAP) {
        return static_cast<BitmapImage*>(mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame()));
    } else if (mWorkingLayer->type() == Layer::VECTOR) {
        // return static_cast<VectorImage*>(mWorkingLayer->getLastKeyFrameAtPosition(keyPos))->selectionEditor();
        // TODO:
        return nullptr;
    }

    return nullptr;
}

SelectionEditor* SelectionManager::getSelectionEditor(KeyFrame* keyframe)
{
    if (!editor()) { return nullptr; }

    if (mWorkingLayer->type() == Layer::BITMAP) {
        BitmapImage* bitmapImage = static_cast<BitmapImage*>(keyframe);
        if (keyframe == nullptr) {
            bitmapImage = static_cast<BitmapImage*>(mWorkingLayer->getLastKeyFrameAtPosition(editor()->currentFrame()));
        }

        if (bitmapImage->temporaryImage()) {
            return bitmapImage->temporaryImage()->selectionEditor();
        } else {
            return bitmapImage->selectionEditor();
        }
    } else if (mWorkingLayer->type() == Layer::VECTOR) {
        // return static_cast<VectorImage*>(mWorkingLayer->getLastKeyFrameAtPosition(keyPos))->selectionEditor();
        // TODO:
        return nullptr;
    }
    return nullptr;
}

void SelectionManager::setSelection(const QRectF& rect)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->setSelection(rect);
}

void SelectionManager::deselect()
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->deselect();
}

QRectF SelectionManager::mySelectionRect()
{
    if (!getSelectionEditor()) { return QRectF(); }

    return getSelectionEditor()->mySelectionRect();
}

QTransform SelectionManager::selectionTransform()
{
    if (!getSelectionEditor()) { return QTransform(); }

    return getSelectionEditor()->selectionTransform();
}

bool SelectionManager::somethingSelected()
{
    if (!getSelectionEditor()) { return false; }

    return getSelectionEditor()->somethingSelected();
}

bool SelectionManager::isOutsideSelectionArea(const QPointF& point)
{
    if (!getSelectionEditor()) { return false; }

    return getSelectionEditor()->isOutsideSelectionArea(point);
}

void SelectionManager::commitChanges()
{
    if (!getSelectionEditor()) { return; }
    // We should probably run through all keyframes here, so we can commit all selection changes.
    // Technically there's currently only going to be one keyframe being selected
    // when commiting
    KeyFrame* keyframe = getCurrentKeyFrame();
    if (keyframe) {
        getSelectionEditor()->commitChanges(keyframe);
        editor()->setModified(editor()->currentLayerIndex(), editor()->currentFrame());
    }
}

void SelectionManager::discardChanges()
{
    if (!getSelectionEditor()) { return; }

    KeyFrame* keyframe = getCurrentKeyFrame();
    if (keyframe) {
        getSelectionEditor()->discardChanges(keyframe);
        editor()->setModified(editor()->currentLayerIndex(), editor()->currentFrame());
    }
}

void SelectionManager::deleteSelection()
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->deleteSelection();
    editor()->setModified(editor()->currentLayerIndex(), editor()->currentFrame());
}

void SelectionManager::translate(const QPointF& point)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->translate(point);
    getSelectionEditor()->calculateSelectionTransformation();

    emit editor()->frameModified(editor()->currentFrame());
}

void SelectionManager::maintainAspectRatio(bool maintainAspectRatio)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->maintainAspectRatio(maintainAspectRatio);
}

void SelectionManager::alignToAxis(bool alignToAxis)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->alignPositionToAxis(alignToAxis);
}

QPointF SelectionManager::currentTransformAnchorPoint()
{
    if (!getSelectionEditor()) { return QPointF(); }

    return getSelectionEditor()->currentTransformAnchor();
}

qreal SelectionManager::angleFromPoint(const QPointF& pos, const QPointF& anchorPoint)
{
    if (!getSelectionEditor()) { return 0; }

    return getSelectionEditor()->angleFromPoint(pos, anchorPoint);
}

void SelectionManager::adjustCurrentSelection(const QPointF& pos, const QPointF& offsetPoint, qreal angle, qreal rotationIncrement)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->adjustCurrentSelection(pos, offsetPoint, angle, rotationIncrement);
}

QRectF SelectionManager::mappedSelectionRect()
{
    SelectionEditor* selectionEditor = getSelectionEditor();
    if (!selectionEditor) { return QRectF(); }

    return selectionEditor->mapToSelection(selectionEditor->mySelectionRect()).boundingRect();
}

void SelectionManager::setMoveModeForAnchorInRange(const QPointF& point)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->setMoveModeForAnchorInRange(point);
}

MoveMode SelectionManager::getMoveMode()
{
    if (!getSelectionEditor()) { return MoveMode::NONE; }

    return getSelectionEditor()->getMoveMode();
}

void SelectionManager::setMoveMode(MoveMode mode)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->setMoveMode(mode);
}

void SelectionManager::setTransformAnchor(const QPointF& point)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->setTransformAnchor(point);
}

void SelectionManager::setDragOrigin(const QPointF& point)
{
    if (!getSelectionEditor()) { return; }

    getSelectionEditor()->setDragOrigin(point);
}

QPointF SelectionManager::getSelectionAnchorPoint()
{
    if (!getSelectionEditor()) { return QPointF(); }

    return getSelectionEditor()->getSelectionAnchorPoint();
}

QPointF SelectionManager::myTranslation()
{
    if (!getSelectionEditor()) { return QPointF(); }

    return getSelectionEditor()->myTranslation();
}

qreal SelectionManager::myRotation()
{
    if (!getSelectionEditor()) { return -1; }

    return getSelectionEditor()->myRotation();
}
