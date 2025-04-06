#ifndef MPBRUSHPREVIEW_H
#define MPBRUSHPREVIEW_H

#include <QWidget>
#include <QHash>

#include "tileindex.h"

class MPHandler;
class MPTile;
class MPSurface;
class QElapsedTimer;

class MPBrushPreview : public QWidget
{
public:
    MPBrushPreview(QWidget* parent = nullptr);
    ~MPBrushPreview() override;

    void updatePreview(const QByteArray& content, const QColor& brushColor);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void drawStroke() const;
    void paintTestBackground();
    void updatePaintSurface(QSize size);

    void updateTile(MPSurface *surface, MPTile* tile);
    void loadTile(MPSurface* surface, MPTile* tile);
    void onTileCleared(MPSurface* surface, MPTile* tile);

    MPHandler* mMypaintHandler = nullptr;

    MPSurface* mSurface = nullptr;
    QElapsedTimer* perfTimer = nullptr;
    QElapsedTimer* updateTimer = nullptr;

    QImage mSurfaceBackground;
};

#endif // MPBRUSHPREVIEW_H
