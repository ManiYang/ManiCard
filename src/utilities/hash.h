#ifndef HASH_H
#define HASH_H

#include <QtCore>

template <class T>
inline void hashCombine(uint &seed, const T &value)
{
    // Modified from Boost's hash_combine() implementation.

    uint hashOfValue = qHash(value);
    seed ^= hashOfValue + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

#endif // HASH_H
