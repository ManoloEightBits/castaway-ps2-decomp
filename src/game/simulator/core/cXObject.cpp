#include "game/simulator/core/cXObject.h"

#include "common/EGlobal.h"
#include "game/GameData.h"
#include "game/simulator/core/Neighborhood.h"
#include "game/simulator/core/ObjectModule.h"
#include "game/simulator/core/ObjSelector.h"

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
