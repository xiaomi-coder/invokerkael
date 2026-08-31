#include "Includes.h"
#include "features/skins/Skins.h"
#include "features/tracer/Tracer.h"
#include "features/loot/LootESP.h"
#include "features/thirdperson/ThirdPerson.h"

FILE* m_pConsoleStream = nullptr;
std::ofstream m_ofsFile{};

bool ConsoleAttach(const char* szConsoleTitle)
{
    if (!AllocConsole())
        return false;

    AttachConsole(ATTACH_PARENT_PROCESS);

    FILE* fp;
    freopen_s(&fp, X("CONIN$"),  X("r"), stdin);
    freopen_s(&fp, X("CONOUT$"), X("w"), stdout);
    freopen_s(&fp, X("CONOUT$"), X("w"), stderr);

    if (!SetConsoleTitleA(szConsoleTitle))
        return false;

    return true;
}

bool DetachConsole()
{
    FILE* fp;
    freopen_s(&fp, X("NUL"), X("r"), stdin);
    freopen_s(&fp, X("NUL"), X("w"), stdout);
    freopen_s(&fp, X("NUL"), X("w"), stderr);
    return FreeConsole() != 0;
}

void SetThreadPriorityWrapper()
{
    HANDLE hThread = GetCurrentThread();
    if (hThread)
    {
        int nPri = GetThreadPriority(hThread);
        if (nPri == THREAD_PRIORITY_ERROR_RETURN)
            throw std::runtime_error(X("failed to get thread priority"));
        if (nPri != THREAD_PRIORITY_HIGHEST)
            SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
        // GetCurrentThread() returns a pseudo-handle — do NOT close it
    }
    else
        throw std::runtime_error(X("failed to set thread priority"));
}

