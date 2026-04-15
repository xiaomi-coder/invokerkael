#include "Includes.h"

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
                    if (!pController || pController->m_bIsLocalPlayerController()) continue;

                    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pController->m_hPawn().Get());
                    if (!pPawn || !pPawn->IsAlive()) continue;

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

            // ===== WEB RADAR SERVER SYNC =====
            if (CONFIG_GET(bool, g_Variables.m_Radar.m_bWebRadar))
                WebRadar::SetEntities(vecEntities);

            // ===== SPECTATOR LIST =====
            if (CONFIG_GET(bool, g_Variables.m_SpectatorList.m_bEnableSpectatorList))
                SpectatorList::Render(vecEntities);

            // ===== GRENADE WARNING =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bGrenadeWarning))
                ESP::RenderGrenades(vecEntities);

            // ===== DROPPED WEAPONS ESP =====
            ESP::RenderWeapons(vecEntities);

            // ===== SNIPER CROSSHAIR =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bSniperCrosshair))
            {
                C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
                if (pLocalPawn && pLocalPawn->IsAlive() && !pLocalPawn->m_bIsScoped())
                {
                    std::string weaponName = pLocalPawn->m_strActiveWeaponName();
                    if (weaponName == "awp" || weaponName == "ssg08" || weaponName == "scar20" || weaponName == "g3sg1")
                    {
                        ImVec2 center(Window::m_iWidth * 0.5f, Window::m_iHeight * 0.5f);
                        Draw::AddCircle(center, 2.5f, Color(255, 30, 30, 255), 12, DRAW_CIRCLE_FILLED | DRAW_CIRCLE_OUTLINE, Color(0, 0, 0, 200), 1.5f);
                    }
                }
            }



            // ===== C4 TIMER + ESP =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bC4Timer))
            {
                static float s_flRemain = -1.f;
                static int s_nSite = -1;
                static int s_iFrame = 0;
                static Vector s_vecBombPos = { 0.f, 0.f, 0.f };
                static bool s_bHasPos = false;

                if (s_iFrame++ % 15 == 0)
                {
                    s_flRemain = -1.f;
                    s_nSite = -1;
                    s_bHasPos = false;

                    std::uintptr_t uBase = g_Globals.m_Offsets.m_uPlantedC4;
                    if (uBase != 0)
                    {
                        std::uintptr_t pList = g_Memory.ReadMemory<std::uintptr_t>(uBase);
                        if (pList > 0x10000)
                        {
                            std::uintptr_t uEnt = g_Memory.ReadMemory<std::uintptr_t>(pList);
                            if (uEnt > 0x10000)
                            {
                                auto itBlow = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("C_PlantedC4->m_flC4Blow"));
                                if (itBlow != SchemaSystem::m_mapSchemaOffsets.end() && itBlow->second > 0)
                                {
                                    float flBlow = g_Memory.ReadMemory<float>(uEnt + itBlow->second);
                                    float flCur = g_Interfaces.m_GlobalVars.m_flCurrentTime;

                                    if (flBlow > 0.f && flCur > 0.f && std::isfinite(flBlow))
                                    {
                                        float flRem = flBlow - flCur;
                                        if (flRem >= 0.f && flRem <= 60.f)
                                        {
                                            s_flRemain = flRem;

                                            auto itSite = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("C_PlantedC4->m_nBombSite"));
                                            if (itSite != SchemaSystem::m_mapSchemaOffsets.end() && itSite->second > 0)
                                                s_nSite = g_Memory.ReadMemory<int>(uEnt + itSite->second);

                                            // Read bomb 3D position: entity -> m_pGameSceneNode -> m_vecAbsOrigin
                                            auto itNode = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("C_BaseEntity->m_pGameSceneNode"));
                                            auto itOrigin = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::HashConst("CGameSceneNode->m_vecAbsOrigin"));

                                            if (itNode != SchemaSystem::m_mapSchemaOffsets.end() && itNode->second > 0 &&
                                                itOrigin != SchemaSystem::m_mapSchemaOffsets.end() && itOrigin->second > 0)
                                            {
                                                std::uintptr_t pSceneNode = g_Memory.ReadMemory<std::uintptr_t>(uEnt + itNode->second);
                                                if (pSceneNode > 0x10000)
                                                {
                                                    s_vecBombPos = g_Memory.ReadMemory<Vector>(pSceneNode + itOrigin->second);
                                                    if (std::isfinite(s_vecBombPos.x) && std::isfinite(s_vecBombPos.y) && std::isfinite(s_vecBombPos.z))
                                                        s_bHasPos = true;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (s_flRemain >= 0.f && s_bHasPos)
                {
                    const char* szSite = (s_nSite == 0) ? "A" : (s_nSite == 1) ? "B" : "?";
                    Color col = (s_flRemain > 10.f) ? Color(255, 255, 50, 255) :
                                (s_flRemain > 5.f)  ? Color(255, 150, 0, 255)  :
                                                       Color(255, 30, 30, 255);

                    // Convert bomb world position to screen
                    ImVec2 screenPos;
                    if (Draw::WorldToScreen(s_vecBombPos, screenPos))
                    {
                        // ESP-style box around bomb
                        float boxW = 20.f;
                        float boxH = 16.f;
                        ImVec2 boxMin(screenPos.x - boxW, screenPos.y - boxH);
                        ImVec2 boxMax(screenPos.x + boxW, screenPos.y + boxH * 0.3f);

                        // Outline
                        Draw::AddRect(ImVec2(boxMin.x - 1.f, boxMin.y - 1.f), ImVec2(boxMax.x + 1.f, boxMax.y + 1.f), Color(0, 0, 0, 180), DRAW_RECT_NONE);
                        // Box
                        Draw::AddRect(boxMin, boxMax, col, DRAW_RECT_NONE);

                        // Timer text above box
                        char szWorld[64];
                        snprintf(szWorld, sizeof(szWorld), "C4 [%s] %.1fs", szSite, s_flRemain);
                        Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                            ImVec2(screenPos.x - 28.f, boxMin.y - Fonts::ESP->FontSize - 3.f),
                            std::string(szWorld), col, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));
                    }
                }
            }
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

        if (!g_Memory.IsWindowInForeground(X("Counter-Strike 2"), g_Globals.m_Instance) || Gui::m_bOpen)
        {
            g_Utilities.Sleep(1000.0f);
            continue;
        }

        // Snapshot entities
        std::unique_lock lockEntityGuard(EntityList::m_mtxEntities);
        std::vector<EntityObject_t> vecEntities;
        vecEntities.assign(EntityList::m_vecEntities.begin(), EntityList::m_vecEntities.end());
        lockEntityGuard.unlock();

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

            // ===== TRIGGERBOT =====
            if (g_License.HasFeature(ETier::MID) && CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bEnableTriggerbot))
                Triggerbot::Run(vecEntities);

            // ===== HIT SOUND =====
            if (CONFIG_GET(bool, g_Variables.m_Misc.m_bHitSound))
            {
                static std::map<std::uintptr_t, int> s_mapEnemyHP;
                
                for (EntityObject_t& object : vecEntities)
                {
                    if (object.m_eType != EEntityType::ENTITY_PLAYER) continue;

                    CCSPlayerController* pController = reinterpret_cast<CCSPlayerController*>(object.m_pEntity);
                    if (!pController || pController->m_bIsLocalPlayerController()) continue;

                    C_CSPlayerPawn* pPawn = reinterpret_cast<C_CSPlayerPawn*>(pController->m_hPawn().Get());
                    if (!pPawn || !pPawn->IsAlive())
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
                        if (it->second > iHealth && iHealth > 0)
                        {
                            PlaySoundA("C:\\Windows\\Media\\Windows Default.wav", NULL, SND_ASYNC | SND_FILENAME);
                        }
                    }
                    s_mapEnemyHP[uPawnAddress] = iHealth;
                }
            }

            // ===== FORCE RADAR (CS2 built-in radar) =====
            if (CONFIG_GET(bool, g_Variables.m_Radar.m_bInGameRadar))
                Radar::ForceRadarSpotted(vecEntities);

            // ===== AUDITORY SONAR =====
            Radar::AuditorySonar(vecEntities);

            // ===== PLAYER GLOW (CS2 native engine glow) =====
            if (CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow))
                PlayerGlow::Run(vecEntities);

            // ===== WORLD / NIGHT MODE / FOV =====
            World::Run(vecEntities);

            // ===== SKIN CHANGER =====
            SkinChanger::Run();
        }
        catch (...) { }

        g_Utilities.Sleep(INTERVAL_PER_TICK * 1000.0f);
    }
}

