#include "../../Includes.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

// -----------------------------------------------------------------------
// Minimap radar rendered via thread-safe Draw:: namespace
// (Previously used ImGui::GetForegroundDrawList() which caused crashes
//  because Radar::Render is called from RenderThread, not the main thread)
// -----------------------------------------------------------------------
void Radar::Render(const std::vector<EntityObject_t>& vecEntities)
{
    float flSize  = CONFIG_GET(float, g_Variables.m_Radar.m_flRadarSize);
    float flRange = CONFIG_GET(float, g_Variables.m_Radar.m_flRadarRange);
    float flX     = CONFIG_GET(float, g_Variables.m_Radar.m_flRadarX);
    float flY     = CONFIG_GET(float, g_Variables.m_Radar.m_flRadarY);
    bool  bRotate = CONFIG_GET(bool,  g_Variables.m_Radar.m_bRadarRotate);

    // Background
    ImVec2 bgMin(flX, flY);
    ImVec2 bgMax(flX + flSize, flY + flSize);
    Draw::AddRect(bgMin, bgMax, Color(10, 10, 10, 180), DRAW_RECT_FILLED, Color(80, 80, 80, 200), 4.f);

    // Cross-hair lines
    ImVec2 center(flX + flSize * 0.5f, flY + flSize * 0.5f);
    Draw::AddLine(ImVec2(center.x, flY),         ImVec2(center.x, flY + flSize), Color(60, 60, 60, 120), 1.f);
    Draw::AddLine(ImVec2(flX, center.y),          ImVec2(flX + flSize, center.y), Color(60, 60, 60, 120), 1.f);

    // Local player
    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn) return;

    CGameSceneNode* pLocalNode = pLocalPawn->m_pGameSceneNode();
    if (!pLocalNode) return;

    Vector vecLocalPos = pLocalNode->m_vecAbsOrigin();
    QAngle angView     = g_Interfaces.m_CSGOInput.m_angViewAngle;
    float  flYawRad    = M_DEG2RAD(bRotate ? angView.y : 0.f);

    // Draw local player marker (white triangle pointing forward)
    {
        float sz = 5.f;
        float fwd = bRotate ? -(float)(M_PI / 2.0) : M_DEG2RAD(-angView.y + 90.f);
        ImVec2 tip(center.x + sz * std::cosf(fwd),        center.y + sz * std::sinf(fwd));
        ImVec2 lft(center.x + sz * std::cosf(fwd + 2.5f), center.y + sz * std::sinf(fwd + 2.5f));
        ImVec2 rgt(center.x + sz * std::cosf(fwd - 2.5f), center.y + sz * std::sinf(fwd - 2.5f));
        Draw::AddTriangle(tip, lft, rgt, Color(255, 255, 255, 255), DRAW_TRIANGLE_FILLED);
    }

    float flScale = (flSize * 0.5f) / flRange;

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_eType != EEntityType::ENTITY_PLAYER) continue;

        CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
        if (!pCtrl) continue;

        // Safe null check before accessing pawn
        try {
            if (pCtrl->m_bIsLocalPlayerController()) continue;
            auto hPawn = pCtrl->m_hPawn();
            C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(hPawn.Get());
            if (!pPawn || !pPawn->IsAlive()) continue;

            CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
            if (!pNode) continue;

            Vector vecPos = pNode->m_vecAbsOrigin();
            float  dx     = vecPos.x - vecLocalPos.x;
            float  dy     = vecPos.y - vecLocalPos.y;

            // Optionally rotate around local player view
            float rdx = dx, rdy = dy;
            if (bRotate)
            {
                float cosA = std::cosf(-flYawRad);
                float sinA = std::sinf(-flYawRad);
                rdx = dx * cosA - dy * sinA;
                rdy = dx * sinA + dy * cosA;
            }

            float screenX = center.x + rdx * flScale;
            float screenY = center.y - rdy * flScale; // y is flipped

            // Clamp inside radar circle area
            float cx = screenX - center.x;
            float cy = screenY - center.y;
            float dist = std::sqrtf(cx * cx + cy * cy);
            float maxR  = flSize * 0.5f - 6.f;
            if (dist > maxR)
            {
                float norm = maxR / dist;
                screenX = center.x + cx * norm;
                screenY = center.y + cy * norm;
            }

            bool bSameTeam = (pPawn->m_iTeamNum() == pLocalPawn->m_iTeamNum());
            Color colDot   = bSameTeam ? Color(50, 220, 50, 255) : Color(220, 50, 50, 255);

            // Filled circle dot
            Draw::AddCircle(ImVec2(screenX, screenY), 4.f, colDot, 12, DRAW_CIRCLE_FILLED, Color(0, 0, 0, 200));

            // Player name (small)
            std::string szName = pCtrl->m_strSanitizedPlayerName();
            if (!szName.empty() && Fonts::ESP)
            {
                // Remove invalid characters to prevent ImGui UTF-8 crash
                szName.erase(std::remove_if(szName.begin(), szName.end(), [](unsigned char c) {
                    return c < 32 || c > 126;
                }), szName.end());

                if (!szName.empty())
                {
                    Draw::AddText(Fonts::ESP, 8.f, ImVec2(screenX + 5.f, screenY - 4.f),
                                  szName, colDot, DRAW_TEXT_NONE);
                }
            }

        } catch (...) { continue; } // safety catch for invalid memory
    }
}

