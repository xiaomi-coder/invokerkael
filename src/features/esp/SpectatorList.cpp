#include "../../Includes.h"

// -----------------------------------------------------------------------
// Spectator List
// Shows which players are currently spectating the local player.
//
// IMPORTANT: All rendering uses the thread-safe Draw:: namespace,
// NOT direct ImGui calls. This is called from RenderThread (background).
// -----------------------------------------------------------------------

struct SpectatorEntry_t
{
    std::string m_strName;
};

void SpectatorList::Render(const std::vector<EntityObject_t>& vecEntities)
{
    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    CCSPlayerController* pLocalCtrl = g_Globals.m_LocalPlayer.m_pController;
    if (!pLocalPawn || !pLocalCtrl)
        return;

    CBaseHandle hLocalPawn = pLocalCtrl->m_hPawn();
    if (!hLocalPawn.IsValid())
        return;

    std::vector<SpectatorEntry_t> vecSpectators;

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_eType != EEntityType::ENTITY_PLAYER)
            continue;

        CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
        if (!pCtrl || pCtrl->m_bIsLocalPlayerController())
            continue;

        C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pCtrl->m_hPawn().Get());

        if (pPawn && pPawn->IsAlive())
            continue;

        CBaseHandle hObsPawn = pCtrl->m_hObserverPawn();
        if (!hObsPawn.IsValid())
            continue;

        C_BaseEntity* pObsEntity = C_BaseEntity::GetBaseEntity(hObsPawn.GetEntryIndex());
        if (!pObsEntity)
            continue;

        std::uintptr_t uObsPawn = reinterpret_cast<std::uintptr_t>(pObsEntity);
        if (uObsPawn < 0x10000)
            continue;

        static std::uint32_t uObsServicesOffset = SchemaSystem::m_mapSchemaOffsets[FNV1A::HashConst("C_BasePlayerPawn->m_pObserverServices")];
        static std::uint32_t uObsTargetOffset   = SchemaSystem::m_mapSchemaOffsets[FNV1A::HashConst("CPlayer_ObserverServices->m_hObserverTarget")];

        if (uObsServicesOffset == 0 || uObsTargetOffset == 0)
            continue;

        std::uintptr_t pObsServices = g_Memory.ReadMemory<std::uintptr_t>(uObsPawn + uObsServicesOffset);
        if (pObsServices < 0x10000)
            continue;

        CBaseHandle hObsTarget = g_Memory.ReadMemory<CBaseHandle>(pObsServices + uObsTargetOffset);
        if (!hObsTarget.IsValid())
            continue;

        CBaseHandle hLocalPawnEntry = pLocalCtrl->m_hPlayerPawn();
        if (!hLocalPawnEntry.IsValid())
            continue;

        if (hObsTarget.GetEntryIndex() == hLocalPawnEntry.GetEntryIndex())
        {
            std::string strName = pCtrl->m_strSanitizedPlayerName();
            if (!strName.empty())
            {
                SpectatorEntry_t entry;
                entry.m_strName = strName;
                vecSpectators.push_back(entry);
            }
        }
    }

    if (vecSpectators.empty())
        return;

    // ---------------------------------------------------------------
    // Draw spectator list using THREAD-SAFE Draw:: functions
    // (NOT direct ImGui calls — that causes crashes!)
    // ---------------------------------------------------------------
    float flScreenW = static_cast<float>(Window::m_iWidth);

    float flPadding  = 10.f;
    float flBoxW     = 200.f;
    float flLineH    = 16.f;
    float flHeaderH  = 22.f;
    float flTotalH   = flHeaderH + flLineH * static_cast<float>(vecSpectators.size()) + flPadding;

    float flX = flScreenW - flBoxW - 20.f;
    float flY = 20.f;

    // Background box
    Draw::AddRect(
        ImVec2(flX, flY),
        ImVec2(flX + flBoxW, flY + flTotalH),
        Color(10, 10, 10, 200),
        DRAW_RECT_FILLED, Color(255, 60, 60, 180), 4.f);

    // Header text: "TOMOSHABINLAR"
    float flTextX = flX + flPadding;
    float flTextY = flY + 4.f;

    Draw::AddText(Fonts::ESP, 12.f,
        ImVec2(flTextX, flTextY),
        X("TOMOSHABINLAR"),
        Color(255, 80, 80, 255), DRAW_TEXT_NONE);

    // Count badge
    char szCount[16];
    snprintf(szCount, sizeof(szCount), "[%d]", (int)vecSpectators.size());
    Draw::AddText(Fonts::ESP, 10.f,
        ImVec2(flX + flBoxW - 40.f, flTextY + 1.f),
        szCount,
        Color(255, 200, 50, 255), DRAW_TEXT_NONE);

    // Separator line
    float flSepY = flY + flHeaderH;
    Draw::AddLine(
        ImVec2(flX + 4.f, flSepY),
        ImVec2(flX + flBoxW - 4.f, flSepY),
        Color(255, 60, 60, 100));

    // Player names
    float flEntryY = flSepY + 4.f;
    for (const auto& spec : vecSpectators)
    {
        Draw::AddText(Fonts::ESP, 10.f,
            ImVec2(flTextX + 4.f, flEntryY),
            spec.m_strName,
            Color(255, 220, 220, 230), DRAW_TEXT_NONE);

        flEntryY += flLineH;
    }
}
