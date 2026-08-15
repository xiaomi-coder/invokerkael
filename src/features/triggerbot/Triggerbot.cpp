#include "../../Includes.h"

// -----------------------------------------------------------------------
// Check whether our crosshair is aimed at an enemy by projecting
// all enemy bounding-box centers to screen and checking proximity.
// -----------------------------------------------------------------------
bool Triggerbot::IsEnemyUnderCrosshair(const std::vector<EntityObject_t>& vecEntities)
{
    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;

    ImVec2 screenCenter(Window::m_iWidth * 0.5f, Window::m_iHeight * 0.5f);

    bool bIgnoreTeam = CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bIgnoreTeammates);

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_eType != EEntityType::ENTITY_PLAYER) continue;

        CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
        if (!pCtrl || pCtrl->m_bIsLocalPlayerController()) continue;

        C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pCtrl->m_hPawn().Get());
        if (!pPawn || !pPawn->IsAlive()) continue;

        if (pLocalPawn && bIgnoreTeam && pPawn->m_iTeamNum() == pLocalPawn->m_iTeamNum()) continue;

        if (CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bOnlyVisible) && pLocalPawn)
        {
            Vector vecEye = pLocalPawn->GetEyePosition();
            CGameSceneNode* pNodeCheck = pPawn->m_pGameSceneNode();
            if (!pNodeCheck) continue;
            Vector vecTarget = pNodeCheck->m_vecAbsOrigin();
            vecTarget.z += 40.f;
            if (!g_Utilities.IsVisible(pPawn, vecEye, vecTarget)) continue;
        }

        CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
        if (!pNode) continue;

        Vector vecOrigin = pNode->m_vecAbsOrigin();

        // Project feet and head to screen, check if crosshair is inside the bounding column
        Vector vecFeet = vecOrigin;
        Vector vecHead = vecOrigin;
        vecHead.z += 75.f; // approximate head height
        Vector vecChest = vecOrigin;
        vecChest.z += 40.f;

        ImVec2 screenFeet, screenHead, screenChest;
        bool bFeetOk  = Draw::WorldToScreen(vecFeet,  screenFeet);
        bool bHeadOk  = Draw::WorldToScreen(vecHead,  screenHead);
        bool bChestOk = Draw::WorldToScreen(vecChest, screenChest);

        if (!bFeetOk && !bHeadOk && !bChestOk) continue;

        // Build a 2D bounding box from feet to head on screen
        float fMinY = bHeadOk ? screenHead.y : screenFeet.y;
        float fMaxY = bFeetOk ? screenFeet.y : screenHead.y;

        // Horizontal tolerance: scales with entity height (wider for close enemies)
        float fHeight    = std::fabsf(fMaxY - fMinY);
        float fHalfW     = std::max(fHeight * 0.25f, 12.f); // at least 12px, widens when close

        // Reference center: chest level
        float fCenterX   = bChestOk ? screenChest.x : (bHeadOk ? screenHead.x : screenFeet.x);
        float fCenterY   = bChestOk ? screenChest.y : ((fMinY + fMaxY) * 0.5f);

        bool bXInRange = std::fabsf(screenCenter.x - fCenterX) <= fHalfW;
        bool bYInRange = (screenCenter.y >= fMinY - 5.f) && (screenCenter.y <= fMaxY + 5.f);

        if (bXInRange && bYInRange)
            return true;
    }
    return false;
}

// -----------------------------------------------------------------------
// V2.0: Random number generator for hitchance and burst
// -----------------------------------------------------------------------
static std::mt19937& GetRNG()
{
    static std::mt19937 rng(static_cast<unsigned>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    return rng;
}

static float RandomFloat(float flMin, float flMax)
{
    std::uniform_real_distribution<float> dist(flMin, flMax);
    return dist(GetRNG());
}

static int RandomInt(int iMin, int iMax)
{
    if (iMin >= iMax) return iMin;
    std::uniform_int_distribution<int> dist(iMin, iMax);
    return dist(GetRNG());
}

// -----------------------------------------------------------------------
void Triggerbot::Run(const std::vector<EntityObject_t>& vecEntities)
{
    bool bAutoShoot = CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bAutoShoot);
    int iTriggerKey  = CONFIG_GET(int, g_Variables.m_TriggerBot.m_iTriggerKey);

    // If auto-shoot is NOT enabled, we MUST have the key pressed (unless key is 0 which was legacy auto)
    if (!bAutoShoot && iTriggerKey != 0 && !(GetAsyncKeyState(iTriggerKey) & 0x8000)) 
        return;

    if (!IsEnemyUnderCrosshair(vecEntities)) return;

    // Wait configured delay
    float flDelay = CONFIG_GET(float, g_Variables.m_TriggerBot.m_flShotDelay);
    if (flDelay > 0.f)
        g_Utilities.Sleep(flDelay);

    // V2.0: Hitchance check — random roll to decide whether to shoot
    float flHitchance = CONFIG_GET(float, g_Variables.m_TriggerBot.m_flHitchance);
    float flRoll = RandomFloat(0.f, 100.f);
    if (flRoll > flHitchance)
        return; // Missed this chance — looks human

    // V2.0: Burst mode — fire 1-N shots with small delay between
    int iMinBurst = CONFIG_GET(int, g_Variables.m_TriggerBot.m_iMinBurst);
    int iMaxBurst = CONFIG_GET(int, g_Variables.m_TriggerBot.m_iMaxBurst);
    if (iMinBurst < 1) iMinBurst = 1;
    if (iMaxBurst < iMinBurst) iMaxBurst = iMinBurst;

    int iBurstCount = RandomInt(iMinBurst, iMaxBurst);

    for (int i = 0; i < iBurstCount; i++)
    {
        // Simulate left mouse button click
        INPUT inp[2]{};
        inp[0].type       = INPUT_MOUSE;
        inp[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inp[1].type       = INPUT_MOUSE;
        inp[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(2, inp, sizeof(INPUT));

        // Small delay between burst shots (30-60ms — human-like)
        if (i < iBurstCount - 1)
            g_Utilities.Sleep(RandomFloat(30.f, 60.f));
    }
}