// -----------------------------------------------------------------------
//  C4 TIMER + DAMAGE
//  Portlagan bombani entity list orqali topamiz (dwPlantedC4 offseti eskirsa
//  ham ishlaydi), ekranga HUD panel + dunyoda 3D belgi chizamiz.
// -----------------------------------------------------------------------
static void RenderBombTimer(const std::vector<EntityObject_t>& vecEntities)
{
    if (!CONFIG_GET(bool, g_Variables.m_Misc.m_bC4Timer))
        return;

    // ---- schema offsetlari (bir marta) ----
    static std::uint32_t s_uSceneNode = 0U, s_uOrigin = 0U, s_uBlow = 0U,
                         s_uDefused   = 0U, s_uSite   = 0U, s_uTicking = 0U;
    static bool s_bResolved = false;
    if (!s_bResolved)
    {
        s_bResolved = true;
        auto Get = [](const char* szField) -> std::uint32_t
        {
            auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash(szField));
            return (it != SchemaSystem::m_mapSchemaOffsets.end()) ? it->second : 0U;
        };
        s_uSceneNode = Get("C_BaseEntity->m_pGameSceneNode");
        s_uOrigin    = Get("CGameSceneNode->m_vecAbsOrigin");
        s_uBlow      = Get("C_PlantedC4->m_flC4Blow");
        s_uDefused   = Get("C_PlantedC4->m_bBombDefused");
        s_uSite      = Get("C_PlantedC4->m_nBombSite");
        s_uTicking   = Get("C_PlantedC4->m_bBombTicking");
    }

    if (s_uBlow == 0U)
        return;

    // ---- bombani topish: avval entity list, keyin global pointer ----
    std::uintptr_t uBomb = 0U;
    for (const EntityObject_t& object : vecEntities)
    {
        if (object.m_eType == EEntityType::ENTITY_PLANTEDC4 && object.m_pEntity)
        {
            uBomb = reinterpret_cast<std::uintptr_t>(object.m_pEntity);
            break;
        }
    }

    if (uBomb == 0U && g_Globals.m_Offsets.m_uPlantedC4 != 0U)
    {
        std::uintptr_t uList = g_Memory.ReadMemory<std::uintptr_t>(g_Globals.m_Offsets.m_uPlantedC4);
        if (uList > 0x10000)
        {
            std::uintptr_t uEntity = g_Memory.ReadMemory<std::uintptr_t>(uList);
            if (uEntity > 0x10000)
                uBomb = uEntity;
        }
    }

    if (uBomb == 0U)
        return;

    if (s_uDefused != 0U && g_Memory.ReadMemory<bool>(uBomb + s_uDefused))
        return;
    if (s_uTicking != 0U && !g_Memory.ReadMemory<bool>(uBomb + s_uTicking))
        return;

    const float flBlow = g_Memory.ReadMemory<float>(uBomb + s_uBlow);
    const float flNow  = g_Interfaces.m_GlobalVars.m_flCurrentTime;
    if (!std::isfinite(flBlow) || flNow <= 0.f)
        return;

    const float flRemain = flBlow - flNow;
    if (flRemain <= 0.f || flRemain > 45.f)   // C4 = 40 soniya
        return;

    const int   nSite  = (s_uSite != 0U) ? g_Memory.ReadMemory<int>(uBomb + s_uSite) : -1;
    const char* szSite = (nSite == 0) ? "A" : (nSite == 1) ? "B" : "?";

    // ---- bomba pozitsiyasi ----
    Vector vecBomb{ 0.f, 0.f, 0.f };
    bool   bHasPos = false;
    if (s_uSceneNode != 0U && s_uOrigin != 0U)
    {
        std::uintptr_t uNode = g_Memory.ReadMemory<std::uintptr_t>(uBomb + s_uSceneNode);
        if (uNode > 0x10000)
        {
            vecBomb = g_Memory.ReadMemory<Vector>(uNode + s_uOrigin);
            bHasPos = std::isfinite(vecBomb.x) && std::isfinite(vecBomb.y) && std::isfinite(vecBomb.z);
        }
    }

    // ---- o'zimizga yetadigan zarar ----
    int  iDamage = -1;
    bool bFatal  = false;
    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (bHasPos && pLocalPawn && pLocalPawn->IsAlive())
    {
        const Vector vecLocal = pLocalPawn->m_pGameSceneNode()->m_vecAbsOrigin();
        const float  flDist   = vecBomb.DistTo(vecLocal);

        // CS2: markazda 500 zarar, 500 * 3.5 unitda 0 gacha chiziqli kamayadi
        float flDamage = 500.f * (1.f - ImClamp(flDist / (500.f * 3.5f), 0.f, 1.f));
        if (pLocalPawn->m_ArmorValue() > 0)
            flDamage *= 0.5f;                  // kevlar taxminan yarmini yutadi

        iDamage = static_cast<int>(flDamage);
        bFatal  = (iDamage >= pLocalPawn->m_iHealth());
    }

    const Color colTime = (flRemain > 10.f) ? Color(255, 210, 60, 255)
                        : (flRemain > 5.f)  ? Color(255, 140, 30, 255)
                                            : Color(255, 45, 45, 255);

    // ================= HUD PANEL (doim ko'rinadi) =================
    {
        const float flW = 260.f;
        const float flH = (iDamage > 0) ? 78.f : 58.f;
        const float flX = Window::m_iWidth * 0.5f - flW * 0.5f;
        const float flY = 58.f;

        Draw::AddRect(ImVec2(flX, flY), ImVec2(flX + flW, flY + flH),
            Color(8, 11, 17, 230), DRAW_RECT_FILLED | DRAW_RECT_OUTLINE, colTime, 4.f, 1.f);

        // chap chekka aksenti
        Draw::AddRect(ImVec2(flX, flY), ImVec2(flX + 3.f, flY + flH), colTime, DRAW_RECT_FILLED);

        char szTitle[48];
        snprintf(szTitle, sizeof(szTitle), "C4  //  %s SITE", szSite);
        Draw::AddText(Fonts::Default, 14.f, ImVec2(flX + 12.f, flY + 8.f), std::string(szTitle),
            Color(150, 170, 190, 255), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));

        char szTime[24];
        snprintf(szTime, sizeof(szTime), "%.1f", flRemain);
        ImVec2 vecTimeSize = Fonts::Default->CalcTextSizeA(26.f, FLT_MAX, 0.f, szTime);
        Draw::AddText(Fonts::Default, 26.f, ImVec2(flX + flW - vecTimeSize.x - 12.f, flY + 6.f),
            std::string(szTime), colTime, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));

        // vaqt chizig'i (40 soniya)
        const float flBarY = flY + 34.f;
        Draw::AddRect(ImVec2(flX + 12.f, flBarY), ImVec2(flX + flW - 12.f, flBarY + 5.f),
            Color(20, 26, 38, 255), DRAW_RECT_FILLED, Color(0, 0, 0, 0), 2.f);
        const float flFill = (flW - 24.f) * ImClamp(flRemain / 40.f, 0.f, 1.f);
        if (flFill > 0.f)
            Draw::AddRect(ImVec2(flX + 12.f, flBarY), ImVec2(flX + 12.f + flFill, flBarY + 5.f),
                colTime, DRAW_RECT_FILLED, Color(0, 0, 0, 0), 2.f);

        // defuse imkoniyati
        const char* szDefuse = (flRemain >= 10.f) ? "DEFUSE: kit'siz ham ulguradi"
                             : (flRemain >= 5.f)  ? "DEFUSE: faqat KIT bilan"
                                                  : "DEFUSE: kech!";
        Draw::AddText(Fonts::Default, 13.f, ImVec2(flX + 12.f, flBarY + 10.f), std::string(szDefuse),
            (flRemain >= 5.f) ? Color(140, 160, 180, 255) : Color(255, 80, 80, 255),
            DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));

        // zarar
        if (iDamage > 0)
        {
            char szDamage[64];
            if (bFatal) snprintf(szDamage, sizeof(szDamage), "SIZGA: O'LIM (-%d HP)", iDamage);
            else        snprintf(szDamage, sizeof(szDamage), "SIZGA: -%d HP", iDamage);

            Draw::AddText(Fonts::Default, 14.f, ImVec2(flX + 12.f, flBarY + 26.f), std::string(szDamage),
                bFatal ? Color(255, 45, 45, 255) : Color(255, 190, 60, 255),
                DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));
        }
    }

    // ================= DUNYODAGI BELGI =================
    if (bHasPos)
    {
        ImVec2 vecScreen;
        if (Draw::WorldToScreen(vecBomb, vecScreen))
        {
            const ImVec2 vecMin(vecScreen.x - 18.f, vecScreen.y - 14.f);
            const ImVec2 vecMax(vecScreen.x + 18.f, vecScreen.y + 5.f);

            Draw::AddRect(ImVec2(vecMin.x - 1.f, vecMin.y - 1.f), ImVec2(vecMax.x + 1.f, vecMax.y + 1.f),
                Color(0, 0, 0, 180), DRAW_RECT_NONE);
            Draw::AddRect(vecMin, vecMax, colTime, DRAW_RECT_NONE);

            char szWorld[48];
            snprintf(szWorld, sizeof(szWorld), "C4 [%s] %.1f", szSite, flRemain);
            ImVec2 vecSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, szWorld);
            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                ImVec2(vecScreen.x - vecSize.x * 0.5f, vecMin.y - Fonts::ESP->FontSize - 3.f),
                std::string(szWorld), colTime, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));
        }
    }
}

