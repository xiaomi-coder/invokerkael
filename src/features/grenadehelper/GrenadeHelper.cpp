#include "../../Includes.h"

static const char* GetGrenadeTypeName(EGrenadeType eType)
{
    switch (eType)
    {
    case GRENADE_SMOKE:   return "SMOKE";
    case GRENADE_FLASH:   return "FLASH";
    case GRENADE_MOLOTOV: return "MOLOTOV";
    case GRENADE_HE:      return "HE";
    default:              return "?";
    }
}

static Color GetGrenadeTypeColor(EGrenadeType eType)
{
    switch (eType)
    {
    case GRENADE_SMOKE:   return Color(130, 200, 255, 255);
    case GRENADE_FLASH:   return Color(255, 255, 100, 255);
    case GRENADE_MOLOTOV: return Color(255, 120, 30,  255);
    case GRENADE_HE:      return Color(255, 60,  60,  255);
    default:              return Color(255, 255, 255, 255);
    }
}

// =======================================================================
//  LINEUP DATA — standPos, aimAngle, landPos
// =======================================================================
const GrenadeLineup_t GrenadeHelper::g_MirageLineups[] =
{
    { "CT Smoke", "T Spawn -> CT", GRENADE_SMOKE,
      Vector(-1490.f,-680.f,-170.f), QAngle(-52.5f,-170.5f,0.f), Vector(-275.f,-2090.f,-168.f), 120.f, true, false },
    { "Jungle Smoke", "T Spawn -> Jungle", GRENADE_SMOKE,
      Vector(-1530.f,-620.f,-170.f), QAngle(-54.2f,-163.f,0.f), Vector(-630.f,-1790.f,-168.f), 120.f, true, false },
    { "Stairs Smoke", "T Spawn -> Zinalar", GRENADE_SMOKE,
      Vector(-1480.f,-700.f,-170.f), QAngle(-48.f,-179.f,0.f), Vector(-540.f,-1960.f,-100.f), 120.f, true, false },
    { "Connector Smoke", "Mid -> Window", GRENADE_SMOKE,
      Vector(-340.f,-920.f,-84.f), QAngle(-60.f,-130.f,0.f), Vector(-1010.f,-1420.f,-84.f), 100.f, true, false },
    { "B Short Smoke", "B Apts -> Short", GRENADE_SMOKE,
      Vector(-2175.f,450.f,-44.f), QAngle(-42.f,-140.f,0.f), Vector(-1730.f,270.f,-168.f), 100.f, true, false },
    { "B Bench Smoke", "B Apts -> Bench", GRENADE_SMOKE,
      Vector(-2160.f,490.f,-44.f), QAngle(-38.f,-155.f,0.f), Vector(-1880.f,350.f,-168.f), 100.f, true, false },
    { "A Pop Flash", "Palace -> A sayt", GRENADE_FLASH,
      Vector(-1680.f,-1850.f,-40.f), QAngle(-35.f,-90.f,0.f), Vector(-400.f,-1900.f,-100.f), 100.f, false, true },
    { "A Ramp Flash", "T Spawn -> Ramp", GRENADE_FLASH,
      Vector(-1440.f,-760.f,-170.f), QAngle(-40.f,-175.f,0.f), Vector(-800.f,-1500.f,-168.f), 120.f, true, false },
    { "Palace Molotov", "A -> Palace tagi", GRENADE_MOLOTOV,
      Vector(-320.f,-1960.f,-164.f), QAngle(-22.f,90.f,0.f), Vector(-320.f,-1650.f,-100.f), 80.f, false, false },
    { "B Van Molotov", "B Apts -> Van", GRENADE_MOLOTOV,
      Vector(-2180.f,430.f,-44.f), QAngle(-15.f,-165.f,0.f), Vector(-2050.f,180.f,-168.f), 100.f, false, true },
};
const int GrenadeHelper::g_nMirageLineupCount = sizeof(GrenadeHelper::g_MirageLineups) / sizeof(GrenadeLineup_t);

