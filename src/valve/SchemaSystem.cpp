#include "../Includes.h"

bool SchemaSystem::Setup()
{
    std::uintptr_t uSchemaInterfaceAddress = g_Memory.PatternScan(SCHEMASYSTEM_DLL, X("48 89 05 ? ? ? ? 4C 8D 0D ? ? ? ? 33 C0 48 C7 05 ? ? ? ? ? ? ? ? 89 05"), EPatternScanFlags::SCAN_RESOLVE_RIP, 0x3, 0x7);
    std::uintptr_t uSchemaSystemScopeArrayPtr = 0U;
    if (!g_Memory.ReadMemoryRaw(uSchemaInterfaceAddress + CS_OFFSETOF(CSchemaSystem, m_pScopeArray), &uSchemaSystemScopeArrayPtr, sizeof(std::uintptr_t)))
    {
        std::cout << X("Failed to read scope array ptr") << std::endl;
        ApplyFallbacks();
        return false;
    }
  
    int nScopeSize = g_Memory.ReadMemory<int>(uSchemaInterfaceAddress + CS_OFFSETOF(CSchemaSystem, m_nScopeSize));
    void** ppScopeArray = new void* [nScopeSize];

    if (!g_Memory.ReadMemoryRaw(uSchemaSystemScopeArrayPtr, ppScopeArray, (nScopeSize * sizeof(void*))))
    {
        std::cout << X("Failed to read scope array") << std::endl;
        ApplyFallbacks();
        return false;
    }

    for (std::uint16_t i = 0U; i <= nScopeSize; ++i)
    {
        CSchemaSystemTypeScope schemaScope{};
        if (!g_Memory.ReadMemoryRaw(ppScopeArray[i], &schemaScope, sizeof(CSchemaSystemTypeScope)) || !schemaScope.m_pDeclaredClasses)
            continue;

        std::vector<CSchemaDeclaredClassEntry> vecDeclaredClassEntries(schemaScope.m_uNumDeclaredClasses + 1U);
        CSchemaDeclaredClassEntry* pDeclaredClassEntries = vecDeclaredClassEntries.data();
        if (!g_Memory.ReadMemoryRaw(schemaScope.m_pDeclaredClasses, pDeclaredClassEntries, (schemaScope.m_uNumDeclaredClasses + 1U) * sizeof(CSchemaDeclaredClassEntry)))
            continue;
     
        if (strcmp(schemaScope.m_szName, X("client.dll")) == 0)
        {     
            for (std::uint16_t j = 0U; j < schemaScope.m_uNumDeclaredClasses; ++j)
            {
                CSchemaDeclaredClass declaredClass{ };
                if (!g_Memory.ReadMemoryRaw(pDeclaredClassEntries[j].m_pDeclaredClass, &declaredClass, sizeof(CSchemaDeclaredClass)))
                    continue;

                CSchemaClass schemaClass{ };
                if (!g_Memory.ReadMemoryRaw(declaredClass.m_Class, &schemaClass, sizeof(CSchemaClass)))
                    continue;

                char szClassName[128]{};
                if (!g_Memory.ReadMemoryRaw((void*)(declaredClass.m_szName), szClassName, sizeof(szClassName)))
                    continue;

                std::uintptr_t uClassFieldsPtr = reinterpret_cast<uintptr_t>(schemaClass.m_pFields);
                if (uClassFieldsPtr)
                {
                    for (std::uint16_t k = 0; k < schemaClass.m_uNumFields; ++k)
                    {
                        CSchemaField schemaField = g_Memory.ReadMemory<CSchemaField>(uClassFieldsPtr + (sizeof(CSchemaField) * k));
                        if (!schemaField.m_pType)
                            continue;

                        char szFieldName[128] = { 0 };
                        if (!g_Memory.ReadMemoryRaw((void*)(schemaField.m_szName), szFieldName, sizeof(szFieldName)))
                            continue;

                        const std::string strSchemaField = std::vformat(X("{}->{}"), std::make_format_args(szClassName, szFieldName));
                        m_mapSchemaOffsets[FNV1A::Hash(strSchemaField.c_str())] = schemaField.m_uOffset;
                    }
                }
            }
        }    
    }

    delete[] ppScopeArray;

	ApplyFallbacks();

	return m_mapSchemaOffsets.size() > 0;
}

