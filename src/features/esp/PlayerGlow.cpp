#include "../../Includes.h"

// -----------------------------------------------------------------------
// Player Glow — uses CS2's native engine glow system
//
// Writes to C_BaseModelEntity->m_Glow (CGlowProperty):
//   m_iGlowType         = 3 (outer glow that shows through walls)
//   m_glowColorOverride  = RGBA color
//   m_bGlowing          = true
//   m_nGlowRange        = 16384 (max visible range)
//   m_nGlowRangeMin     = 0
//
// This is rendered by CS2 engine natively — no overlay needed!
// -----------------------------------------------------------------------

void PlayerGlow::Run(const std::vector<EntityObject_t>& vecEntities)
{
    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn) return;
    if (reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000) return;
    if (!pLocalPawn->IsAlive()) return;

    // Resolve glow offsets once
    static std::once_flag flagInit;
    static std::uintptr_t uGlowOffset = 0;
    static std::uintptr_t uGlowType = 0;
    static std::uintptr_t uGlowColorOverride = 0;
    static std::uintptr_t uGlowing = 0;
    static std::uintptr_t uGlowRange = 0;
    static std::uintptr_t uGlowRangeMin = 0;
    static bool bResolved = false;

    std::call_once(flagInit, []()
    {
        // Find the Glow property offset on C_BaseModelEntity
        auto itGlow = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("C_BaseModelEntity->m_Glow"));
        if (itGlow != SchemaSystem::m_mapSchemaOffsets.end() && itGlow->second > 0)
            uGlowOffset = itGlow->second;

        // Find CGlowProperty sub-fields
        auto itType = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("CGlowProperty->m_iGlowType"));
        if (itType != SchemaSystem::m_mapSchemaOffsets.end())
            uGlowType = itType->second;

        auto itColor = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("CGlowProperty->m_glowColorOverride"));
        if (itColor != SchemaSystem::m_mapSchemaOffsets.end())
            uGlowColorOverride = itColor->second;

        auto itGlowing = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("CGlowProperty->m_bGlowing"));
        if (itGlowing != SchemaSystem::m_mapSchemaOffsets.end())
            uGlowing = itGlowing->second;

        auto itRange = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("CGlowProperty->m_nGlowRange"));
        if (itRange != SchemaSystem::m_mapSchemaOffsets.end())
            uGlowRange = itRange->second;

        auto itRangeMin = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("CGlowProperty->m_nGlowRangeMin"));
        if (itRangeMin != SchemaSystem::m_mapSchemaOffsets.end())
            uGlowRangeMin = itRangeMin->second;

        bResolved = (uGlowOffset > 0 && uGlowing > 0);

        std::cout << "  [GLOW] Glow offset:       0x" << std::hex << uGlowOffset << std::endl;
        std::cout << "  [GLOW] GlowType:          0x" << uGlowType << std::endl;
        std::cout << "  [GLOW] GlowColorOverride: 0x" << uGlowColorOverride << std::endl;
        std::cout << "  [GLOW] Glowing:           0x" << uGlowing << std::endl;
        std::cout << "  [GLOW] GlowRange:         0x" << uGlowRange << std::endl;
        std::cout << "  [GLOW] GlowRangeMin:      0x" << uGlowRangeMin << std::endl;
        std::cout << "  [GLOW] Resolved: " << (bResolved ? "YES" : "FAILED") << std::dec << std::endl;
    });

    if (!bResolved) return;

    bool  bEnemyOnly = CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowEnemyOnly);
    int   iGlowType  = CONFIG_GET(int,  g_Variables.m_PlayerGlow.m_iGlowType);
    Color colEnemy   = CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowEnemy);
    Color colTeam    = CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowTeam);

    std::uint8_t localTeam = pLocalPawn->m_iTeamNum();

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_eType != EEntityType::ENTITY_PLAYER) continue;

        try {
            CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
            if (!pCtrl) continue;
            if (pCtrl->m_bIsLocalPlayerController()) continue;

            C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pCtrl->m_hPawn().Get());
            if (!pPawn || reinterpret_cast<std::uintptr_t>(pPawn) < 0x10000) continue;
            if (!pPawn->IsAlive()) continue;

            bool bIsTeammate = (pPawn->m_iTeamNum() == localTeam);
            if (bEnemyOnly && bIsTeammate) continue;

            Color colGlow = bIsTeammate ? colTeam : colEnemy;

            std::uintptr_t uPawnAddr = reinterpret_cast<std::uintptr_t>(pPawn);
            std::uintptr_t uGlowBase = uPawnAddr + uGlowOffset;

            // Write glow type
            if (uGlowType > 0)
                g_Memory.WriteMemory<int>(uGlowBase + uGlowType, iGlowType);

            // Write glow color override (stored as RGBA int: R | G<<8 | B<<16 | A<<24)
            if (uGlowColorOverride > 0)
            {
                int iColor = colGlow.r() | (colGlow.g() << 8) | (colGlow.b() << 16) | (colGlow.a() << 24);
                g_Memory.WriteMemory<int>(uGlowBase + uGlowColorOverride, iColor);
            }

            // Write glowing = true
            if (uGlowing > 0)
                g_Memory.WriteMemory<bool>(uGlowBase + uGlowing, true);

            // Write glow range (max distance)
            if (uGlowRange > 0)
                g_Memory.WriteMemory<int>(uGlowBase + uGlowRange, 16384);

            // Write glow range min
            if (uGlowRangeMin > 0)
                g_Memory.WriteMemory<int>(uGlowBase + uGlowRangeMin, 0);

        } catch (...) { continue; }
    }
}
