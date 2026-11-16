#include "game/simulator/core/cXObject.h"

#include "common/EGlobal.h"
#include "game/GameData.h"
#include "game/simulator/core/Neighborhood.h"
#include "game/simulator/core/ObjectModule.h"
#include "game/simulator/core/FixedWorld.h"
#include "game/simulator/core/ObjSelector.h"
#include "game/simulator/objectdata/XObjLang.h"

inline int cXObject::GetTreeID(ObjEntryPoint entry)
{
    return mSelector->GetFnTable()->HasFunction(entry);
}

inline int cXObject::RunTreeInline(ObjEntryPoint entry, s16 a, s16 *param)
{
    int tree = GetTreeID(entry);
    if (tree != 0)
        return RunCheckTree(mSelector->mBehavior, a, tree, param);
    return 0;
}

// Runs the tree at an entry point to its end, with `a` as the stack object. 0 when the
// object has no tree there.
int cXObject::RunTree(ObjEntryPoint entry, s16 a, s16 *param)
{
    return RunTreeInline(entry, a, param);
}

void cXObject::PreSave()
{
}

void cXObject::TagGraphicsField()
{
    mGraphicsTagged = 1;
}

SpriteSlot *cXObject::GetSpriteSlot()
{
    return mSpriteSlots.mpBegin;
}

void cXObject::ForceLocation()
{
}

u32 cXObject::ReconType()
{
    return 0x584f424a;
}

int cXObject::HandleBreakpoint(StackElem *elem, BehaviorNode *node)
{
    return TreeSim::HandleBreakpoint(elem, node);
}

SpriteSlot *cXObject::GetSpriteSlot(u32 index)
{
    return &mSpriteSlots[index];
}

int cXObject::IsPartOfMe(cXObject *object)
{
    return object == this;
}

int cXObject::ContainsData(int index)
{
    return mVars.Contains(index);
}

// Variable 0x37 is the colour index; only its low byte counts.
u8 cXObject::GetColorIndex() const
{
    return mVars[0x37];
}

u8 cXObject::SetColorIndex(u8 index)
{
    u8 old = GetColorIndex();
    mVars.WriteVar(0x37, index);
    return old;
}

// A plain object covers one tile; multi-tile objects override this.
void cXObject::GetNumTiles(int &width, int &depth)
{
    width = 1;
    depth = 1;
}

// True once after TagGraphicsField, then clears the tag.
int cXObject::HaveGraphicsBeenTagged()
{
    if (mGraphicsTagged) {
        mGraphicsTagged = 0;
        return 1;
    }
    return 0;
}

// A tree error: flags the object and tells the module its Sim is in error. The code is not
// kept here; TreeSim::SetError has it.
void cXObject::Error(s16 code)
{
    mMiscFlags |= 0x100;
    mModule->SetSimFlag(mID, ObjectModule::kSimFlagError, true);
}

// Not built: the sprite-slot loop is five instructions whose closing branch GCC leaves in
// reorder mode, so the assembler's implicit nop is its sixth, and tools/asm_filter.py refuses a
// short loop it cannot see whole ("a short loop ends in a branch outside .set noreorder"). The
// game's loop (0x255C90) is exactly that: jal, delay slot, addiu, lw, bne, nop. Evidence for the
// filter learning that case; until then the logic is complete and unverified.
#ifdef NON_MATCHING
// The tree step, then the object's own tick: variable 0x19 counts down and every sprite slot
// ticks.
int cXObject::Simulate(int a)
{
    int result = TreeSim::Simulate(a);
    if (result) {
        if (mVars[0x19])
            mVars.WriteVar(0x19, mVars[0x19] - 1);
        for (SpriteSlot *slot = mSpriteSlots.begin(); slot != mSpriteSlots.end(); ++slot)
            slot->Tick();
    }
    return result;
}
#endif