void SchemaSystem::ApplyFallbacks()
{
	auto SetIfZero = [](FNV1A_t hash, std::uint32_t fallback) {
		if (m_mapSchemaOffsets.find(hash) == m_mapSchemaOffsets.end() || m_mapSchemaOffsets[hash] == 0)
			m_mapSchemaOffsets[hash] = fallback;
	};

	// C_BaseEntity
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_pGameSceneNode"), 816);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_iHealth"), 844);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_iMaxHealth"), 840);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_iTeamNum"), 999);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_lifeState"), 852);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_pCollision"), 832);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_fFlags"), 1012);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_hOwnerEntity"), 1312);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_vecVelocity"), 1072);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_vecAbsVelocity"), 1016);
	SetIfZero(FNV1A::HashConst("C_BaseEntity->m_nSubclassID"), 896);

	// CCSPlayerController
	SetIfZero(FNV1A::HashConst("CCSPlayerController->m_sSanitizedPlayerName"), 2152);

	// C_PlantedC4
	SetIfZero(FNV1A::HashConst("C_PlantedC4->m_flC4Blow"), 4560);
	SetIfZero(FNV1A::HashConst("C_PlantedC4->m_bBombDefused"), 4596);

	// CGameSceneNode
	SetIfZero(FNV1A::HashConst("CGameSceneNode->m_vecAbsOrigin"), 200);
	SetIfZero(FNV1A::HashConst("CGameSceneNode->m_bDormant"), 259);
	SetIfZero(FNV1A::HashConst("CGameSceneNode->m_nodeToWorld"), 16);

	// CCollisionProperty
	SetIfZero(FNV1A::HashConst("CCollisionProperty->m_vecMins"), 64);
	SetIfZero(FNV1A::HashConst("CCollisionProperty->m_vecMaxs"), 76);

	// CBasePlayerController
	SetIfZero(FNV1A::HashConst("CBasePlayerController->m_hPawn"), 1724);
	SetIfZero(FNV1A::HashConst("CBasePlayerController->m_iszPlayerName"), 1780);
	SetIfZero(FNV1A::HashConst("CBasePlayerController->m_iDesiredFOV"), 1932);   // FOV changer

	// C_PostProcessingVolume (tungi rejim — aksar xaritalarda yorug'likni shu boshqaradi)
	SetIfZero(FNV1A::HashConst("C_PostProcessingVolume->m_flMinExposure"), 4284);
	SetIfZero(FNV1A::HashConst("C_PostProcessingVolume->m_flMaxExposure"), 4288);
	SetIfZero(FNV1A::HashConst("C_PostProcessingVolume->m_bExposureControl"), 4309);
	SetIfZero(FNV1A::HashConst("C_PostProcessingVolume->m_bMaster"), 4308);
	SetIfZero(FNV1A::HashConst("C_PostProcessingVolume->m_flFadeDuration"), 4272);
	SetIfZero(FNV1A::HashConst("C_PostProcessingVolume->m_flExposureCompensation"), 4292);

	// C_TonemapController2 (tungi rejim — CS2 dagi yangi klass nomi)
	SetIfZero(FNV1A::HashConst("C_TonemapController2->m_flAutoExposureMin"), 1536);
	SetIfZero(FNV1A::HashConst("C_TonemapController2->m_flAutoExposureMax"), 1540);

	// C_BaseModelEntity
	SetIfZero(FNV1A::HashConst("C_BaseModelEntity->m_vecViewOffset"), 3704);
	SetIfZero(FNV1A::HashConst("C_BaseModelEntity->m_Glow"), 3552);

	// C_EconEntity
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_AttributeManager"), 4520);
	SetIfZero(FNV1A::HashConst("C_AttributeContainer->m_Item"), 80);
	SetIfZero(FNV1A::HashConst("C_EconItemView->m_iItemDefinitionIndex"), 442);

	// CGlowProperty
	SetIfZero(FNV1A::HashConst("CGlowProperty->m_iGlowType"), 48);
	SetIfZero(FNV1A::HashConst("CGlowProperty->m_glowColorOverride"), 64);
	SetIfZero(FNV1A::HashConst("CGlowProperty->m_bGlowing"), 81);
	SetIfZero(FNV1A::HashConst("CGlowProperty->m_nGlowRange"), 56);
	SetIfZero(FNV1A::HashConst("CGlowProperty->m_nGlowRangeMin"), 60);

	// C_BasePlayerPawn
	SetIfZero(FNV1A::HashConst("C_BasePlayerPawn->m_pWeaponServices"), 4616);
	SetIfZero(FNV1A::HashConst("C_BasePlayerPawn->m_pObserverServices"), 4640);
	SetIfZero(FNV1A::HashConst("C_BasePlayerPawn->m_pViewModelServices"), 4688);
	SetIfZero(FNV1A::HashConst("C_BasePlayerPawn->m_pMovementServices"), 4680);

	// CPlayer_WeaponServices
	SetIfZero(FNV1A::HashConst("CPlayer_WeaponServices->m_hActiveWeapon"), 96);
	SetIfZero(FNV1A::HashConst("CPlayer_WeaponServices->m_hMyWeapons"), 72);

	// CBasePlayerController - CRITICAL for ESP
	SetIfZero(FNV1A::HashConst("CBasePlayerController->m_bIsLocalPlayerController"), 1928);

	// CCSPlayerController
	SetIfZero(FNV1A::HashConst("CCSPlayerController->m_hPlayerPawn"), 2324);
	SetIfZero(FNV1A::HashConst("CCSPlayerController->m_hObserverPawn"), 2328);
	SetIfZero(FNV1A::HashConst("CCSPlayerController->m_sSanitizedPlayerName"), 2152);
	SetIfZero(FNV1A::HashConst("CCSPlayerController->m_iCompTeammateColor"), 2128);
	SetIfZero(FNV1A::HashConst("CCSPlayerController->m_bPawnIsAlive"), 2332);

	// CPlayer_ObserverServices - NOT in client.dll schema, MUST have fallback!
	SetIfZero(FNV1A::HashConst("CPlayer_ObserverServices->m_hObserverTarget"), 76);
	SetIfZero(FNV1A::HashConst("CPlayer_ObserverServices->m_iObserverMode"), 72);

	// C_CSPlayerPawnBase / C_CSPlayerPawn
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawnBase->m_flFlashMaxAlpha"), 5156);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawnBase->m_flFlashDuration"), 5160);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawnBase->m_entitySpottedState"), 4504);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawn->m_entitySpottedState"), 7264);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawn->m_bIsScoped"), 7288);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawn->m_bIsWalking"), 7256);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawn->m_bIsDefusing"), 7290);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawn->m_ArmorValue"), 7332);
	SetIfZero(FNV1A::HashConst("C_CSPlayerPawn->m_iShotsFired"), 7308);

	// EntitySpottedState_t
	SetIfZero(FNV1A::HashConst("EntitySpottedState_t->m_bSpotted"), 8);
	SetIfZero(FNV1A::HashConst("EntitySpottedState_t->m_bSpottedByMask"), 12);

	// ===================== SKIN CHANGER OFFSETS =====================
	// C_EconEntity (Fallback fields — on the weapon entity itself)
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_AttributeManager"), 4520);
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_nFallbackPaintKit"), 5760);
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_nFallbackSeed"), 5764);
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_flFallbackWear"), 5768);
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_nFallbackStatTrak"), 5772);
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_OriginalOwnerXuidLow"), 5752);
	SetIfZero(FNV1A::HashConst("C_EconEntity->m_OriginalOwnerXuidHigh"), 5756);

	// C_AttributeContainer
	SetIfZero(FNV1A::HashConst("C_AttributeContainer->m_Item"), 80);

	// C_EconItemView (inside AttributeManager->Item)
	SetIfZero(FNV1A::HashConst("C_EconItemView->m_iItemDefinitionIndex"), 442);
	SetIfZero(FNV1A::HashConst("C_EconItemView->m_iItemIDHigh"), 464);
	SetIfZero(FNV1A::HashConst("C_EconItemView->m_iItemIDLow"), 468);
	SetIfZero(FNV1A::HashConst("C_EconItemView->m_iAccountID"), 472);
	SetIfZero(FNV1A::HashConst("C_EconItemView->m_iEntityQuality"), 444);

	std::cout << "  [+] Schema offsets: " << m_mapSchemaOffsets.size() << " entries (with fallbacks)" << std::endl;
}