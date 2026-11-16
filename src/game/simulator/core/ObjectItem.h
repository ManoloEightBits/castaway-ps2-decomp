#ifndef GAME_SIMULATOR_CORE_OBJECTITEM_H
#define GAME_SIMULATOR_CORE_OBJECTITEM_H

#include "common/types.h"
#include "game/simulator/core/InventoryItems.h"

class cXObject;
class Family;
class ReconBuffer;

// Generated from the Wii symbol names and the PS2 code of each function. Only the functions
// decompiled so far are declared, as plain member functions: the base class, the vtable order
// and the full layout are not known yet. Return types are guesses, and the fields are only
// the ones these functions touch. The names come from pkg/castaway_ps2_names.csv, mostly its
// vtable pairing tiers; a one-instruction body cannot confirm that a name is right.
class ObjectItem : public InventoryItem {
public:
    u32 field_0x8; // 0x08  the GUID
    s8 field_0xc; // 0x0c  the status text
    u8 pad_0xd;
    s16 field_0xe; // 0x0e  the constructor zeroes these three; MakeObject copies them into
    s16 field_0x10; // 0x10  the object's variables 0x29, 0xF and 0x27
    s16 field_0x12; // 0x12
    eastl::vector<s16, eastl::allocator> mAttributes; // 0x14  0x24 bytes in all: InventoryItems::AddObjectItem allocates that

    ObjectItem(int guid);
    ObjectItem(cXObject *object);

    s16 GetID() const;
    u32 GetGuid() const;
    s8 GetCategory() const;
    s16 GetCount() const;
    s16 GetOwnerID() const;
    int GetOwnershipPoints(s16 a) const;
    s8 GetStatusText();
    void SetStatusText(s8 value);
    void IncrementCount(s16 a);
    void DecrementCount(s16 a);
    const InventoryTokens *GetToken();
    const IngredientToken *GetIngredientToken();
    bool IsFood();
    bool IsFastFood();
    bool IsTool();
    bool IsFastTool();
    bool IsResource();
    bool CanBePlanted();
    bool IsHandObject();
    bool IsClothing();
    bool IsDye();
    bool IsDyable();
    void SetupOwnershipInfo(cXObject *a);

    // The attribute list, written from the PS2 code.
    int GetNumAttributes() const;
    s16 GetAttribute(int index);
    void SetAttribute(int index, s16 value);
    void AddAtribute(s16 value);
    void ClearAttributes();
    void DoStream(ReconBuffer *buffer, int version);
    // MakeObject and LoadItemInfo are inline in the original: OwnableItem's versions contain
    // their bodies, and the vtable's copies are these. Inventory.h has the inline bodies.
    inline cXObject *MakeObjectInline(Family *family, s16 a);
    inline void LoadItemInfoInline(cXObject *object, Family *family, s16 a);
    cXObject *MakeObject(Family *family, s16 a);
    void LoadItemInfo(cXObject *object, Family *family, s16 a);
};

#endif