// -----------------------------------------------------------------------
void EntityThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        if (!g_Utilities.IsInGame())
        {
            g_Utilities.Sleep(3000.f);
            continue;
        }
        EntityList::m_mtxEntities.lock();
        EntityList::UpdateEntities();
        EntityList::m_mtxEntities.unlock();
        g_Utilities.Sleep(INTERVAL_PER_TICK * 1000.0f);
    }
}

// -----------------------------------------------------------------------
void RenderThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        Draw::ClearDrawData();

        if (!g_Memory.IsWindowInForeground(X("Counter-Strike 2"), g_Globals.m_Instance))
        {
            Draw::SwapDrawData();
            g_Utilities.Sleep(1000.0f);
            continue;
        }

        std::unique_lock lockEntityGuard(EntityList::m_mtxEntities);
        std::vector<EntityObject_t> vecEntities;
        vecEntities.assign(EntityList::m_vecEntities.begin(), EntityList::m_vecEntities.end());
        lockEntityGuard.unlock();

        if (Window::m_bInitialized)
        {
            // ===== ESP =====
            bool bDrawESP = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals);
            bool bDrawGlowInfo = CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow) && CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowInfo);

            if (bDrawESP || bDrawGlowInfo)
            {
                for (EntityObject_t& object : vecEntities)
                {
                    if (object.m_eType != EEntityType::ENTITY_PLAYER) continue;

                    CCSPlayerController* pController = reinterpret_cast<CCSPlayerController*>(object.m_pEntity);
                    if (!pController || pController->m_bIsLocalPlayerController() || !pController->m_bPawnIsAlive()) continue;

                    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pController->m_hPlayerPawn().Get());
                    if (!pPawn) continue;

                    if (bDrawESP)
                        ESP::RenderPlayer(pController, pPawn);

                    if (bDrawGlowInfo)
                        ESP::RenderGlowInfo(pController, pPawn);
                }
            }

            // ===== AIMBOT FOV CIRCLE (drawn here in RenderThread, not TickThread) =====
            if (CONFIG_GET(bool, g_Variables.m_AimBot.m_bEnableAimbot))
                Aimbot::DrawFOVCircle();

            // ===== RADAR =====
            if (CONFIG_GET(bool, g_Variables.m_Radar.m_bEnableRadar))
                Radar::Render(vecEntities);

            // ===== SPECTATOR LIST =====
            if (CONFIG_GET(bool, g_Variables.m_SpectatorList.m_bEnableSpectatorList))
                SpectatorList::Render(vecEntities);

            // ===== GRENADE WARNING =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bGrenadeWarning))
                ESP::RenderGrenades(vecEntities);

            // ===== DROPPED WEAPONS ESP =====
            // Eski ESP::RenderWeapons ba'zi qurol klasslarini tanimasdi
            // (C_M4A1, C_WeaponAWP...), shuning uchun mustaqil modul.
            LootESP::Render();

            // ===== C4 TIMER =====
            ESP::RenderC4Timer(vecEntities);

            // ===== 3D DAMAGE INDICATORS =====
            ESP::RenderDamageIndicators();

            // ===== SNIPER CROSSHAIR =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bSniperCrosshair))
            {
                C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
                if (pLocalPawn && pLocalPawn->IsAlive() && !pLocalPawn->m_bIsScoped())
                {
                    std::string weaponName = pLocalPawn->m_strActiveWeaponName();
                    std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), ::tolower);
                    if (weaponName.find("awp") != std::string::npos || 
                        weaponName.find("ssg08") != std::string::npos || 
                        weaponName.find("scar20") != std::string::npos || 
                        weaponName.find("g3sg1") != std::string::npos)
                    {
                        ImVec2 center(Window::m_iWidth * 0.5f, Window::m_iHeight * 0.5f);
                        Draw::AddCircle(center, 2.5f, Color(255, 30, 30, 255), 12, DRAW_CIRCLE_FILLED | DRAW_CIRCLE_OUTLINE, Color(0, 0, 0, 200), 1.5f);
                    }
                }
            }

            // ===== WATERMARK =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bWatermark))
            {
                char szWatermark[64];
                float flWmFrameTime = g_Interfaces.m_GlobalVars.m_flFrameTime;
                float flWmFps = (flWmFrameTime > 0.0001f && flWmFrameTime < 1.f) ? (1.f / flWmFrameTime) : ImGui::GetIO().Framerate;
                snprintf(szWatermark, sizeof(szWatermark), "KaeL CS2 v%s | FPS: %03d", SHIFTHUB_VERSION, static_cast<int>(flWmFps));
                
                ImVec2 textSize = Fonts::Default->CalcTextSizeA(Fonts::Default->FontSize, FLT_MAX, 0.0f, szWatermark);
                ImVec2 padding(8.f, 4.f);
                ImVec2 boxMin(15.f, 15.f);
                ImVec2 boxMax(15.f + textSize.x + padding.x * 2.f, 15.f + textSize.y + padding.y * 2.f);

                // Background and outline
                Draw::AddRect(boxMin, boxMax, Color(8, 11, 17, 210), DRAW_RECT_FILLED | DRAW_RECT_OUTLINE, Color(34, 226, 255, 200), 4.f);
                // Text
                Draw::AddText(Fonts::Default, Fonts::Default->FontSize, ImVec2(boxMin.x + padding.x, boxMin.y + padding.y), std::string(szWatermark), Color(208, 224, 240, 255), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 255));
            }

            // ===== O'Q IZI (TRACER) =====
            Tracer::Render();

            // ===== C4 TIMER + DAMAGE =====
            RenderBombTimer(vecEntities);
        }

        Draw::SwapDrawData();
    }
}

