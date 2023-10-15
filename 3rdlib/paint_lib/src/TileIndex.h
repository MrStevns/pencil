#ifndef TILEINDEX_H
#define TILEINDEX_H

struct TileIndex {
    int x;
    int y;
};

inline uint qHash(const TileIndex &key, uint seed)
{
    return qHash(key.x, seed) ^ key.y;
}

inline bool operator==(const TileIndex &e1, const TileIndex &e2)
{
    return e1.x == e2.x
           && e1.y == e2.y;
}

#endif // TILEINDEX_H
