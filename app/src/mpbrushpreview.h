#ifndef MPBRUSHPREVIEW_H
#define MPBRUSHPREVIEW_H

#include <QWidget>

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
    void applyBackground();

    MPHandler* mMypaintHandler = nullptr;

    void updateTile(MPSurface *surface, MPTile *);

    MPSurface* mMypaintSurface = nullptr;
    QElapsedTimer* perfTimer = nullptr;

    QImage* mSurfaceBackground = nullptr;
};

#endif // MPBRUSHPREVIEW_H
