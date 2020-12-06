#ifndef MAPPINGCONFIGURATOR_H
#define MAPPINGCONFIGURATOR_H

#include <QWidget>

class MappingDistributionWidget;

class MappingConfiguratorWidget : public QWidget
{
  Q_OBJECT
public:

    MappingConfiguratorWidget(QWidget* parent = nullptr);
    MappingConfiguratorWidget(QString name, QString description, qreal min, qreal max, QVector<QPointF> points, int maxPoints, QWidget* parent = nullptr);
    ~MappingConfiguratorWidget();

signals:
    void mappingUpdated(QVector<QPointF> points);
    void updateInputsInMappingWidget(QVector<QPointF> points, qreal min, qreal max);
public slots:
    void updateMapping(QVector<QPointF> points);

private:
    void setupUI();
    void resetMapping();

    MappingDistributionWidget* mMappingDistributionWidget = nullptr;
    QString mDescription;
    QString mName;
};
#endif // MAPPINGCONFIGURATOR_H