// -----------------------------------------------------------------------
void MapParserThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        if (!g_Utilities.IsInGame())
        {
            g_Utilities.Sleep(3000.f);
            continue;
        }
        std::string strMapName = g_Memory.ReadMemoryString(g_Interfaces.m_GlobalVars.m_uMapNameShort);
        if (strMapName.empty())
        {
            g_Utilities.Sleep(3000.f);
            continue;
        }
        g_MapParser.VerifyMapNameHash(strMapName);
        g_Utilities.Sleep(3000.f);
    }
}

// -----------------------------------------------------------------------
void AimbotThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        if (g_Utilities.IsInGame() && g_License.HasFeature(ETier::PRO) &&
            CONFIG_GET(bool, g_Variables.m_AimBot.m_bEnableAimbot) && !Gui::m_bOpen &&
            g_Memory.IsWindowInForeground(X("Counter-Strike 2"), g_Globals.m_Instance))
        {
            std::unique_lock lockEntityGuard(EntityList::m_mtxEntities);
            std::vector<EntityObject_t> vecEntities;
            vecEntities.assign(EntityList::m_vecEntities.begin(), EntityList::m_vecEntities.end());
            lockEntityGuard.unlock();

            try { Aimbot::Run(vecEntities); } catch (...) { }
        }
        g_Utilities.Sleep(5.f); // 5ms = 200Hz — fast enough for smooth aim
    }
}