// -----------------------------------------------------------------------
void AutoAcceptThread()
{
    SetThreadPriorityWrapper();
    while (!g_Globals.m_bIsUnloading)
    {
        if (CONFIG_GET(bool, g_Variables.m_Misc.m_bAutoAccept))
        {
            HWND hCS2 = FindWindowA("SDL_app", "Counter-Strike 2");
            if (hCS2 && GetForegroundWindow() == hCS2)
            {
                RECT rect;
                if (GetClientRect(hCS2, &rect))
                {
                    int width = rect.right - rect.left;
                    int height = rect.bottom - rect.top;

                    if (width >= 800 && height >= 600)
                    {
                        HDC hdc = GetDC(hCS2);
                        if (hdc)
                        {
                            int centerX = width / 2;
                            int centerY = height / 2;
                            
                            int greenCount = 0;
                            // Scan a grid in the center
                            for (int x = centerX - 30; x <= centerX + 30; x += 10)
                            {
                                for (int y = centerY - 50; y <= centerY + 50; y += 10)
                                {
                                    COLORREF color = GetPixel(hdc, x, y);
                                    int r = GetRValue(color);
                                    int g = GetGValue(color);
                                    int b = GetBValue(color);
                                    
                                    // Accept button is vivid green: High G.
                                    if (g > 100 && g > (r + 20) && g > (b + 20))
                                        greenCount++;
                                }
                            }
                            ReleaseDC(hCS2, hdc);

                            // Out of ~77 points, if we find enough green, it's the accept button!
                            if (greenCount > 3)
                            {
                                // Move cursor to center and click
                                POINT pt;
                                GetCursorPos(&pt);
                                
                                POINT centerPt = { rect.left + centerX, rect.top + centerY };
                                SetCursorPos(centerPt.x, centerPt.y);
                                
                                INPUT inputs[2] = {};
                                inputs[0].type = INPUT_MOUSE;
                                inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                inputs[1].type = INPUT_MOUSE;
                                inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                SendInput(2, inputs, sizeof(INPUT));
                                
                                Sleep(100);
                                SetCursorPos(pt.x, pt.y);

                                Sleep(3000); // Wait 3 seconds to avoid spamming
                            }
                        }
                    }
                }
            }
        }
        g_Utilities.Sleep(500); // Check every half a second
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
            while (g_Memory.GetModule(NAVSYSTEM_DLL).m_uBaseAddress == 0U)
                g_Utilities.Sleep(500.0f);

            SchemaSystem::Setup();
        }
        catch (const std::exception& ex)
        {
            MessageBoxA(NULL, ex.what(), "SHIFTHUB - Xato", MB_OK | MB_ICONERROR);
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
    DetachConsole();

    try
    {
        // Create overlay (Login window already destroyed, ImGui context fresh)
        if (!Window::m_bInitialized)
            Window::Create();

        // Load weapon icon PNGs (must be after Window::Create for DX11 device)
        WeaponIcons::Initialize();
        WebRadar::Initialize();

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
    ConsoleAttach(X("External Base"));
    g_Globals.m_hDll = hInstance;

    if (!MainLoop(hInstance))
    {
        WeaponIcons::Shutdown();
        WebRadar::Shutdown();
        // g_Memory is a global — its destructor runs automatically at exit
        if (Window::m_bInitialized)
            Window::Destroy();
    }
    return EXIT_SUCCESS;
}
