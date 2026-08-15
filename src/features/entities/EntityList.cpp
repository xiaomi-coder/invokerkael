#include "../../Includes.h"

void EntityList::UpdateEntities()
{
	m_vecEntities.clear();

	int iPlayerCount = 0;
	int iTotalValid = 0;

	for (int i = 0; i <= 8192; ++i)
	{
		C_BaseEntity* pBaseEntity = C_BaseEntity::GetBaseEntity(i);
		if (!pBaseEntity)
			continue;

		std::uintptr_t uBase = reinterpret_cast<std::uintptr_t>(pBaseEntity);
		if (uBase < 0x10000) continue;
		iTotalValid++;

		const std::string strSchemaName = pBaseEntity->GetSchemaName();

		if (strSchemaName.empty()) continue;

		const FNV1A_t uSchemaNameHash = FNV1A::Hash(strSchemaName.c_str());

		if (strSchemaName.find("C_Weapon") != std::string::npos || strSchemaName.find("weapon_") != std::string::npos || strSchemaName == "C_DEagle" || strSchemaName == "C_AK47" || strSchemaName == "C_C4")
		{
			// Only show dropped weapons (not held by anyone)
			if (pBaseEntity->m_hOwnerEntity().IsValid())
				continue;

			// Skip knives
			if (strSchemaName.find("Knife") == std::string::npos && strSchemaName.find("Bayonet") == std::string::npos)
			{
				m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, i, EEntityType::ENTITY_WEAPON, uSchemaNameHash));
				continue;
			}
		}

		switch (uSchemaNameHash)
		{
		case FNV1A::HashConst("CCSPlayerController"):
		case FNV1A::HashConst("cs_player_controller"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, i, EEntityType::ENTITY_PLAYER, uSchemaNameHash));
			iPlayerCount++;
			break;
		}
		case FNV1A::HashConst("C_PlantedC4"):
		case FNV1A::HashConst("CPlantedC4"):
		case FNV1A::HashConst("planted_c4"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, i, EEntityType::ENTITY_PLANTEDC4, uSchemaNameHash));
			break;
		}
		case FNV1A::HashConst("C_SmokeGrenadeProjectile"):
		case FNV1A::HashConst("C_MolotovProjectile"):
		case FNV1A::HashConst("C_HEGrenadeProjectile"):
		case FNV1A::HashConst("C_FlashbangProjectile"):
		case FNV1A::HashConst("C_DecoyProjectile"):
		case FNV1A::HashConst("smokegrenade_projectile"):
		case FNV1A::HashConst("molotov_projectile"):
		case FNV1A::HashConst("hegrenade_projectile"):
		case FNV1A::HashConst("flashbang_projectile"):
		case FNV1A::HashConst("decoy_projectile"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, i, EEntityType::ENTITY_GRENADE, uSchemaNameHash));
			break;
		}
		case FNV1A::HashConst("C_EnvTonemapController"):
		case FNV1A::HashConst("env_tonemap_controller"):
		{
			m_vecEntities.emplace_back(EntityObject_t(pBaseEntity, i, EEntityType::ENTITY_WORLD, uSchemaNameHash));
			break;
		}
		}
	}
}
