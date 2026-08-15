#include "../../Includes.h"
#include "../../config/Variables.h"


static constexpr int BONE_HEAD  = 6;
static constexpr int BONE_NECK  = 5;
static constexpr int BONE_CHEST = 4;

// -----------------------------------------------------------------------
// Get hitbox position for a player.
// Tries bone cache first, falls back to m_vecAbsOrigin + height offset.
// -----------------------------------------------------------------------
Vector Aimbot::GetHitboxPosition(C_CSPlayerPawn* pPawn, int iHitbox)
{
    CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
    if (!pNode) return {};

    Vector vecOrigin = pNode->m_vecAbsOrigin();
    if (vecOrigin.IsZero()) return {};

    // --- Try bone data first ---
    BoneData_t* pBones = pNode->m_pBoneCache();
    if (pBones && reinterpret_cast<std::uintptr_t>(pBones) > 0x10000)
    {
        int iBone = BONE_HEAD;
        if (iHitbox == 1) iBone = BONE_NECK;
        else if (iHitbox == 2) iBone = BONE_CHEST;

        BoneData_t bone = g_Memory.ReadMemory<BoneData_t>(
            reinterpret_cast<std::uintptr_t>(pBones) + iBone * sizeof(BoneData_t));

        // Validate: bone position should be finite and not zero
        if (!bone.m_vecPosition.IsZero() &&
            std::isfinite(bone.m_vecPosition.x) &&
            std::isfinite(bone.m_vecPosition.y) &&
            std::isfinite(bone.m_vecPosition.z))
        {
            return bone.m_vecPosition;
        }
    }

    // --- Fallback: estimate from m_vecAbsOrigin + height ---
    // Detect crouch: read collision bounds to check player height
    // Standing player: ~72u tall, Crouching: ~54u tall
    float flHeadOfs = 64.f;
    float flNeckOfs = 55.f;
    float flChestOfs = 38.f;

    // Try to detect crouch via m_vecViewOffset (from schema)

    // Read collision bounds from pawn for height detection
    // C_BaseEntity->m_pCollision->m_vecMaxs.z tells us actual height
    std::uintptr_t uPawnAddr = reinterpret_cast<std::uintptr_t>(pPawn);
    if (uPawnAddr > 0x10000)
    {
        // Read view offset Z — this tells us standing vs crouching
        // Standing eye height: ~64, Crouching eye height: ~46
        // We use CCSPlayerPawn schema if available
        static std::uint32_t uViewOffsetZ = SchemaSystem::m_mapSchemaOffsets[FNV1A::HashConst("C_BaseModelEntity->m_vecViewOffset")];
        if (uViewOffsetZ != 0)
        {
            Vector vecViewOffset = g_Memory.ReadMemory<Vector>(uPawnAddr + uViewOffsetZ);
            if (std::isfinite(vecViewOffset.z) && vecViewOffset.z > 1.f)
            {
                // Use view offset directly as head height (most accurate)
                flHeadOfs = vecViewOffset.z;
                flNeckOfs = vecViewOffset.z - 7.f;
                flChestOfs = vecViewOffset.z - 24.f;
            }
        }
    }

    if (iHitbox == 0)       // Head
        vecOrigin.z += flHeadOfs;
    else if (iHitbox == 1)  // Neck
        vecOrigin.z += flNeckOfs;
    else if (iHitbox == 2)  // Chest
        vecOrigin.z += flChestOfs;

    return vecOrigin;
}

// -----------------------------------------------------------------------
QAngle Aimbot::SmoothAngle(const QAngle& angCurrent, const QAngle& angTarget, float flSmooth)
{
    if (flSmooth <= 1.f) return angTarget;

    QAngle angDelta = angTarget - angCurrent;
    angDelta.Normalize();

    return angCurrent + (angDelta / flSmooth);
}