const GrenadeLineup_t GrenadeHelper::g_Dust2Lineups[] =
{
    { "Long Cross Smoke", "T Spawn -> Cross", GRENADE_SMOKE,
      Vector(-680.f,1750.f,3.f), QAngle(-47.f,128.f,0.f), Vector(1050.f,2250.f,0.f), 120.f, true, false },
    { "A CT Smoke", "Long -> CT", GRENADE_SMOKE,
      Vector(1250.f,2500.f,96.f), QAngle(-50.f,100.f,0.f), Vector(1550.f,2100.f,0.f), 100.f, true, false },
    { "Short Smoke", "T Spawn -> Short", GRENADE_SMOKE,
      Vector(-720.f,1700.f,3.f), QAngle(-43.f,110.f,0.f), Vector(350.f,1850.f,128.f), 120.f, true, false },
    { "B Doors Smoke", "Tunnel -> Eshik", GRENADE_SMOKE,
      Vector(-1540.f,2700.f,64.f), QAngle(-32.f,60.f,0.f), Vector(-1250.f,2450.f,32.f), 100.f, false, false },
    { "B Site Smoke", "Tunnel -> B sayt", GRENADE_SMOKE,
      Vector(-1400.f,2880.f,4.f), QAngle(-35.f,50.f,0.f), Vector(-1600.f,2350.f,16.f), 100.f, true, false },
    { "Long Flash", "T Spawn -> Long", GRENADE_FLASH,
      Vector(-650.f,1760.f,3.f), QAngle(-28.f,135.f,0.f), Vector(500.f,2400.f,0.f), 120.f, true, false },
    { "B Pop Flash", "Tunnel -> B sayt", GRENADE_FLASH,
      Vector(-1380.f,2740.f,36.f), QAngle(-18.f,55.f,0.f), Vector(-1500.f,2400.f,16.f), 100.f, false, true },
    { "Car Molotov", "Long -> Mashina", GRENADE_MOLOTOV,
      Vector(1200.f,2480.f,96.f), QAngle(-12.f,85.f,0.f), Vector(1450.f,2300.f,0.f), 100.f, false, false },
};
const int GrenadeHelper::g_nDust2LineupCount = sizeof(GrenadeHelper::g_Dust2Lineups) / sizeof(GrenadeLineup_t);

const GrenadeLineup_t GrenadeHelper::g_InfernoLineups[] =
{
    { "Pit Smoke", "2nd Mid -> Pit", GRENADE_SMOKE,
      Vector(570.f,520.f,160.f), QAngle(-62.f,175.f,0.f), Vector(1850.f,-340.f,80.f), 100.f, true, false },
    { "Library Smoke", "Apts -> Library", GRENADE_SMOKE,
      Vector(2070.f,170.f,225.f), QAngle(-55.f,150.f,0.f), Vector(2450.f,-350.f,160.f), 100.f, true, false },
    { "Arch Smoke", "T Spawn -> Arch", GRENADE_SMOKE,
      Vector(-360.f,760.f,100.f), QAngle(-68.f,-170.f,0.f), Vector(1200.f,250.f,100.f), 120.f, true, false },
    { "B CT Smoke", "Banana -> CT", GRENADE_SMOKE,
      Vector(240.f,3070.f,76.f), QAngle(-48.f,-25.f,0.f), Vector(450.f,2570.f,68.f), 100.f, true, false },
    { "Coffin Molotov", "Banana -> Coffin", GRENADE_MOLOTOV,
      Vector(310.f,3010.f,76.f), QAngle(-15.f,-5.f,0.f), Vector(350.f,2650.f,68.f), 100.f, false, false },
    { "A Site Flash", "2nd Mid -> A sayt", GRENADE_FLASH,
      Vector(600.f,560.f,160.f), QAngle(-45.f,-165.f,0.f), Vector(2000.f,-100.f,80.f), 100.f, true, false },
};
const int GrenadeHelper::g_nInfernoLineupCount = sizeof(GrenadeHelper::g_InfernoLineups) / sizeof(GrenadeLineup_t);

const GrenadeLineup_t GrenadeHelper::g_AnubisLineups[] =
{
    { "A Main Smoke", "T -> A Main", GRENADE_SMOKE,
      Vector(-900.f,-1000.f,30.f), QAngle(-45.f,90.f,0.f), Vector(-200.f,-800.f,0.f), 120.f, true, false },
    { "B Main Smoke", "T -> B Main", GRENADE_SMOKE,
      Vector(750.f,-700.f,50.f), QAngle(-42.f,45.f,0.f), Vector(500.f,-200.f,0.f), 120.f, true, false },
    { "Mid Smoke", "T Spawn -> Mid", GRENADE_SMOKE,
      Vector(-100.f,-1200.f,40.f), QAngle(-50.f,70.f,0.f), Vector(100.f,-500.f,0.f), 130.f, true, false },
};
const int GrenadeHelper::g_nAnubisLineupCount = sizeof(GrenadeHelper::g_AnubisLineups) / sizeof(GrenadeLineup_t);

const GrenadeMap_t GrenadeHelper::g_vecMaps[] =
{
    { "de_mirage",  GrenadeHelper::g_MirageLineups,  GrenadeHelper::g_nMirageLineupCount  },
    { "de_dust2",   GrenadeHelper::g_Dust2Lineups,   GrenadeHelper::g_nDust2LineupCount   },
    { "de_inferno", GrenadeHelper::g_InfernoLineups, GrenadeHelper::g_nInfernoLineupCount },
    { "de_anubis",  GrenadeHelper::g_AnubisLineups,  GrenadeHelper::g_nAnubisLineupCount  },
};
const int GrenadeHelper::g_nMapCount = sizeof(GrenadeHelper::g_vecMaps) / sizeof(GrenadeMap_t);