// Runs a tree of another object's behaviour on this stack, with that object as the stack object.
int cXObject::GosubObjectTree(cXObject *object, s16 *params, s16 treeID, bool a)
{
    mModule->ClearIdleStatus(mID);
    if (Gosub(object->mSelector->mBehavior, params, treeID)) {
        GetCurElem()->mStackObject = object->mID;
        return 1;
    }
    return 0;
}

// Whether the object can be interacted with on its tile: never for definition word 0x45 == 2,
// always for a Sim, a single-tile object or a negative word 8, never for word 0x45 == 1, and
// otherwise when the object has a tree at entry point 8.
int cXObject::IsInteractionTile(bool inWorldOnly)
{
    if (inWorldOnly && !IsInWorld())
        return 0;
    s16 *def = mDef;
    if (def[0x45] == 2)
        return 0;
    // The game keeps this first `return 1` and the next two cases jump back to it, which only a
    // goto gives (see HANDOFF.md on cross-jumping).
    if (def[9] == 2) {
    returnOne:
        return 1;
    }
    if (def[0xa] == 0)
        goto returnOne;
    if (def[8] < 0)
        goto returnOne;
    // The result's 1 is set here: the game reuses it as the operand of the next compare.
    int result = 1;
    if (def[0x45] == 1)
        return 0;
    if ((s16)mSelector->GetFnTable()->HasFunction(8) == 0)
        result = 0;
    return result;
}

int cXObject::CanPlace(const FTilePt &tile, int a, cXObject *object, int b)
{
    return CanPlaceSingleTile(tile, a, object, b);
}

// Flags the transition in the EOR globals by type: 2 goes to the main menu, 3 clears
// neighbourhood variable 1 if it is 1, 5 carries the lot number.
void cXObject::TransitionToNewHouse(s16 lot, s16 type, s16 c)
{
    if (type == 2) {
        Globs::pEORGlobals->field_0x480 = 1;
        _gd.mHudTarget->GotoMainMenu();
    } else if (type == 3) {
        Globs::pEORGlobals->field_0x47c = 1;
        if (GetNeighborhood()->mVars[1] == 1)
            GetNeighborhood()->SetNeighborhoodVar(1, 0);
    } else {
        EGlobal *globals = Globs::pEORGlobals;
        globals->field_0x48c = 1;
        if (type == 5) {
            globals->mTransitionLot = lot;
            globals->field_0x47c = 1;
        } else {
            globals->mTransitionLot = 1;
            globals->field_0x484 = 1;
        }
    }
}

// The user may delete an object when the globals say anything goes, or when it can be picked
// up, its variable 0x2b has bit 3 set, and none of its slots holds another object.
int cXObject::UserCanDelete()
{
    if (Globs::pEORGlobals->field_0x414)
        return 1;
    // The game calls this one directly, not through the vtable.
    if (!cXObject::UserCanPickup())
        return 0;
    if (!(mVars[0x2b] & 8))
        return 0;
    for (int i = CountObjectSlots() - 1; i >= 0; i--) {
        if (GetContainedObject(i))
            return 0;
    }
    return 1;
}

// A single-tile object runs its user-pickup tree, then the tree of every object in its slots;
// a multi-tile one leaves that to its parts. Then it is picked up.
void cXObject::UserPickup(bool a)
{
    if (!IsMultiTile()) {
        RunTree(kObjEntryUserPickup, 0, 0);
        int count = CountObjectSlots();
        for (int i = 0; i < count; i++) {
            if (GetContainedObject(i))
                GetContainedObject(i)->RunTree(kObjEntryUserPickup, 0, 0);
        }
    }
    Pickup();
}

// Placed, then the user-placement trees run the same way UserPickup runs the pickup ones.
void cXObject::UserPlace(const FTilePt &tile, int a, cXObject *object, int b)
{
    Place(tile, a, object, b);
    if (!IsMultiTile()) {
        RunTree(kObjEntryUserPlacement, 0, 0);
        int count = CountObjectSlots();
        for (int i = 0; i < count; i++) {
            if (GetContainedObject(i))
                GetContainedObject(i)->RunTree(kObjEntryUserPlacement, 0, 0);
        }
    }
}

