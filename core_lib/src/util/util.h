/*

Pencil - Traditional Animation Software
Copyright (C) 2005-2007 Patrick Corrieri & Pascal Naidon
Copyright (C) 2012-2018 Matthew Chiawen Chang

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

*/
#ifndef UTIL_H
#define UTIL_H

#include <cstddef>
#include <functional>
#include <QTransform>

#include <random>


QTransform RectMapTransform( QRectF source, QRectF target );

class ScopeGuard
{
public:
    explicit ScopeGuard(std::function< void() > onScopeExit) { m_onScopeExit = onScopeExit; }
    ~ScopeGuard() { m_onScopeExit(); }
private:
    std::function<void()> m_onScopeExit;
};

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)

#define OnScopeExit( callback ) ScopeGuard SCOPEGUARD_LINENAME( myScopeGuard, __LINE__ ) ( [&] { callback; } );


#define NULLReturnVoid( p ) if ( p == nullptr ) { return; }
#define NULLReturn( p, ret ) if ( p == nullptr ) { return ret; }
#define NULLReturnAssert( p ) if ( p == nullptr ) { Q_ASSERT(false); return; }


class SignalBlocker
{
public:
    explicit SignalBlocker(QObject* o);
    ~SignalBlocker();
private:
    QObject* mObject = nullptr;
    bool mBlocked = false;
};


class PerlinNoise
{

public:
    explicit PerlinNoise(std::uint32_t seed = std::default_random_engine::default_seed);

    void reseed(std::uint32_t seed);
    double noise(double x) const;
    double noise(double x, double y) const;
    double noise(double x, double y, double z) const;

private:
    std::int32_t p[512];

    static double Fade(double t) noexcept;
    static double Lerp(double t, double a, double b) noexcept;
    static double Grad(std::int32_t hash, double x, double y, double z) noexcept;

};

#endif // UTIL_H
