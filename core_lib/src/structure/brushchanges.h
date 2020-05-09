#ifndef BRUSHCHANGES_H
#define BRUSHCHANGES_H

#include <QHash>
#include <QJsonObject>
#include <QJsonArray>

#include "brushsetting.h"

struct InputChanges {
    QVector<QPointF> mappedPoints;
    BrushInputType inputType;
    bool enabled;

    InputChanges(QVector<QPointF> newMappedPoints, BrushInputType newInputType, bool newEnabled)
    {
        mappedPoints = newMappedPoints;
        inputType = newInputType;
        enabled = newEnabled;
    }

    InputChanges(QVector<QPointF> newMappedPoints, BrushInputType newInputType)
    {
        mappedPoints = newMappedPoints;
        inputType = newInputType;
        enabled = true;
    }

    void write(QJsonArray& object) const
    {
        for (int i = 0; i < mappedPoints.count(); i++) {
            const auto mappedPoint = mappedPoints[i];
            QJsonArray pointArray = { mappedPoint.x(), mappedPoint.y() };
            object.removeAt(i);
            object.insert(i, pointArray);
        }
    }
};

struct BrushChanges {
    QHash<int, InputChanges> listOfinputChanges;
    qreal baseValue;
    BrushSettingType settingsType;

    void write(QJsonObject& object) const
    {
        QJsonObject::iterator baseValueObjIt = object.find("base_value");

        if (baseValueObjIt->isUndefined()) {
            object.insert("base_value", baseValue);
        } else {
            object.remove("base_value");
            object.insert("base_value", baseValue);
        }

        QJsonObject::iterator inputsObjIt = object.find("inputs");
        if (inputsObjIt->isUndefined()) {
            object.insert("inputs", QJsonObject());
        }

        QJsonValueRef inputsContainerRef = object.find("inputs").value();
        QJsonObject inputsContainerObj = inputsContainerRef.toObject();
        QHashIterator<int, InputChanges> inputIt(listOfinputChanges);
        while (inputIt.hasNext()) {
            inputIt.next();

            InputChanges inputChanges = inputIt.value();

            QString inputId = getBrushInputIdentifier(inputChanges.inputType);
            QJsonObject::iterator inputContainerObjIt = inputsContainerObj.find(inputId);

            if (inputContainerObjIt->isUndefined()) {
                if (inputChanges.enabled) {
                    QJsonArray inputArray;
                    inputChanges.write(inputArray);
                    inputsContainerObj.insert(inputId, inputArray);
                }
            } else {

                if (inputChanges.enabled) {
                    QJsonValueRef inputArrRef = inputContainerObjIt.value();
                    QJsonArray inputArray = inputArrRef.toArray();
                    inputChanges.write(inputArray);

                    inputsContainerObj.remove(inputId);
                    inputsContainerObj.insert(inputId, inputArray);
                } else {
                    QString inputKey = inputContainerObjIt.key();
                    inputsContainerObj.remove(inputKey);
                }
            }
            inputsContainerRef = inputsContainerObj;
        }
    }
};

#endif // BRUSHCHANGES_H