// After a load: the level and location are set again, a selector that needs a reset (or the
// placeholder selector) loses its people and is cleaned up and re-initialised, and outside
// author mode the load tree runs with the transition type as its parameter.
void cXObject::PostLoad(int a, Globs::TransitionType type)
{
    if (mLevel == 0)
        SetLevel(1);
    SetLocation(GetLocationRef(), mLevel);
    s16 params[4];
    params[0] = type;
    params[1] = 0;
    params[2] = 0;
    params[3] = 0;
    int needsReset = mSelector->GetNeedsReset();
    ObjSelector *placeholder = GetObjectFolder()->GetPlaceholder();
    // Both builds compute this compare as a value before testing needsReset.
    int isPlaceholder = mSelector == placeholder;
    if (needsReset || isPlaceholder) {
        mModule->CleanupPeople(this);
        Cleanup(0);
        if (needsReset)
            RunTree(kObjEntryInit, 0, 0);
    }
    if (!_gd.IsAuthorMode())
        RunTree(kObjEntryLoad, 0, params);
    UpdateSimFlags();
}

// Weather damage: each damage type whose armour is under 255 times the scale adds its amount
// less the armour to the total, which never goes below zero. A positive total runs the
// apply-damage tree with it.
void cXObject::ApplyDamage(const Weather::DamageEvent &event, s16 scale)
{
    if ((s16)GetTreeID(kObjEntryApplyDamage) == 0)
        return;
    Weather::Armor armor;
    CalcArmorAgainstDamage(event, armor);
    s16 params[8];
    memset(params, 0, sizeof(params));
    for (int i = 0; i < Weather::DAMAGE_TYPE_COUNT; i++) {
        if (armor.values[i] < scale * 255) {
            int damage = params[0] + event.amounts[i] - armor.values[i];
            if (damage < 0)
                damage = 0;
            params[0] = damage;
        }
    }
    if (params[0] > 0)
        RunTreeInline(kObjEntryApplyDamage, 0, params);
}

bool cXObject::IsPerson() const
{
    return IsPersonInline();
}

cXObject *cXObject::HierGetParent()
{
    return GetObjectFromID(mVars[0x1a]);
}

cXObject *cXObject::GetContainer()
{
    return GetContainerInline();
}

// Whether the user may pick the object up, with the reason in gPlacementError when not: 0x25
// its variable 0x2b says it can't be, 0x26 it is in use (variable 0x3e, the occupied bit, or
// a Sim holds it), 0x27 its tile may not be edited. Two objects by GUID skip the check of
// what they contain; anything else fails when something in its slots can't be picked up.
int cXObject::UserCanPickup()
{
    gPlacementError = 0;
    gPlacementConflict = 0;
    if (Globs::pEORGlobals->field_0x414)
        return 1;
    if (!(mVars[0x2b] & 2)) {
        gPlacementError = 0x25;
        return 0;
    }
    if (mVars[0x3e] > 0) {
        gPlacementError = 0x26;
        return 0;
    }
    if (IsOccupied()) {
        gPlacementError = 0x26;
        return 0;
    }
    cXObject *container = GetContainerInline();
    if (container != 0 && container->IsPerson()) {
        gPlacementError = 0x26;
        return 0;
    }
    int guid = mSelector != 0 ? mSelector->GetGUID() : 0;
    switch (guid) {
    case 0x0D4BA4C8:
    case 0x152C7A14:
        break;
    default:
        for (int i = CountObjectSlots() - 1; i >= 0; i--) {
            cXObject *object = GetContainedObject(i);
            if (object != 0 && !object->UserCanPickup())
                return 0;
        }
        break;
    }
    CTilePt tile = GetCTilePt();
    if (!gFixedWorld->OutOfGrid(tile) && !gFixedWorld->MayEditTile(tile)) {
        gPlacementError = 0x27;
        return 0;
    }
    return 1;
}
