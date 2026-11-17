#include "game/simulator/core/ObjectIterator.h"

// 0x254CA8
ObjectIterator::ObjectIterator(cXObject *object, IterateType type)
{
    mStart = object;
    mObject = object;
    mType = type;
}
