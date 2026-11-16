#include "game/cas/CasUtils.h"

// The out-of-line copies of the three inlines in CasUtils.h; ClothingItem's inlines call them.
// Each is the inline body again: compiled through a wrapper, the compiler merges the byte
// truncations of the returns into one at the end, where the game truncates on each path.

#ifdef NON_MATCHING
// Three of 26 words by the aligned diff, and one word longer: the register copy that parks
// SimBodyPart::GetCharacterType. The game tests GetTable's result in v0, copies it to v1 in
// the branch's delay slot, reads the count through v0 and the rows through v1; this compiler
// copies first and tests the copy. Sixteen spellings tried here: the || test, nested ifs, a
// result variable, inline accessors for the count and the rows (together and apart), the
// assignment inside the condition, a second pointer variable, and three inlined out-parameter
// wrappers in the Wii's GetTable<T> shape. None keeps a copy that CSE can see through.
const CasSimTypeInfo *CasUtils::GetBodyPartTypeInfoTable()
{
    const ERQTable *table = GetQuickdata()->GetTable("Sim::BodyPartTypeInfoTableTSC6");
    if (table == 0 || table->count == 0)
        return 0;
    return (const CasSimTypeInfo *)table->rows;
}
#endif

u8 CasUtils::GetBodyPartIndex(Character::eBodyPart part)
{
    int index = part - 0x1a;
    if (index >= 0 && index < 15)
        return index;
    if (part >= 0 && part <= 0x19)
        return GetS2CEquivalentBodyPart(part) - 0x1a;
    return 14;
}

u8 CasUtils::GetBodyPartDBIndex(Character::eBodyPart part)
{
    int index = part - 0x1a;
    if (index >= 0 && index < 13)
        return index;
    if (part >= 0 && part <= 0x19) {
        Character::eBodyPart converted = GetS2CEquivalentBodyPart(part);
        index = converted - 0x1a;
        if (index >= 0 && index < 13)
            return index;
        part = converted;
    }
    if (part == 0x27)
        return 4;
    return 0x7f;
}

// The sex's row of the parts table holds one list of clothing sets per body part database
// index; the set is one entry of that list. The list's address is computed twice, once for
// each inlined VECTOR call, as the game does.
Sim::BodyPartGeometryTSC6 *CasUtils::GetClothingSetData(int male, Character::eBodyPart part,
                                                        u8 setIndex)
{
    u8 dbIndex = GetBodyPartDBIndex(part);
    if (dbIndex == 0x7f)
        return 0;
    const CasSimTextureInfo *body = GetBodyData(male);
    if (body == 0)
        return 0;
    if (setIndex >= body->parts[dbIndex].size())
        return 0;
    return &body->parts[dbIndex][setIndex];
}
