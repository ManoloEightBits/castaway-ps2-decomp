#include "game/simulator/core/ObjectItem.h"

#include "common/ReconBuffer.h"
#include "game/simulator/core/Inventory.h"

s16 ObjectItem::GetID() const
{
    return 0;
}

u32 ObjectItem::GetGuid() const
{
    return field_0x8;
}

s8 ObjectItem::GetCategory() const
{
    return 0;
}

s16 ObjectItem::GetCount() const
{
    return 1;
}

s16 ObjectItem::GetOwnerID() const
{
    return -1;
}

int ObjectItem::GetOwnershipPoints(s16 a) const
{
    return -1;
}

s8 ObjectItem::GetStatusText()
{
    return field_0xc;
}

void ObjectItem::SetStatusText(s8 value)
{
    field_0xc = value;
}

void ObjectItem::IncrementCount(s16 a)
{
}

void ObjectItem::DecrementCount(s16 a)
{
}

const InventoryTokens *ObjectItem::GetToken()
{
    return 0;
}

const IngredientToken *ObjectItem::GetIngredientToken()
{
    return 0;
}

bool ObjectItem::IsFood()
{
    return false;
}

bool ObjectItem::IsFastFood()
{
    return false;
}

bool ObjectItem::IsTool()
{
    return false;
}

bool ObjectItem::IsFastTool()
{
    return false;
}

bool ObjectItem::IsResource()
{
    return false;
}

bool ObjectItem::CanBePlanted()
{
    return false;
}

bool ObjectItem::IsHandObject()
{
    return false;
}

bool ObjectItem::IsClothing()
{
    return false;
}

bool ObjectItem::IsDye()
{
    return false;
}

bool ObjectItem::IsDyable()
{
    return false;
}

void ObjectItem::SetupOwnershipInfo(cXObject *a)
{
}

int ObjectItem::GetNumAttributes() const
{
    return mAttributes.size();
}

s16 ObjectItem::GetAttribute(int index)
{
    return mAttributes[index];
}

void ObjectItem::SetAttribute(int index, s16 value)
{
    mAttributes[index] = value;
}

void ObjectItem::AddAtribute(s16 value)
{
    mAttributes.push_back(value);
}

void ObjectItem::ClearAttributes()
{
    mAttributes.erase(mAttributes.begin(), mAttributes.end());
}

// Streams the GUID, the status byte and the attributes. Saves from before version 0x6D have no
// owner fields, and before 0x71 a bool stood where the third one is.
void ObjectItem::DoStream(ReconBuffer *buffer, int version)
{
    buffer->Recon32((int *)&field_0x8, 1);
    buffer->Recon8(&field_0xc, 1);
    s16 count = mAttributes.size();
    buffer->Recon16(&count, 1);
    for (int i = 0; i < count; i++) {
        if (buffer->IsReading()) {
            s16 value;
            buffer->Recon16(&value, 1);
            mAttributes.push_back(value);
        } else {
            buffer->Recon16(&mAttributes[i], 1);
        }
    }
    if (version >= 0x6D) {
        buffer->Recon16(&field_0xe, 1);
        buffer->Recon16(&field_0x10, 1);
    }
    if (version < 0x71) {
        bool unused;
        buffer->ReconBool(&unused);
        field_0x12 = 0;
    } else {
        buffer->Recon16(&field_0x12, 1);
    }
}

cXObject *ObjectItem::MakeObject(Family *family, s16 a)
{
    return MakeObjectInline(family, a);
}

void ObjectItem::LoadItemInfo(cXObject *object, Family *family, s16 a)
{
    LoadItemInfoInline(object, family, a);
}
