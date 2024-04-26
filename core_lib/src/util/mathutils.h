#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <QtMath>
#include <QPoint>

namespace MathUtils
{
    /** Get the angle from the difference vector a->b to the x-axis.
     *
     * \param a Start point of vector
     * \param b End point of vector
     * \return Angle in radians from [-pi,+pi]
     */
    inline qreal getDifferenceAngle(const QPointF a, const QPointF b)
    {
        return qAtan2(b.y() - a.y(), b.x() - a.x());
    }

    /**
     * @brief linearMap
     * affine transformation
     * formula: y = (x-a)*(d-c)/(b-a)+c
     * \param value The value that needs to be mapped
     * \param fromA The old min value
     * \param fromB The old max value
     * \param toC The new min value
     * \param toD The new max value
     * \return The value in the new range
     */
    inline qreal linearMap(qreal value, qreal fromA, qreal fromB, qreal toC, qreal toD)
    {
        return (value-fromA)*((toD-toC)/(fromB-fromA))+toC;
    }

    /** Normalize x to a value between 0 and 1;
    *  \param x The input value
    *  \param min The input min value
    *  \param max The input max value
    *  \return The value of x normalized to a range between 0 and 1
    */
    inline qreal normalize(qreal x, qreal min, qreal max)
    {
        return qAbs((x - max) / (min - max));
    }
}

#endif // MATHUTILS_H