// -----------------------------------------------------------------------
C_CSPlayerPawn* Aimbot::FindBestTarget(const std::vector<EntityObject_t>& vecEntities, float& flBestFOV)
{
    C_CSPlayerPawn* pBest = nullptr;
    flBestFOV = 999999.f;

    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn) return nullptr;

    float flMaxFOV = CONFIG_GET(float, g_Variables.m_AimBot.m_flFOV);
    bool  bIgnoreTeam = CONFIG_GET(bool, g_Variables.m_AimBot.m_bIgnoreTeammates);
    int   iAimMode = CONFIG_GET(int, g_Variables.m_AimBot.m_iAimMode);
    bool  bVisCheck = (iAimMode == 1) && CONFIG_GET(bool, g_Variables.m_AimBot.m_bVisibilityCheck);

    // Screen center = crosshair position
    float flScreenCX = Window::m_iWidth  * 0.5f;
    float flScreenCY = Window::m_iHeight * 0.5f;

    // Convert FOV degrees to screen pixel radius
    float flFOVRadius = std::tan(M_DEG2RAD(flMaxFOV) * 0.5f) * static_cast<float>(Window::m_iHeight);
    if (flFOVRadius <= 0.f) return nullptr;

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_eType != EEntityType::ENTITY_PLAYER) continue;

        CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
        if (!pCtrl || pCtrl->m_bIsLocalPlayerController()) continue;

        C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pCtrl->m_hPawn().Get());
        if (!pPawn || !pPawn->IsAlive()) continue;

        if (bIgnoreTeam && pPawn->m_iTeamNum() == pLocalPawn->m_iTeamNum()) continue;

        // V2.0: Visibility Check — skip enemies behind walls
        if (bVisCheck)
        {
            Vector vecEye = pLocalPawn->GetEyePosition();
            CGameSceneNode* pNodeVis = pPawn->m_pGameSceneNode();
            if (pNodeVis)
            {
                Vector vecTargetVis = pNodeVis->m_vecAbsOrigin();
                vecTargetVis.z += 40.f; // chest height
                if (!g_Utilities.IsVisible(pPawn, vecEye, vecTargetVis))
                    continue; // Devor ortida — o'tkazib yubor
            }
        }

        Vector vecTarget = GetHitboxPosition(pPawn, CONFIG_GET(int, g_Variables.m_AimBot.m_iHitbox));
        if (vecTarget.IsZero()) continue;

        // Project hitbox to screen
        ImVec2 screenPos;
        if (!Draw::WorldToScreen(vecTarget, screenPos)) continue;

        float flDX = screenPos.x - flScreenCX;
        float flDY = screenPos.y - flScreenCY;
        float flDist = std::sqrtf(flDX * flDX + flDY * flDY);

        // Only targets within FOV circle
        if (flDist > flFOVRadius) continue;

        if (flDist < flBestFOV)
        {
            flBestFOV = flDist;
            pBest = pPawn;
        }
    }
    return pBest;
}

// -----------------------------------------------------------------------
void Aimbot::DrawFOVCircle()
{
    if (!CONFIG_GET(bool, g_Variables.m_AimBot.m_bDrawFOV)) return;

    float flFOV = CONFIG_GET(float, g_Variables.m_AimBot.m_flFOV);
    float flW = static_cast<float>(Window::m_iWidth);
    float flH = static_cast<float>(Window::m_iHeight);
    ImVec2 center(flW * 0.5f, flH * 0.5f);

    float flRadius = std::tan(M_DEG2RAD(flFOV) * 0.5f) * flH;
    if (flRadius <= 0.f || !std::isfinite(flRadius)) return;

    Draw::AddCircle(center, flRadius, Color(0, 255, 80, 180), 64, DRAW_CIRCLE_NONE);
}

// -----------------------------------------------------------------------
// V2.0: Random helpers for anti-ban
// -----------------------------------------------------------------------
static std::mt19937& GetAimRNG()
{
    static std::mt19937 rng(static_cast<unsigned>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count() ^ 0xDEADBEEF));
    return rng;
}

static float AimRandomFloat(float flMin, float flMax)
{
    std::uniform_real_distribution<float> dist(flMin, flMax);
    return dist(GetAimRNG());
}

// -----------------------------------------------------------------------
// V2.0: Anti-ban state tracking
// -----------------------------------------------------------------------
static C_CSPlayerPawn* s_pLastTarget      = nullptr;
static CTimer          s_timerReaction(false);
static CTimer          s_timerAimDuration(false);
static CTimer          s_timerKillDelay(false);
static float           s_flCurrentReactionDelay = 0.f;
static bool            s_bWaitingReaction  = false;
static bool            s_bWaitingKillDelay = false;

