#ifndef GAME_SIMULATOR_CORE2_SLOTLOADER_H
#define GAME_SIMULATOR_CORE2_SLOTLOADER_H

#include "common/types.h"
#include "common/eastl.h"

class ObjectSlot;
class RoutingSlot;
class SpriteSlot;
class cXObject;

// Wii: SlotLoader. Built on the stack by cXObject::Initialize to fill an object's three slot
// vectors from its definition. Eight bytes: the Wii frame holds it in 8, and the PS2
// constructor writes a word and a short.
class SlotLoader {
public:
    u32 mNamespaceID; // 0x00  the object's namespace, or its semi-namespace for 1000 slots or more
    s16 field_0x4;    // 0x04  0 from the constructor

    SlotLoader(u32 namespaceID); // 0x1E1C38
    ~SlotLoader();               // 0x1E1C48
    void Load(s16 count, eastl::vector<ObjectSlot, eastl::allocator> *objectSlots,
              eastl::vector<RoutingSlot, eastl::allocator> *routingSlots,
              eastl::vector<SpriteSlot, eastl::allocator> *spriteSlots, cXObject *object); // 0x1C1148
};

#endif
