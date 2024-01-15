#ifndef TILEINDEX_H
#define TILEINDEX_H

struct TileIndex {
    int x;
    int y;
};

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
inline uint qHash(const TileIndex &key, uint seed)
#else
inline size_t qHash(const TileIndex &key, size_t seed)
#endif
{
    return qHash(key.x, seed) ^ key.y;
}

inline bool operator==(const TileIndex &e1, const TileIndex &e2)
{
    return e1.x == e2.x
           && e1.y == e2.y;
}

#endif // TILEINDEX_H