// -----------------------------------------------------------------------
// Debug counter to limit console spam (print every ~200 calls = ~1 sec)
static int s_iDebugCounter = 0;

void Aimbot::Run(const std::vector<EntityObject_t>& vecEntities)
{
    int iAimKey = CONFIG_GET(int, g_Variables.m_AimBot.m_iAimKey);

    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn)
    {
        if (s_iDebugCounter++ % 200 == 0)
            std::cout << X("  [AIM] Local pawn = null") << std::endl;
        return;
    }

    // --- Standalone RCS ---
    static QAngle angOldPunch = {0, 0, 0};
    QAngle angPunch = {0, 0, 0};

    bool bRcsEnabled = CONFIG_GET(bool, g_Variables.m_RCS.m_bEnable);
    float flRcsScaleX = CONFIG_GET(float, g_Variables.m_RCS.m_flScaleX);
    float flRcsScaleY = CONFIG_GET(float, g_Variables.m_RCS.m_flScaleY);

    if (bRcsEnabled)
    {
        CUtlVectorSimple cache = pLocalPawn->m_aimPunchCache();
        if (cache.m_nSize > 0 && cache.m_nSize < 200 && cache.m_pData > 0x1000)
        {
            angPunch = g_Memory.ReadMemory<QAngle>(cache.m_pData + (cache.m_nSize - 1) * sizeof(QAngle));
            
            // Limit to valid angles
            if (std::isnan(angPunch.x) || std::isnan(angPunch.y)) angPunch = {0, 0, 0};
        }

        if (pLocalPawn->m_iShotsFired() > 1)
        {
            QAngle angDelta = angPunch - angOldPunch;
            
            // Adjust based on a hardcoded sensitivity baseline for smoothing
            // Better to allow user to tweak X/Y scale to their sensitivity
            float pixels_x = -(angDelta.y * 2.0f * flRcsScaleX);
            float pixels_y = (angDelta.x * 2.0f * flRcsScaleY);

            // Execute mouse move for RCS alone (if aimbot is not heavily doing it)
            if ((std::fabs(pixels_x) > 0.1f || std::fabs(pixels_y) > 0.1f) && !(GetAsyncKeyState(iAimKey) & 0x8000))
            {
                mouse_event(MOUSEEVENTF_MOVE, static_cast<LONG>(pixels_x), static_cast<LONG>(pixels_y), 0, 0);
            }
        }
        else
        {
            angPunch = {0, 0, 0};
        }
        angOldPunch = angPunch;
    }
    // --- End Standalone RCS ---

    if (!(GetAsyncKeyState(iAimKey) & 0x8000)) return;

    // V2.0: Aimbot rejimini aniqlash
    int iAimMode = CONFIG_GET(int, g_Variables.m_AimBot.m_iAimMode);
    // iAimMode: 0 = Klassik (eski, tez, anti-ban yo'q)
    //           1 = Xavfsiz (V2.0 — reaction, jitter, kill delay, vis check)

    // V2.0: Kill Delay — faqat Xavfsiz rejimda
    if (iAimMode == 1 && s_bWaitingKillDelay)
    {
        float flKillDelay = CONFIG_GET(float, g_Variables.m_AimBot.m_flKillDelay);
        if (s_timerKillDelay.Elapsed() < static_cast<long long>(flKillDelay))
            return;
        s_bWaitingKillDelay = false;
    }

    float flBestDist;
    C_CSPlayerPawn* pTarget = FindBestTarget(vecEntities, flBestDist);
    if (!pTarget)
    {
        s_pLastTarget = nullptr;
        s_bWaitingReaction = false;
        if (s_iDebugCounter++ % 200 == 0)
            std::cout << X("  [AIM] Target topilmadi (entities=") << vecEntities.size()
                      << X(", FOV=") << CONFIG_GET(float, g_Variables.m_AimBot.m_flFOV) << X(")") << std::endl;
        return;
    }

    // V2.0: Reaction Time + Kill Delay — faqat Xavfsiz rejimda
    if (iAimMode == 1)
    {
        if (pTarget != s_pLastTarget)
        {
            // Eski target o'lgan bo'lishi mumkin — kill delay
            if (s_pLastTarget != nullptr)
            {
                bool bOldTargetFound = false;
                for (const EntityObject_t& obj : vecEntities)
                {
                    if (obj.m_eType != EEntityType::ENTITY_PLAYER) continue;
                    CCSPlayerController* pCtrl = reinterpret_cast<CCSPlayerController*>(obj.m_pEntity);
                    if (!pCtrl) continue;
                    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pCtrl->m_hPawn().Get());
                    if (pPawn == s_pLastTarget && pPawn && pPawn->IsAlive())
                    { bOldTargetFound = true; break; }
                }
                if (!bOldTargetFound)
                {
                    s_bWaitingKillDelay = true;
                    s_timerKillDelay.Reset();
                    s_pLastTarget = nullptr;
                    s_bWaitingReaction = false;
                    return;
                }
            }

            s_pLastTarget = pTarget;
            s_bWaitingReaction = true;
            s_timerReaction.Reset();
            s_timerAimDuration.Reset();
            float flMinReact = CONFIG_GET(float, g_Variables.m_AimBot.m_flReactionTimeMin);
            float flMaxReact = CONFIG_GET(float, g_Variables.m_AimBot.m_flReactionTimeMax);
            s_flCurrentReactionDelay = AimRandomFloat(flMinReact, flMaxReact);
        }

        // Reaction Time kutish
        if (s_bWaitingReaction)
        {
            if (s_timerReaction.Elapsed() < static_cast<long long>(s_flCurrentReactionDelay))
                return;
            s_bWaitingReaction = false;
        }

        // Max Aim Time
        float flMaxAimTime = CONFIG_GET(float, g_Variables.m_AimBot.m_flMaxAimTime);
        if (flMaxAimTime > 0.f && s_timerAimDuration.Elapsed() > static_cast<long long>(flMaxAimTime))
            return;
    }

    Vector vecTarget = GetHitboxPosition(pTarget, CONFIG_GET(int, g_Variables.m_AimBot.m_iHitbox));
    if (vecTarget.IsZero()) return;

    // Project target hitbox to screen
    ImVec2 screenTarget;
    if (!Draw::WorldToScreen(vecTarget, screenTarget)) return;

    float flScreenCX = Window::m_iWidth  * 0.5f;
    float flScreenCY = Window::m_iHeight * 0.5f;

    float flDeltaX = screenTarget.x - flScreenCX;
    float flDeltaY = screenTarget.y - flScreenCY;

    if (!std::isfinite(flDeltaX) || !std::isfinite(flDeltaY)) return;

    // V2.0: Aim Jitter — faqat Xavfsiz rejimda
    if (iAimMode == 1)
    {
        float flJitter = CONFIG_GET(float, g_Variables.m_AimBot.m_flAimJitter);
        if (flJitter > 0.f)
        {
            flDeltaX += AimRandomFloat(-flJitter, flJitter);
            flDeltaY += AimRandomFloat(-flJitter, flJitter);
        }
    }

    // Apply smoothing (divide pixel delta by smooth factor)
    float flSmooth = CONFIG_GET(float, g_Variables.m_AimBot.m_flSmooth);
    if (flSmooth < 1.f) flSmooth = 1.f;
    flDeltaX /= flSmooth;
    flDeltaY /= flSmooth;

    // Clamp to prevent crazy jumps
    flDeltaX = std::clamp(flDeltaX, -150.f, 150.f);
    flDeltaY = std::clamp(flDeltaY, -150.f, 150.f);

    // Skip tiny movements (already on target)
    if (std::fabsf(flDeltaX) < 0.5f && std::fabsf(flDeltaY) < 0.5f) return;

    if (s_iDebugCounter++ % 200 == 0)
        std::cout << X("  [AIM] MOVING dx=") << flDeltaX << X(" dy=") << flDeltaY << std::endl;

    // Use mouse_event — works with CS2 Raw Input (unlike SendInput)
    mouse_event(MOUSEEVENTF_MOVE, static_cast<LONG>(flDeltaX), static_cast<LONG>(flDeltaY), 0, 0);
}
