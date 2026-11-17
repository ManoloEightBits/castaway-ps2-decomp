#include "game/simulator/core2/SlotLoader.h"

// 0x1E1C38
SlotLoader::SlotLoader(u32 namespaceID)
{
    mNamespaceID = namespaceID;
    field_0x4 = 0;
}

// 0x1E1C48. Empty, so the body is the compiler's: delete this when the in-charge flag's low
// bit is set.
SlotLoader::~SlotLoader()
{
}