// -----------------------------------------------------------------------
// Force all enemies to appear on CS2's built-in radar
// by writing m_bSpotted = true in their EntitySpottedState_t.
//
// This is called from TickThread (not RenderThread) since it does memory writes.
// -----------------------------------------------------------------------
void Radar::ForceRadarSpotted(const std::vector<EntityObject_t>& vecEntities)
{
    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn) return;

    if (reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000) return;
    if (!pLocalPawn->IsAlive()) return;

    // Get the schema offset for entitySpottedState once
    static std::uintptr_t uSpottedStateOffset = 0;
    static std::uintptr_t uSpottedOffset = 0;
    static bool bOffsetsResolved = false;
    static std::once_flag flagInit;

    std::call_once(flagInit, []()
    {
        auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("C_CSPlayerPawn->m_entitySpottedState"));
        if (it != SchemaSystem::m_mapSchemaOffsets.end() && it->second > 0)
        {
            uSpottedStateOffset = it->second;
            // m_bSpotted is at offset 0x8 within EntitySpottedState_t (after the 0x8 pad)
            uSpottedOffset = uSpottedStateOffset + 0x8;
            bOffsetsResolved = true;
        }

        std::cout << "  [RADAR] SpottedState offset: 0x" << std::hex << uSpottedStateOffset << std::dec << std::endl;
        std::cout << "  [RADAR] Spotted offset:      0x" << std::hex << uSpottedOffset << std::dec << std::endl;
        std::cout << "  [RADAR] Resolved: " << (bOffsetsResolved ? "YES" : "FAILED") << std::endl;
    });

    if (!bOffsetsResolved) return;

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

            // Only spot enemies (different team)
            if (pPawn->m_iTeamNum() == localTeam) continue;

            // Write m_bSpotted = true
            std::uintptr_t uPawnAddr = reinterpret_cast<std::uintptr_t>(pPawn);
            bool bCurrentSpotted = g_Memory.ReadMemory<bool>(uPawnAddr + uSpottedOffset);
            if (!bCurrentSpotted)
            {
                g_Memory.WriteMemory<bool>(uPawnAddr + uSpottedOffset, true);
            }
        } catch (...) { continue; }
    }
}

// -----------------------------------------------------------------------
// AuditorySonar: Beeps when aiming near an enemy
// -----------------------------------------------------------------------
void Radar::AuditorySonar(const std::vector<EntityObject_t>& vecEntities)
{
    if (!CONFIG_GET(bool, g_Variables.m_Misc.m_bEnableSonar)) return;

    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn || !pLocalPawn->IsAlive()) return;

    Vector vecLocalEye = pLocalPawn->GetEyePosition();
    QAngle angView = g_Interfaces.m_CSGOInput.m_angViewAngle;
    
    float flMaxFOV = CONFIG_GET(float, g_Variables.m_Misc.m_flSonarFOV);
    float flClosestDist = FLT_MAX;
    bool  bFound = false;

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_eType != EEntityType::ENTITY_PLAYER) continue;

        CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
        if (!pCtrl || pCtrl->m_bIsLocalPlayerController()) continue;

        C_CSPlayerPawn* pEnemyPawn = reinterpret_cast<C_CSPlayerPawn*>(pCtrl->m_hPawn().Get());
        if (!pEnemyPawn || !pEnemyPawn->IsAlive()) continue;
        if (pEnemyPawn->m_iTeamNum() == pLocalPawn->m_iTeamNum()) continue;

        Vector vecEnemyPos = pEnemyPawn->m_pGameSceneNode()->m_vecAbsOrigin() + Vector(0,0,35.f); // point roughly to torso
        
        ImVec2 screenPos;
        if (!Draw::WorldToScreen(vecEnemyPos, screenPos)) continue;

        float flDX = screenPos.x - Window::m_iWidth * 0.5f;
        float flDY = screenPos.y - Window::m_iHeight * 0.5f;
        float flScreenDist = std::sqrt(flDX*flDX + flDY*flDY);
        
        // FOV multiplier: 1 degree is roughly 15 pixels on a 1080p screen without scope
        if (flScreenDist < flMaxFOV * 25.f)
        {
            float flDist = vecLocalEye.DistTo(vecEnemyPos);
            if (flDist < flClosestDist)
            {
                flClosestDist = flDist;
                bFound = true;
            }
        }
    }

    if (bFound)
    {
        static ULONGLONG ullLastBeep = 0;
        
        // Closer distance = faster ping!
        float flInterval = std::clamp(flClosestDist * 0.4f, 80.f, 1500.f);

        if (GetTickCount64() - ullLastBeep > flInterval)
        {
            ullLastBeep = GetTickCount64();
            
            // Beep() and MessageBeep(0xFFFF) often depend on disabled system sounds.
            // We use PlaySoundA to play a custom file or a very soft/short default Windows UI sound.
            std::thread([]() {
                // If the user drops their own "radar_ping.wav" next to the cheat .exe, it plays that.
                // Otherwise, it plays the soft "Windows Navigation Start.wav"
                if (GetFileAttributesA("radar_ping.wav") != INVALID_FILE_ATTRIBUTES) {
                    PlaySoundA("radar_ping.wav", NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
                } else {
                    PlaySoundA("C:\\Windows\\Media\\Windows Navigation Start.wav", NULL, SND_FILENAME | SND_ASYNC);
                }
            }).detach();
        }
    }
}

