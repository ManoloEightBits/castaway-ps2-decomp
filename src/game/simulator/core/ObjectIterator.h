#ifndef GAME_SIMULATOR_CORE_OBJECTITERATOR_H
#define GAME_SIMULATOR_CORE_OBJECTITERATOR_H

#include "common/types.h"

class CTilePt;
class cXObject;

// Wii: ObjectIterator. Walks the objects on a tile (SetToNext) or the parts of an object
// (UpdateChairFacing). 12 bytes, from the two constructors.
class ObjectIterator {
public:
    enum IterateType { kAll };

    cXObject *mStart;  // 0x00  the object given; what the tile constructor stores is unread
    cXObject *mObject; // 0x04  the current object, 0 at the end
    int mType;         // 0x08

    ObjectIterator(const CTilePt &tile, IterateType type); // 0x254C50
    ObjectIterator(cXObject *object, IterateType type);    // 0x254CA8
    ObjectIterator &operator++();                          // 0x220208
    cXObject *operator*() const { return mObject; }
    bool finished() const { return mObject == 0 ? true : false; }
};

#endif
