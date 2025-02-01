#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <QObject>

#include "basemanager.h"
#include "pencildef.h"

#include "layer.h"
#include "movemode.h"

class Object;
class BitmapImage;
class SelectionEditor;

class SelectionManager : public BaseManager
{
    Q_OBJECT
public:
    explicit SelectionManager(Editor* parent = nullptr);
    ~SelectionManager() override;

    bool init() override;
    Status load(Object* obj) override;
    Status save(Object* obj) override;

    SelectionEditor* getSelectionEditor(KeyFrame* keyframe = nullptr);

    void workingLayerChanged(Layer*) override;

    void createEditor(BitmapImage* bitmapImage);

    void setSelection(const QRectF& rect);
    void deselect();

    void deleteSelection();
    void commitChanges();
    void discardChanges();

    void setMoveModeForAnchorInRange(const QPointF& point);

    void translate(const QPointF& point);

    void alignToAxis(bool alignToAxis);
    void maintainAspectRatio(bool maintainAspectRatio);

    void adjustCurrentSelection(const QPointF& pos, const QPointF& offsetPoint, qreal angle, qreal rotationIncrement);

    void setTransformAnchor(const QPointF& point);
    void setDragOrigin(const QPointF& point);

    KeyFrame* getCurrentKeyFrame();

    qreal angleFromPoint(const QPointF& pos, const QPointF& anchorPoint);
    qreal myRotation();

    QPointF getSelectionAnchorPoint();
    QPointF currentTransformAnchorPoint();
    QPointF myTranslation();

    QPolygonF mySelectionPolygon();

    QTransform selectionTransform();

    void setMoveMode(MoveMode mode);
    MoveMode getMoveMode();

    QPolygonF mappedSelectionPolygon();

    bool isOutsideSelectionArea(const QPointF& point);
    bool somethingSelected();

    void setEditorCallbacks(SelectionEditor* selectionEditor);

signals:
    void selectionChanged();
    void selectionReset();

private:
    Layer* mWorkingLayer = nullptr;

};

#endif // SELECTIONMANAGER_H