// -----------------------------------------------------------------------
void BhopThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        if (g_Utilities.IsInGame() && g_License.HasFeature(ETier::MID) && !Gui::m_bOpen)
        {
            try { Bhop::Run(); } catch (...) { }
        }
        g_Utilities.Sleep(0.5f); // 0.5ms = 2000Hz — maximum polling speed
    }
}

// -----------------------------------------------------------------------
void TickThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        if (!g_Utilities.IsInGame())
        {
            g_Utilities.Sleep(3000.f);
            continue;
        }

        // Snapshot entities
        std::unique_lock lockEntityGuard(EntityList::m_mtxEntities);
        std::vector<EntityObject_t> vecEntities;
        vecEntities.assign(EntityList::m_vecEntities.begin(), EntityList::m_vecEntities.end());
        lockEntityGuard.unlock();

        // ===== WORLD / NIGHT MODE / FOV =====
        // Menyu ochiq bo'lsa ham qo'llanadi — slayderni surganda o'zgarish darhol ko'rinsin
        try { World::Run(vecEntities); } catch (...) { }

        // ===== UCHINCHI SHAXS =====
        // Menyu ochiq bo'lsa ham ishlaydi
        try { ThirdPerson::Run(); } catch (...) { }

        // ===== SKIN CHANGER =====
        // Menyu ochiq bo'lsa ham qo'llanadi — skin tanlanganda darhol ko'rinsin
        try { Skins::Run(); } catch (...) { }

        if (!g_Memory.IsWindowInForeground(X("Counter-Strike 2"), g_Globals.m_Instance) || Gui::m_bOpen)
        {
            g_Utilities.Sleep(50.0f);
            continue;
        }

        try
        {
            // ===== ESP HOTKEY TOGGLE =====
            {
                static bool bESPKeyWasDown = false;
                bool bESPKeyDown = (GetAsyncKeyState(CONFIG_GET(int, g_Variables.m_Hotkeys.m_iESPToggleKey)) & 0x8000) != 0;
                if (bESPKeyDown && !bESPKeyWasDown)
                    CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals) = !CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals);
                bESPKeyWasDown = bESPKeyDown;
            }

            // ===== ANTI-FLASH =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bAntiFlash))
            {
                C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
                if (pLocalPawn)
                {
                    float flAlpha = pLocalPawn->m_flFlashMaxAlpha();
                    if (flAlpha > 0.f)
                    {
                        static std::uint32_t uOffset = SchemaSystem::m_mapSchemaOffsets[FNV1A::HashConst("C_CSPlayerPawnBase->m_flFlashMaxAlpha")];
                        g_Memory.WriteMemory<float>(reinterpret_cast<std::uintptr_t>(pLocalPawn) + uOffset, 0.f);
                    }
                }
            }

            // ===== O'Q IZI (TRACER) =====
            Tracer::Run();

            // ===== TRIGGERBOT =====
            if (g_License.HasFeature(ETier::MID) && CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bEnableTriggerbot))
                Triggerbot::Run(vecEntities);

            // ===== HIT SOUND + KILL SOUND (V2.0) =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bHitSound) || CONFIG_GET(bool, g_Variables.m_Misc.m_bKillSound))
            {
                static std::map<std::uintptr_t, int> s_mapEnemyHP;
                static bool s_bSoundPathsChecked = false;
                static std::string s_strHitSoundPath;
                static std::string s_strKillSoundPath;
                static std::string s_strDefaultSound = "C:\\Windows\\Media\\Windows Default.wav";

                // V2.0: One-time check for custom sound files next to exe
                if (!s_bSoundPathsChecked)
                {
                    s_bSoundPathsChecked = true;
                    char szExePath[MAX_PATH];
                    GetModuleFileNameA(NULL, szExePath, MAX_PATH);
                    std::string strExeDir(szExePath);
                    strExeDir = strExeDir.substr(0, strExeDir.find_last_of("\\/") + 1);

                    std::string strHitFile  = strExeDir + "hit_sound.wav";
                    std::string strKillFile = strExeDir + "kill_sound.wav";

                    if (std::filesystem::exists(strHitFile))
                        s_strHitSoundPath = strHitFile;
                    else
                        s_strHitSoundPath = s_strDefaultSound;

                    if (std::filesystem::exists(strKillFile))
                        s_strKillSoundPath = strKillFile;
                    else
                        s_strKillSoundPath = s_strDefaultSound;
                }

                // V2.0: Set volume based on config (0-100%)
                float flVol = CONFIG_GET(float, g_Variables.m_Misc.m_flSoundVolume);
                WORD wVolume = static_cast<WORD>((flVol / 100.0f) * 0xFFFF);
                DWORD dwVolume = MAKELONG(wVolume, wVolume);
                waveOutSetVolume(NULL, dwVolume);

                for (EntityObject_t& object : vecEntities)
                {
                    if (object.m_eType != EEntityType::ENTITY_PLAYER) continue;

                    CCSPlayerController* pController = reinterpret_cast<CCSPlayerController*>(object.m_pEntity);
                    if (!pController || pController->m_bIsLocalPlayerController()) continue;

                    // Fix: Use m_hPlayerPawn() instead of m_hPawn()
                    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pController->m_hPlayerPawn().Get());
                    // Fix: Use pController->m_bPawnIsAlive() instead of pPawn->IsAlive()
                    if (!pPawn || !pController->m_bPawnIsAlive())
                    {
                        if (pPawn)
                            s_mapEnemyHP.erase(reinterpret_cast<std::uintptr_t>(pPawn));
                        continue;
                    }

                    if (pPawn->m_iTeamNum() == g_Globals.m_LocalPlayer.m_pPlayerPawn->m_iTeamNum())
                        continue;

                    int iHealth = pPawn->m_iHealth();
                    std::uintptr_t uPawnAddress = reinterpret_cast<std::uintptr_t>(pPawn);

                    auto it = s_mapEnemyHP.find(uPawnAddress);
                    if (it != s_mapEnemyHP.end())
                    {
                        if (it->second > iHealth && iHealth >= 0)
                        {
                            // Calculate exact damage done
                            int damage = it->second - (iHealth < 0 ? 0 : iHealth);
                            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bDamageIndicator) && damage > 0 && damage <= 100)
                            {
                                ESP::AddDamageIndicator(pPawn->GetEyePosition(), damage, pPawn->m_iMaxHealth());
                            }

                            // Hit sound — o'q tegdi
                            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bHitSound))
                                PlaySoundA(s_strHitSoundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME);
                        }
                        else if (it->second > 0 && iHealth <= 0)
                        {
                            // Calculate fatal damage done
                            int damage = it->second;
                            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bDamageIndicator) && damage > 0 && damage <= 100)
                            {
                                ESP::AddDamageIndicator(pPawn->GetEyePosition(), damage, pPawn->m_iMaxHealth());
                            }

                            // V2.0: Kill sound — dushman o'ldi!
                            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bKillSound))
                                PlaySoundA(s_strKillSoundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME);
                            else if (CONFIG_GET(bool, g_Variables.m_Misc.m_bHitSound))
                                PlaySoundA(s_strHitSoundPath.c_str(), NULL, SND_ASYNC | SND_FILENAME);
                        }
                    }
                    s_mapEnemyHP[uPawnAddress] = iHealth;
                }
            }

            // ===== FORCE RADAR (CS2 built-in radar) =====
            // Always enabled as requested
            Radar::ForceRadarSpotted(vecEntities);

            // ===== AUDITORY SONAR =====
            Radar::AuditorySonar(vecEntities);

            // ===== PLAYER GLOW (CS2 native engine glow) =====
            if (CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow))
                PlayerGlow::Run(vecEntities);

        }
        catch (...) { }

        g_Utilities.Sleep(INTERVAL_PER_TICK * 1000.0f);
    }
}

