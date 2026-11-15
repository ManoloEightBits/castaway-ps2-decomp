#include "game/simulator/core/OwnableItem.h"

#include "common/ReconBuffer.h"
#include "game/simulator/core/Inventory.h"
#include "game/simulator/core2/RelMatrix.h"

s8 OwnableItem::GetCategory() const
{
    return 1;
}

// Fills the owners from the object's relationships: every Sim with ownership points above 0.
void OwnableItem::SetupOwnershipInfo(cXObject *object)
{
    mOwners.clear();
    RelMatrix *relations = object->mRelMatrix;
    for (int i = 0; i < relations->Count(); i++) {
        OwnerInfo info;
        info.mOwner = relations->GetIDByIndex(i);
        info.mPoints = relations->GetRelationship(info.mOwner, 0);
        if (info.mPoints > 0)
            AddOwner(info);
    }
}

// ObjectItem's MakeObject, then LoadItemInfo once more (ObjectItem's already ran it once).
cXObject *OwnableItem::MakeObject(Family *family, s16 a)
{
    cXObject *object = MakeObjectInline(family, a);
    LoadItemInfo(object, family, a);
    return object;
}

// ObjectItem's LoadItemInfo, then the owners go back into the object's relationships.
void OwnableItem::LoadItemInfo(cXObject *object, Family *family, s16 a)
{
    LoadItemInfoInline(object, family, a);
    RelMatrix *relations = object->mRelMatrix;
    for (OwnerInfo *i = mOwners.begin(); i != mOwners.end(); ++i)
        relations->SetRelationship(i->mOwner, 0, i->mPoints);
}

// ObjectItem's fields, then the owners.
void OwnableItem::DoStream(ReconBuffer *buffer, int version)
{
    ObjectItem::DoStream(buffer, version);
    s16 count = mOwners.size();
    buffer->Recon16(&count, 1);
    for (int i = 0; i < count; i++) {
        if (buffer->IsReading()) {
            OwnerInfo info;
            buffer->Recon16(&info.mOwner, 1);
            buffer->Recon16(&info.mPoints, 1);
            mOwners.push_back(info);
        } else {
            OwnerInfo info = mOwners[i];
            buffer->Recon16(&info.mOwner, 1);
            buffer->Recon16(&info.mPoints, 1);
        }
    }
}