// -----------------------------------------------------------------------
// Get current map data (CACHED)
// -----------------------------------------------------------------------
const GrenadeMap_t* GrenadeHelper::GetCurrentMapData()
{
    m_nCacheTick++;
    if (m_nCacheTick < 120 && m_pCachedMap != nullptr)
        return m_pCachedMap;
    m_nCacheTick = 0;

    std::string strMapName = g_Memory.ReadMemoryString(g_Interfaces.m_GlobalVars.m_uMapNameShort);
    if (strMapName.empty()) { m_pCachedMap = nullptr; return nullptr; }

    for (int i = 0; i < g_nMapCount; i++)
    {
        if (strMapName.find(g_vecMaps[i].m_szMapName) != std::string::npos ||
            std::string(g_vecMaps[i].m_szMapName).find(strMapName) != std::string::npos)
        { m_pCachedMap = &g_vecMaps[i]; return m_pCachedMap; }
    }
    m_pCachedMap = nullptr;
    return nullptr;
}

// -----------------------------------------------------------------------
// Draw a parabolic arc between two 3D points
// -----------------------------------------------------------------------
static void DrawTrajectoryArc(const Vector& vecStart, const Vector& vecEnd, const Color& col, float flArcHeight, float flThickness)
{
    const int nSegments = 20;
    Vector vecThrowStart = vecStart;
    vecThrowStart.z += 64.f; // o'yinchining qo'l balandligi

    ImVec2 prevScreen;
    bool bPrevValid = false;

    for (int s = 0; s <= nSegments; s++)
    {
        float t = static_cast<float>(s) / static_cast<float>(nSegments);

        // Parabola: lerp X/Y, arc Z
        Vector vecPoint;
        vecPoint.x = vecThrowStart.x + (vecEnd.x - vecThrowStart.x) * t;
        vecPoint.y = vecThrowStart.y + (vecEnd.y - vecThrowStart.y) * t;
        vecPoint.z = vecThrowStart.z + (vecEnd.z - vecThrowStart.z) * t + flArcHeight * 4.f * t * (1.f - t);

        ImVec2 curScreen;
        bool bCurValid = Draw::WorldToScreen(vecPoint, curScreen);

        if (bCurValid && bPrevValid)
            Draw::AddLine(prevScreen, curScreen, col, flThickness);

        prevScreen = curScreen;
        bPrevValid = bCurValid;
    }
}