// -----------------------------------------------------------------------
//  AUTO-ACCEPT
//  Eski usul o'yin oynasining DC'sidan GetPixel qilardi — DirectX oynasida
//  bu ishlamaydi. Endi ekranning o'zidan (desktop DC) surat olamiz va
//  yashil "ACCEPT" tugmasini qidiramiz.
// -----------------------------------------------------------------------
static bool FindAcceptButton(HWND hCS2, POINT& ptOut)
{
    RECT rc{};
    if (!GetWindowRect(hCS2, &rc))
        return false;

    const int iW = rc.right - rc.left;
    const int iH = rc.bottom - rc.top;
    if (iW < 800 || iH < 600)
        return false;

    // "Match found" paneli ekran markazida turadi — shu hududni skanerlaymiz
    const int iBandX = rc.left + static_cast<int>(iW * 0.25f);
    const int iBandY = rc.top  + static_cast<int>(iH * 0.32f);
    const int iBandW = static_cast<int>(iW * 0.50f);
    const int iBandH = static_cast<int>(iH * 0.52f);

    HDC hScreen = GetDC(NULL);
    if (!hScreen)
        return false;

    HDC     hMem = CreateCompatibleDC(hScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hScreen, iBandW, iBandH);
    HGDIOBJ hOld = SelectObject(hMem, hBmp);

    bool bFound = false;

    if (BitBlt(hMem, 0, 0, iBandW, iBandH, hScreen, iBandX, iBandY, SRCCOPY))
    {
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = iBandW;
        bmi.bmiHeader.biHeight      = -iBandH;   // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<std::uint8_t> vecPixels(static_cast<size_t>(iBandW) * iBandH * 4);
        if (GetDIBits(hMem, hBmp, 0, iBandH, vecPixels.data(), &bmi, DIB_RGB_COLORS))
        {
            long long llSumX = 0, llSumY = 0;
            int nHits = 0;
            int iMinX = iBandW, iMaxX = 0, iMinY = iBandH, iMaxY = 0;

            for (int y = 0; y < iBandH; y += 3)
            {
                const std::uint8_t* pRow = vecPixels.data() + static_cast<size_t>(y) * iBandW * 4;
                for (int x = 0; x < iBandW; x += 3)
                {
                    const int b = pRow[x * 4 + 0];
                    const int g = pRow[x * 4 + 1];
                    const int r = pRow[x * 4 + 2];

                    // CS2 "ACCEPT" tugmasi — to'yingan yashil
                    if (g > 110 && (g - r) > 35 && (g - b) > 45 && r < 170 && b < 170)
                    {
                        llSumX += x; llSumY += y; nHits++;
                        if (x < iMinX) iMinX = x;
                        if (x > iMaxX) iMaxX = x;
                        if (y < iMinY) iMinY = y;
                        if (y > iMaxY) iMaxY = y;
                    }
                }
            }

            // tugmaga o'xshash blok bo'lsin: keng, juda baland emas
            if (nHits > 350)
            {
                const int iBoxW = iMaxX - iMinX;
                const int iBoxH = iMaxY - iMinY;
                if (iBoxW > 90 && iBoxH > 15 && iBoxH < iBandH / 2)
                {
                    ptOut.x = iBandX + static_cast<int>(llSumX / nHits);
                    ptOut.y = iBandY + static_cast<int>(llSumY / nHits);
                    bFound  = true;
                }
            }
        }
    }

    SelectObject(hMem, hOld);
    DeleteObject(hBmp);
    DeleteDC(hMem);
    ReleaseDC(NULL, hScreen);
    return bFound;
}

void AutoAcceptThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        // O'yin ichida yoki menyu ochiq bo'lsa tekshirmaymiz — noto'g'ri bosishning oldini oladi
        if (CONFIG_GET(bool, g_Variables.m_Misc.m_bAutoAccept) && !Gui::m_bOpen && !g_Utilities.IsInGame())
        {
            HWND hCS2 = FindWindowA("SDL_app", "Counter-Strike 2");
            if (hCS2 && IsWindowVisible(hCS2) && !IsIconic(hCS2))
            {
                POINT ptAccept{};
                if (FindAcceptButton(hCS2, ptAccept))
                {
                    POINT ptOld{};
                    GetCursorPos(&ptOld);

                    if (GetForegroundWindow() != hCS2)
                    {
                        SetForegroundWindow(hCS2);
                        g_Utilities.Sleep(150.f);
                    }

                    SetCursorPos(ptAccept.x, ptAccept.y);
                    g_Utilities.Sleep(50.f);

                    INPUT inputs[2] = {};
                    inputs[0].type = INPUT_MOUSE;
                    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    inputs[1].type = INPUT_MOUSE;
                    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(2, inputs, sizeof(INPUT));

                    g_Utilities.Sleep(100.f);
                    SetCursorPos(ptOld.x, ptOld.y);

                    g_Utilities.Sleep(5000.f);   // spam qilmaslik uchun
                }
            }
        }
        g_Utilities.Sleep(400.f);
    }
}

// -----------------------------------------------------------------------
void UpdateThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        g_Globals.Update();
        g_Interfaces.Update();
        g_Utilities.Sleep(1);
    }
}

// -----------------------------------------------------------------------
void HeartbeatThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        g_License.SendHeartbeat();
        g_Utilities.Sleep(300000.f);  // 5 minutes
    }
}

