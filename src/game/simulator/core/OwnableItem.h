#ifndef GAME_SIMULATOR_CORE_OWNABLEITEM_H
#define GAME_SIMULATOR_CORE_OWNABLEITEM_H

#include "common/types.h"
#include "game/simulator/core/ObjectItem.h"

class cXObject;

// Generated from the Wii symbol names and the PS2 code of each function. Only the functions
// decompiled so far are declared, as plain member functions: the base class, the vtable order
// and the full layout are not known yet. Return types are guesses, and the fields are only
// the ones these functions touch. The names come from pkg/castaway_ps2_names.csv, mostly its
// vtable pairing tiers; a one-instruction body cannot confirm that a name is right.
//
// The base is ObjectItem: OwnableItem's constructor (0x1FBB68) stores ObjectItem's vtable
// before its own (0x4BB350), and that vtable keeps ObjectItem's functions in most slots.
class OwnableItem : public ObjectItem {
public:
    // Wii: OwnableItem::OwnerInfo, a Sim and its ownership points for this object.
    struct OwnerInfo {
        s16 mOwner;  // 0x00
        s16 mPoints; // 0x02
    };
    eastl::vector<OwnerInfo, eastl::allocator> mOwners; // 0x24  0x34 bytes in all: AddOwnableItem allocates that

    OwnableItem(cXObject *object);
    OwnableItem(u32 guid); // 0x24F428: ObjectItem(guid), then its own vtable and zeroed fields
    s8 GetCategory() const;
    // Its own in slots 4, 5 and 31 of its vtable, so these are the names.
    s16 GetOwnerID() const;
    int GetOwnershipPoints(s16 a) const;
    void SetupOwnershipInfo(cXObject *object);
    void AddOwner(OwnerInfo &info); // 0x24F470
    cXObject *MakeObject(Family *family, s16 a);
    void LoadItemInfo(cXObject *object, Family *family, s16 a);
    void DoStream(ReconBuffer *buffer, int version);
};

#endif
