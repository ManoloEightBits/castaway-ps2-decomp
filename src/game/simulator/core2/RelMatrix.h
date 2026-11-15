#ifndef GAME_SIMULATOR_CORE2_RELMATRIX_H
#define GAME_SIMULATOR_CORE2_RELMATRIX_H

#include "common/types.h"

// Wii: RelMatrix, the relationship interface RelMatrixImpl implements. Only the slots
// OwnableItem calls have names, from what it does with them; the others keep them in place.
class RelMatrix {
public:
    virtual void Slot0();
    virtual void Slot1();
    virtual void Slot2();
    virtual void Slot3();
    virtual void Slot4();
    virtual void Slot5();
    virtual void Slot6();
    virtual int GetRelationship(s16 id, int type); // slot 7: type 0 is ownership points
    virtual void SetRelationship(s16 id, int type, s16 value); // slot 8
    virtual void Slot9();
    virtual int Count();                 // slot 10
    virtual int GetIDByIndex(int index); // slot 11
};

#endif