// -----------------------------------------------------------------------
__forceinline void CreateThreads()
{
    std::thread(&EntityThread).detach();
    std::thread(&RenderThread).detach();
    std::thread(&MapParserThread).detach();
    std::thread(&TickThread).detach();
    std::thread(&AimbotThread).detach();
    std::thread(&BhopThread).detach();
    std::thread(&AutoAcceptThread).detach();
    std::thread(&UpdateThread).detach();
    std::thread(&HeartbeatThread).detach();
}

// -----------------------------------------------------------------------
bool MainLoop(LPVOID lpParameter)
{
    // Show GUI login window — handles login, CS2 detection, ALL inside the window
    if (LoginWindow::Create())
    {
        // Pre-load configurations so that GUI elements (like saved Menu Key) 
        // will display the correct user-saved values in the Login window!
        Config::Setup(X("default.json"));

        LoginWindow::Run();
        // At this point: login done, CS2 found (checked in loading step)
        // Now initialize everything BEFORE closing login window
        // (login window is still visible, user sees "Dasturni sozlash...")

        // Save any changes made inside LoginWindow immediately!
        Config::Save(X("default.json"));

        try
        {
            // CS2 is already running (checked in loading step 3)
            g_Memory.Initialize(X("cs2.exe"));

            // Wait for navsystem.dll
            int iNavAttempts = 0;
            while (g_Memory.GetModule(NAVSYSTEM_DLL).m_uBaseAddress == 0U)
            {
                g_Utilities.Sleep(500.0f);
                iNavAttempts++;
                if (iNavAttempts > 30) // 15 soniya
                    throw std::runtime_error("CS2 modullari (navsystem.dll) topilmadi! O'yin to'liq yonguncha kuting yoki dasturni Administrator nomidan ishlating.");
            }

            SchemaSystem::Setup();
        }
        catch (const std::exception& ex)
        {
            MessageBoxA(NULL, ex.what(), "KaeL CS2 - Xato", MB_OK | MB_ICONERROR);
            LoginWindow::Destroy();
            exit(EXIT_FAILURE);
        }

        // NOW destroy login window (everything is ready)
        LoginWindow::Destroy();
    }
    else
    {
        // Fallback to console login if GUI fails
        g_License.Load();

        try
        {
            g_Memory.Initialize(X("cs2.exe"));
            while (g_Memory.GetModule(NAVSYSTEM_DLL).m_uBaseAddress == 0U)
                g_Utilities.Sleep(500.0f);
            Config::Setup(X("default.json"));
            SchemaSystem::Setup();
        }
        catch (const std::exception& ex)
        {
            std::cout << "  [X] XATO: " << ex.what() << std::endl;
            Sleep(5000);
            exit(EXIT_FAILURE);
        }
    }

    // Hide console
    // DetachConsole();

    try
    {
        // Create overlay (Login window already destroyed, ImGui context fresh)
        if (!Window::m_bInitialized)
            Window::Create();

        // Skin bazasini fonda yuklashni boshlaymiz
        Skins::Initialize();

        // Load weapon icon PNGs (must be after Window::Create for DX11 device)
        WeaponIcons::Initialize();

        SetPriorityClass(g_Globals.m_Instance, HIGH_PRIORITY_CLASS);
        SetPriorityClass(g_Globals.m_hDll,     HIGH_PRIORITY_CLASS);
        SetPriorityClass(GetCurrentProcess(),   HIGH_PRIORITY_CLASS);

        CreateThreads();

        while (!g_Globals.m_bIsUnloading)
        {
            if (!Window::Render())
                return false;
        }
    }
    catch (const std::exception& ex)
    {
        std::cout << X("  [X] XATO: ") << ex.what() << std::endl;
        std::cout << X("  [*] 10 soniyadan keyin yopiladi...") << std::endl;
        Sleep(10000);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

// -----------------------------------------------------------------------
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR pArgs, int iCmdShow)
{
    // Fix CWD issue when launched from browsers or shortcuts
    char szPath[MAX_PATH];
    if (GetModuleFileNameA(NULL, szPath, MAX_PATH))
    {
        std::string strPath = szPath;
        size_t pos = strPath.find_last_of("\\/");
        if (pos != std::string::npos)
        {
            strPath = strPath.substr(0, pos);
            SetCurrentDirectoryA(strPath.c_str());
        }
    }

    // ConsoleAttach(X("External Base")); // Disabled to hide console from users
    g_Globals.m_hDll = hInstance;

    if (!MainLoop(hInstance))
    {
        WeaponIcons::Shutdown();
        // g_Memory is a global — its destructor runs automatically at exit
        if (Window::m_bInitialized)
            Window::Destroy();
    }
    return EXIT_SUCCESS;
}