// -----------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------
void GrenadeHelper::Render()
{
    if (!CONFIG_GET(bool, g_Variables.m_GrenadeHelper.m_bEnable))
        return;

    C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocalPawn || !pLocalPawn->IsAlive())
        return;

    // Faqat granata ushlayotganda ko'rsatish
    CCSPlayer_WeaponServices* pWeaponServices = pLocalPawn->m_pWeaponServices();
    if (!pWeaponServices) return;

    CHandle<C_BasePlayerWeapon> hActiveWeapon = pWeaponServices->m_hActiveWeapon();
    if (!hActiveWeapon.IsValid()) return;

    C_BasePlayerWeapon* pWeapon = reinterpret_cast<C_BasePlayerWeapon*>(hActiveWeapon.Get());
    if (!pWeapon) return;

    std::uint16_t nDefIndex = pWeapon->GetItemDefinitionIndex();
    // 43=Flash, 44=HE, 45=Smoke, 46=Molotov, 47=Decoy, 48=Incendiary
    bool bHoldingGrenade = (nDefIndex >= 43 && nDefIndex <= 48);
    if (!bHoldingGrenade) return;

    CGameSceneNode* pNode = pLocalPawn->m_pGameSceneNode();
    if (!pNode) return;

    Vector vecLocalPos = pNode->m_vecAbsOrigin();
    if (vecLocalPos.IsZero()) return;

    const GrenadeMap_t* pMapData = GetCurrentMapData();
    if (!pMapData) return;

    float flMaxDist = CONFIG_GET(float, g_Variables.m_GrenadeHelper.m_flMaxDistance);
    bool bShowSmoke = CONFIG_GET(bool, g_Variables.m_GrenadeHelper.m_bShowSmoke);
    bool bShowFlash = CONFIG_GET(bool, g_Variables.m_GrenadeHelper.m_bShowFlash);
    bool bShowMolly = CONFIG_GET(bool, g_Variables.m_GrenadeHelper.m_bShowMolotov);
    bool bShowHE    = CONFIG_GET(bool, g_Variables.m_GrenadeHelper.m_bShowHE);

    for (int i = 0; i < pMapData->m_nLineupCount; i++)
    {
        const GrenadeLineup_t& lineup = pMapData->m_pLineups[i];

        switch (lineup.m_eType)
        {
        case GRENADE_SMOKE:   if (!bShowSmoke) continue; break;
        case GRENADE_FLASH:   if (!bShowFlash) continue; break;
        case GRENADE_MOLOTOV: if (!bShowMolly) continue; break;
        case GRENADE_HE:      if (!bShowHE)    continue; break;
        }

        float flDist = vecLocalPos.DistTo(lineup.m_vecStandPos);
        if (flDist > flMaxDist) continue;

        Color colType = GetGrenadeTypeColor(lineup.m_eType);
        bool bNearby = (flDist < lineup.m_flRadius);

        // ============================================================
        // 1) TRAEKTORIYA CHIZIG'I (Arc from stand to land)
        // ============================================================
        float flArcHeight = 200.f;
        if (lineup.m_bJumpThrow) flArcHeight = 350.f;

        Color colArc = bNearby
            ? colType
            : Color(static_cast<int>(colType.r()), static_cast<int>(colType.g()), static_cast<int>(colType.b()), 120);
        float flLineWidth = bNearby ? 2.5f : 1.5f;

        DrawTrajectoryArc(lineup.m_vecStandPos, lineup.m_vecLandPos, colArc, flArcHeight, flLineWidth);

        // ============================================================
        // 2) TUSHISH JOYI MARKERI (Landing circle + X)
        // ============================================================
        ImVec2 landScreen;
        if (Draw::WorldToScreen(lineup.m_vecLandPos, landScreen))
        {
            float flCircleSize = bNearby ? 18.f : 10.f;

            // Tashqi doira
            Draw::AddCircle(landScreen, flCircleSize, colType, 24, DRAW_CIRCLE_NONE, Color(0,0,0,0), 2.f);

            // X belgisi
            float cx = bNearby ? 8.f : 5.f;
            Draw::AddLine(ImVec2(landScreen.x - cx, landScreen.y - cx),
                          ImVec2(landScreen.x + cx, landScreen.y + cx), colType, 2.f);
            Draw::AddLine(ImVec2(landScreen.x + cx, landScreen.y - cx),
                          ImVec2(landScreen.x - cx, landScreen.y + cx), colType, 2.f);

            // Nomi va turi
            char szLabel[96];
            snprintf(szLabel, sizeof(szLabel), "[%s] %s", GetGrenadeTypeName(lineup.m_eType), lineup.m_szName);
            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                ImVec2(landScreen.x + flCircleSize + 4.f, landScreen.y - 12.f),
                szLabel, colType, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));

            // Tavsif
            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                ImVec2(landScreen.x + flCircleSize + 4.f, landScreen.y),
                lineup.m_szDescription, Color(180, 180, 180, 180), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 150));

            // Tashlash usuli
            if (bNearby)
            {
                const char* szThrow = lineup.m_bJumpThrow ? "SAKRAB TASHLANG!" :
                                      lineup.m_bRunThrow  ? "YUGRIB TASHLANG!" : "ODDIY TASHLANG!";
                Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                    ImVec2(landScreen.x + flCircleSize + 4.f, landScreen.y + 12.f),
                    szThrow, Color(255, 200, 50, 255), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));
            }
        }

        // ============================================================
        // 3) TURISH JOYI MARKERI (Stand position circle on ground)
        // ============================================================
        ImVec2 standScreen;
        if (Draw::WorldToScreen(lineup.m_vecStandPos, standScreen))
        {
            if (bNearby)
            {
                // Yashil doira — shu yerda turing
                Draw::AddCircle(standScreen, 14.f, Color(0, 255, 80, 255), 20, DRAW_CIRCLE_NONE, Color(0,0,0,0), 2.f);
                Draw::AddCircle(standScreen, 4.f, Color(0, 255, 80, 255), 8, DRAW_CIRCLE_FILLED, Color(0,0,0,0), 0.f);
                Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                    ImVec2(standScreen.x + 18.f, standScreen.y - 4.f),
                    "SHU YERDA TURING", Color(0, 255, 80, 255), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));
            }
            else
            {
                // Kichik marker — uzoqdan ko'rinadi
                float flDistM = flDist * 0.01905f;
                char szDist[32];
                snprintf(szDist, sizeof(szDist), "%.0fm", flDistM);
                Draw::AddCircle(standScreen, 6.f, colType, 12, DRAW_CIRCLE_FILLED, Color(0,0,0,0), 0.f);
                Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                    ImVec2(standScreen.x + 10.f, standScreen.y - 4.f),
                    szDist, Color(180, 180, 180, 150), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 150));
            }
        }
    }
}
