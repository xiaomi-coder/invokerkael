#include "../../Includes.h"

void EntityList::UpdateEntities()
{
	m_vecEntities.clear();

	for(CEntityIdentity* pEntity = g_Interfaces.m_GameEntitySystem.m_pFirst; pEntity != nullptr; pEntity = pEntity->m_pNext())
	{
		C_BaseEntity* pBaseEntity = reinterpret_cast<C_BaseEntity*>(pEntity->m_pInstance());
		if (!pBaseEntity)
			continue;

		std::uintptr_t uBase = reinterpret_cast<std::uintptr_t>(pBaseEntity);
		if (uBase < 0x10000) continue;

		const std::string strSchemaName = pBaseEntity->GetSchemaName();
		if (strSchemaName.empty()) continue;

		const FNV1A_t uSchemaNameHash = FNV1A::Hash(strSchemaName.c_str());

		if (strSchemaName.find("C_Weapon") != std::string::npos || strSchemaName == "C_DEagle" || strSchemaName == "C_AK47")
		{
			// Only show dropped weapons (not held by anyone)
			if (pBaseEntity->m_hOwnerEntity().IsValid())
				continue;

			// Skip knives
			if (strSchemaName.find("Knife") == std::string::npos && strSchemaName.find("Bayonet") == std::string::npos)
			{
				m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, pBaseEntity->GetRefEHandle().GetEntryIndex(), EEntityType::ENTITY_WEAPON, uSchemaNameHash));
				continue;
			}
		}

		switch (uSchemaNameHash)
		{
		case FNV1A::HashConst("CCSPlayerController"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, pBaseEntity->GetRefEHandle().GetEntryIndex(), EEntityType::ENTITY_PLAYER, uSchemaNameHash));
			break;
		}
		case FNV1A::HashConst("C_PlantedC4"):
		case FNV1A::HashConst("CPlantedC4"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, pBaseEntity->GetRefEHandle().GetEntryIndex(), EEntityType::ENTITY_PLANTEDC4, uSchemaNameHash));
			break;
		}
		case FNV1A::HashConst("C_SmokeGrenadeProjectile"):
		case FNV1A::HashConst("C_MolotovProjectile"):
		case FNV1A::HashConst("C_HEGrenadeProjectile"):
		case FNV1A::HashConst("C_FlashbangProjectile"):
		case FNV1A::HashConst("C_DecoyProjectile"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, pBaseEntity->GetRefEHandle().GetEntryIndex(), EEntityType::ENTITY_GRENADE, uSchemaNameHash));
			break;
		}
		case FNV1A::HashConst("C_EnvTonemapController"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, pBaseEntity->GetRefEHandle().GetEntryIndex(), EEntityType::ENTITY_WORLD, uSchemaNameHash));
			break;
		}
		}
	}
}
