#include "../../Includes.h"

C_BaseEntity* CBaseHandle::Get() const
{
    if (!IsValid())
        return nullptr;

    C_BaseEntity* pEntity = C_BaseEntity::GetBaseEntity(GetEntryIndex());
    if (!pEntity)
        return nullptr;

    // Skip GetRefEHandle check — not reliable with new entity list layout
    return pEntity;
}