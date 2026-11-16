#include "game/simulator/core/ClothingItem.h"

#include "common/ReconBuffer.h"
#include "game/cas/CasUtils.h"

s16 ClothingItem::GetID() const
{
    return field_0xe;
}

s16 ClothingItem::GetCount() const
{
    return 1;
}

u32 ClothingItem::GetGuid() const
{
    return 0;
}

s8 ClothingItem::GetCategory() const
{
    return 3;
}

s16 ClothingItem::GetOwnerID() const
{
    return field_0xc;
}

int ClothingItem::GetOwnershipPoints(s16 a) const
{
    return -1;
}

int ClothingItem::GetNumAttributes() const
{
    return -1;
}

s16 ClothingItem::GetAttribute(int a)
{
    return 0;
}

void ClothingItem::SetAttribute(int a, s16 b)
{
}

void ClothingItem::AddAtribute(s16 a)
{
}

s8 ClothingItem::GetStatusText()
{
    return -1;
}

void ClothingItem::SetStatusText(s8 a)
{
}

void ClothingItem::IncrementCount(s16 a)
{
}

void ClothingItem::DecrementCount(s16 a)
{
}

const InventoryTokens *ClothingItem::GetToken()
{
    return 0;
}

const IngredientToken *ClothingItem::GetIngredientToken()
{
    return 0;
}

void ClothingItem::LoadItemInfo(cXObject *a, Family *b, s16 c)
{
}

cXObject *ClothingItem::MakeObject(Family *a, s16 b)
{
    return 0;
}

bool ClothingItem::IsFood()
{
    return false;
}

bool ClothingItem::IsFastFood()
{
    return false;
}

bool ClothingItem::IsTool()
{
    return false;
}

bool ClothingItem::IsFastTool()
{
    return false;
}

bool ClothingItem::IsResource()
{
    return false;
}

bool ClothingItem::CanBePlanted()
{
    return false;
}

bool ClothingItem::IsSeed()
{
    return false;
}

bool ClothingItem::CanBecomeSeed()
{
    return false;
}

bool ClothingItem::IsHandObject()
{
    return false;
}

bool ClothingItem::IsClothing()
{
    return true;
}

bool ClothingItem::IsDye()
{
    return false;
}

void ClothingItem::ClearAttributes()
{
}

void ClothingItem::SetupOwnershipInfo(cXObject *a)
{
}

// The body part streams as one byte. Saves from version 0x7B on have the damaged flag.
void ClothingItem::DoStream(ReconBuffer *buffer, int version)
{
    buffer->Recon16(&field_0xe, 1);
    s8 part = bodyPart;
    buffer->Recon8(&part, 1);
    bodyPart = part;
    buffer->Recon8((s8 *)&clothingSetIndex, 1);
    buffer->Recon8((s8 *)&field_0x15, 1);
    buffer->Recon8((s8 *)&field_0x16, 1);
    buffer->Recon8((s8 *)&field_0x17, 1);
    buffer->Recon16(&field_0xc, 1);
    buffer->Recon32(&currentHitPoints, 1);
    buffer->ReconBool((bool *)&isMale);
    if (version >= 0x7B)
        buffer->ReconBool((bool *)&isDamaged);
}

inline BodyPartTypeInfo *ClothingItem::GetBodyPartTypeInfo() const
{
    const CasSimTypeInfo *table = CasUtils::GetBodyPartTypeInfoTable();
    if (table == 0)
        return 0;
    return &table->bodyPartTypes[CasUtils::GetBodyPartIndex((Character::eBodyPart)bodyPart)];
}

inline Sim::BodyPartGeometryTSC6 *ClothingItem::GetClothingSetData() const
{
    return CasUtils::GetClothingSetData(isMale, (Character::eBodyPart)bodyPart, clothingSetIndex);
}

// A piece can be dyed when its body part type allows it (flag 0x80) and its clothing set has
// more than one texture variation.
bool ClothingItem::IsDyable()
{
    BodyPartTypeInfo *info = GetBodyPartTypeInfo();
    if (info == 0)
        return false;
    if (!(info->field_0xc & 0x80))
        return false;
    Sim::BodyPartGeometryTSC6 *set = GetClothingSetData();
    if (set == 0)
        return false;
    // An if that returns true keeps the game's compare: the 1 in a register, then slt.
    if (set->textureVariations.size() > 1)
        return true;
    return false;
}
