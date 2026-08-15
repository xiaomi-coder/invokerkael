#include "../../Includes.h"

// -----------------------------------------------------------------------
// Helper: build a screen-space bounding box from feet/head positions
// -----------------------------------------------------------------------
bool ESP::GetBoundingBox(C_CSPlayerPawn* pPawn, ImVec2& vecMin, ImVec2& vecMax)
{
    CGameSceneNode* pSceneNode = pPawn->m_pGameSceneNode();
    if (!pSceneNode)
        return false;

    Vector vecOrigin = pSceneNode->m_vecAbsOrigin();
    if (vecOrigin.x == 0.f && vecOrigin.y == 0.f && vecOrigin.z == 0.f)
        return false;

    float flHeadZ = 72.f;
    CCollisionProperty* pCollision = pPawn->m_pCollision();
    if (pCollision && reinterpret_cast<std::uintptr_t>(pCollision) > 0x10000)
    {
        Vector vecMaxs = pCollision->m_vecMaxs();
        if (vecMaxs.z > 10.f && vecMaxs.z < 100.f) flHeadZ = vecMaxs.z;
    }

    Vector vecHead = vecOrigin;
    vecHead.z += flHeadZ;

    ImVec2 screenFoot, screenHead;
    if (!Draw::WorldToScreen(vecOrigin, screenFoot) || !Draw::WorldToScreen(vecHead, screenHead))
        return false;

    float flHeight = screenFoot.y - screenHead.y;
    float flWidth  = flHeight * 0.5f;

    vecMin = ImVec2(screenFoot.x - flWidth * 0.5f, screenHead.y);
    vecMax = ImVec2(screenFoot.x + flWidth * 0.5f, screenFoot.y);
    return true;
}

// -----------------------------------------------------------------------
// Helper: pick colour based on team / visibility
// -----------------------------------------------------------------------
Color ESP::GetPlayerColor(CCSPlayerController* pController, C_CSPlayerPawn* pPawn)
{
    CCSPlayerController* pLocal = g_Globals.m_LocalPlayer.m_pController;
    if (!pLocal)
        return CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyVisible);

    bool bSameTeam = (pPawn->m_iTeamNum() == g_Globals.m_LocalPlayer.m_pPlayerPawn->m_iTeamNum());
    if (bSameTeam)
        return CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colTeammate);

    // simple spotted-state check for visibility
    bool bVisible = pPawn->m_entitySpottedState().m_bSpotted;
    return bVisible
        ? CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyVisible)
        : CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyOccluded);
}

// -----------------------------------------------------------------------
// Draw: 2-D solid rectangle box
// -----------------------------------------------------------------------
void ESP::DrawBox2D(const ImVec2& vecMin, const ImVec2& vecMax, const Color& col)
{
    bool bOutline = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline);
    unsigned int uFlags = DRAW_RECT_NONE;
    if (bOutline) uFlags |= DRAW_RECT_OUTLINE;

    Draw::AddRect(vecMin, vecMax, col, uFlags, Color(0, 0, 0, 200));
}

// -----------------------------------------------------------------------
// Draw: corner box (only the four corners are drawn)
// -----------------------------------------------------------------------
void ESP::DrawBoxCorner(const ImVec2& vecMin, const ImVec2& vecMax, const Color& col)
{
    float flW = (vecMax.x - vecMin.x) * 0.25f;
    float flH = (vecMax.y - vecMin.y) * 0.25f;
    Color outline(0, 0, 0, 200);

    auto drawCorner = [&](float ox, float oy, float sx, float sy)
    {
        ImVec2 A(ox, oy);
        ImVec2 Bh(ox + flW * sx, oy);
        ImVec2 Bv(ox, oy + flH * sy);
        // outline (slightly offset)
        Draw::AddLine(ImVec2(A.x - sx, A.y - sy), ImVec2(Bh.x - sx, Bh.y - sy), outline, 3.0f);
        Draw::AddLine(ImVec2(A.x - sx, A.y - sy), ImVec2(Bv.x - sx, Bv.y - sy), outline, 3.0f);
        // colour
        Draw::AddLine(A, Bh, col, 1.5f);
        Draw::AddLine(A, Bv, col, 1.5f);
    };

    drawCorner(vecMin.x, vecMin.y,  1.f,  1.f); // top-left
    drawCorner(vecMax.x, vecMin.y, -1.f,  1.f); // top-right
    drawCorner(vecMin.x, vecMax.y,  1.f, -1.f); // bottom-left
    drawCorner(vecMax.x, vecMax.y, -1.f, -1.f); // bottom-right
}

// -----------------------------------------------------------------------
// Draw: health bar (vertical, left side)
// -----------------------------------------------------------------------
void ESP::DrawHealthBar(const ImVec2& vecMin, const ImVec2& vecMax, int iHealth, int iMaxHealth)
{
    if (iMaxHealth <= 0) iMaxHealth = 100;
    float flFrac   = static_cast<float>(iHealth) / static_cast<float>(iMaxHealth);
    flFrac         = std::clamp(flFrac, 0.f, 1.f);

    // green → yellow → red based on HP
    int r = static_cast<int>((1.f - flFrac) * 255.f);
    int g = static_cast<int>(flFrac         * 255.f);
    Color colHP(r, g, 0, 255);

    float flBarW = 4.f;
    float flPad  = 2.f;
    ImVec2 bgMin(vecMin.x - flBarW - flPad, vecMin.y);
    ImVec2 bgMax(vecMin.x - flPad,          vecMax.y);

    float flFilledY = bgMax.y - (bgMax.y - bgMin.y) * flFrac;

    // background
    Draw::AddRect(bgMin, bgMax, Color(0, 0, 0, 180), DRAW_RECT_FILLED);
    // filled portion
    Draw::AddRect(ImVec2(bgMin.x, flFilledY), bgMax, colHP, DRAW_RECT_FILLED);

    // HP number at bottom
    char szHP[8];
    snprintf(szHP, sizeof(szHP), "%d", iHealth);
    Draw::AddText(
        Fonts::ESP, Fonts::ESP->FontSize,
        ImVec2(bgMin.x - 1.f, bgMax.y + 1.f),
        szHP, Color(255, 255, 255, 220),
        DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200)
    );
}

// -----------------------------------------------------------------------
// Draw: player name (top-center)
// -----------------------------------------------------------------------
void ESP::DrawName(const ImVec2& vecMin, const ImVec2& vecMax, const std::string& szName)
{
    if (szName.empty()) return;
    ImVec2 textSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, szName.c_str());
    float  cx       = (vecMin.x + vecMax.x) * 0.5f - textSize.x * 0.5f;
    Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
        ImVec2(cx, vecMin.y - textSize.y - 1.f),
        szName, Color(255, 255, 255, 255),
        DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));
}

// -----------------------------------------------------------------------
// Draw: weapon name (bottom-center)
// -----------------------------------------------------------------------
static Color GetWeaponColor(const std::string& w)
{
    if (w == "ak47" || w == "m4a1" || w == "m4a1_silencer" || w == "aug" || w == "sg556" || w == "famas" || w == "galilar")
        return Color(255, 80, 80, 255);
    if (w == "awp" || w == "ssg08" || w == "scar20" || w == "g3sg1")
        return Color(255, 50, 200, 255);
    if (w == "mp9" || w == "mac10" || w == "mp7" || w == "mp5sd" || w == "ump45" || w == "p90" || w == "bizon")
        return Color(100, 200, 255, 255);
    if (w == "deagle" || w == "elite" || w == "fiveseven" || w == "glock" || w == "hkp2000" || w == "p250" || w == "tec9" || w == "usp_silencer" || w == "cz75a" || w == "revolver")
        return Color(255, 200, 50, 255);
    if (w == "nova" || w == "xm1014" || w == "sawedoff" || w == "mag7")
        return Color(255, 140, 50, 255);
    if (w == "m249" || w == "negev")
        return Color(200, 100, 255, 255);
    if (w.find("knife") != std::string::npos || w.find("bayonet") != std::string::npos)
        return Color(200, 200, 200, 255);
    if (w == "flashbang" || w == "hegrenade" || w == "smokegrenade" || w == "molotov" || w == "incgrenade" || w == "decoy")
        return Color(100, 255, 100, 255);
    return Color(200, 200, 100, 230);
}

void ESP::DrawWeapon(const ImVec2& vecMin, const ImVec2& vecMax, const std::string& szWeapon)
{
    if (szWeapon.empty()) return;

    // Try to draw weapon icon instead of text
    if (WeaponIcons::HasIcon(szWeapon))
    {
        ImTextureID tex = WeaponIcons::GetIcon(szWeapon);
        if (tex)
        {
            float flBoxW = vecMax.x - vecMin.x;

            // Scale icon to fit nicely under the bounding box
            int iTexW = 0, iTexH = 0;
            WeaponIcons::GetIconSize(szWeapon, iTexW, iTexH);

            float flAspect = (iTexH > 0) ? (float)iTexW / (float)iTexH : 2.67f;
            float flIconW = std::clamp(flBoxW * 0.75f, 28.f, 64.f);
            float flIconH = flIconW / flAspect;

            float cx = (vecMin.x + vecMax.x) * 0.5f;
            float cy = vecMax.y + Fonts::ESP->FontSize + 4.f; // below HP number

            Color colWeapon = GetWeaponColor(szWeapon);

            Draw::AddImage(tex,
                ImVec2(cx - flIconW * 0.5f, cy),
                ImVec2(cx + flIconW * 0.5f, cy + flIconH),
                colWeapon);
            return;
        }
    }

    // Fallback: draw weapon name as colored text
    Color colWeapon = GetWeaponColor(szWeapon);

    std::string szDisplay = szWeapon;
    std::transform(szDisplay.begin(), szDisplay.end(), szDisplay.begin(), ::toupper);

    ImVec2 textSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, szDisplay.c_str());
    float  cx = (vecMin.x + vecMax.x) * 0.5f - textSize.x * 0.5f;
    float  cy = vecMax.y + 2.f;

    Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
        ImVec2(cx, cy),
        szDisplay, colWeapon,
        DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));
}

// -----------------------------------------------------------------------
// Draw: distance (below weapon name)
// -----------------------------------------------------------------------
void ESP::DrawDistance(const ImVec2& vecMin, const ImVec2& vecMax, float flDist)
{
    char szDist[16];
    snprintf(szDist, sizeof(szDist), "%.0fm", flDist / 52.49f); // units → metres
    ImVec2 textSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, szDist);
    float  cx       = (vecMin.x + vecMax.x) * 0.5f - textSize.x * 0.5f;
    Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
        ImVec2(cx, vecMax.y + Fonts::ESP->FontSize + 3.f),
        szDist, Color(180, 180, 180, 200),
        DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 180));
}

// -----------------------------------------------------------------------
// Draw: head dot
// -----------------------------------------------------------------------
void ESP::DrawHeadDot(C_CSPlayerPawn* pPawn)
{
    CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
    if (!pNode) return;
    Vector vecHead = pNode->m_vecAbsOrigin();
    CCollisionProperty* pCol = pPawn->m_pCollision();
    float flH = 72.f;
    if (pCol && reinterpret_cast<std::uintptr_t>(pCol) > 0x10000)
    {
        float fz = pCol->m_vecMaxs().z;
        if (fz > 10.f && fz < 100.f) flH = fz;
    }
    vecHead.z += flH;

    ImVec2 screenHead;
    if (!Draw::WorldToScreen(vecHead, screenHead)) return;
    Draw::AddCircle(screenHead, 3.f, Color(255, 255, 255, 220), 12, DRAW_CIRCLE_FILLED);
}

// -----------------------------------------------------------------------
// Draw: snapline from bottom-center of screen
// -----------------------------------------------------------------------
void ESP::DrawSnapline(const ImVec2& vecMin, const ImVec2& vecMax)
{
    ImVec2 origin(Window::m_iWidth * 0.5f, static_cast<float>(Window::m_iHeight));
    ImVec2 target((vecMin.x + vecMax.x) * 0.5f, vecMax.y);
    Draw::AddLine(origin, target, Color(255, 255, 100, 120));
}

// -----------------------------------------------------------------------
// Draw: skeleton (simplified – spine + arms + legs)
// Bone indices for CS2 may vary; this is a common approximate set.
// -----------------------------------------------------------------------
void ESP::DrawSkeleton(C_CSPlayerPawn* pPawn, const Color& col)
{
    CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
    if (!pNode) return;

    BoneData_t* pBones = pNode->m_pBoneCache();
    if (!pBones || reinterpret_cast<std::uintptr_t>(pBones) < 0x1000) return;

    // pairs: { parent, child }
    // CS2 bone indices (may need adjustment per update)
    static const std::pair<int,int> skeleton[] = {
        {6,  5},   // head -> neck
        {5,  4},   // spine top
        {4,  2},   // spine low
        {2,  0},   // pelvis
        // arms
        {5,  8},   // neck -> left shoulder
        {8,  9},   // left upper arm
        {9,  11},  // left forearm -> hand
        {5,  13},  // neck -> right shoulder
        {13, 14},  // right upper arm
        {14, 16},  // right forearm -> hand
        // legs
        {0,  23},  // pelvis -> left thigh
        {23, 24},  // left calf
        {24, 25},  // left foot
        {0,  26},  // pelvis -> right thigh
        {26, 27},  // right calf
        {27, 28},  // right foot
    };

    for (auto& [parent, child] : skeleton)
    {
        BoneData_t bParent = g_Memory.ReadMemory<BoneData_t>(reinterpret_cast<std::uintptr_t>(pBones) + parent * sizeof(BoneData_t));
        BoneData_t bChild  = g_Memory.ReadMemory<BoneData_t>(reinterpret_cast<std::uintptr_t>(pBones) + child  * sizeof(BoneData_t));

        if (bParent.m_vecPosition.x == 0.f && bParent.m_vecPosition.y == 0.f) continue;
        if (bChild.m_vecPosition.x == 0.f && bChild.m_vecPosition.y == 0.f) continue;

        ImVec2 scrParent, scrChild;
        if (!Draw::WorldToScreen(bParent.m_vecPosition, scrParent)) continue;
        if (!Draw::WorldToScreen(bChild.m_vecPosition,  scrChild))  continue;

        Draw::AddLine(scrParent, scrChild, col, 1.f);
    }
}

// -----------------------------------------------------------------------
// Draw: filled body ("ghost" / "ruh" mode)
// Semi-transparent silhouette from bone positions.
// -----------------------------------------------------------------------
void ESP::DrawFilledBody(C_CSPlayerPawn* pPawn, const Color& col)
{
    CGameSceneNode* pNode = pPawn->m_pGameSceneNode();
    if (!pNode) return;

    BoneData_t* pBones = pNode->m_pBoneCache();
    if (!pBones || reinterpret_cast<std::uintptr_t>(pBones) < 0x1000) return;

    auto ReadBone = [&](int idx) -> ImVec2 {
        BoneData_t b = g_Memory.ReadMemory<BoneData_t>(
            reinterpret_cast<std::uintptr_t>(pBones) + idx * sizeof(BoneData_t));
        if (b.m_vecPosition.x == 0.f && b.m_vecPosition.y == 0.f) return ImVec2(-1, -1);
        ImVec2 scr;
        if (!Draw::WorldToScreen(b.m_vecPosition, scr))
            return ImVec2(-1, -1);
        return scr;
    };

    auto Valid = [](const ImVec2& v) { return v.x >= 0 && v.y >= 0; };

    // Read bones (updated indices for CS2)
    ImVec2 head = ReadBone(6), neck = ReadBone(5);
    ImVec2 spineTop = ReadBone(4), pelvis = ReadBone(0);
    ImVec2 lShoulder = ReadBone(8), lElbow = ReadBone(9), lHand = ReadBone(11);
    ImVec2 rShoulder = ReadBone(13), rElbow = ReadBone(14), rHand = ReadBone(16);
    ImVec2 lThigh = ReadBone(23), lKnee = ReadBone(24), lFoot = ReadBone(25);
    ImVec2 rThigh = ReadBone(26), rKnee = ReadBone(27), rFoot = ReadBone(28);

    if (!Valid(neck) || !Valid(pelvis)) return;

    Color colFill(col, 45);
    Color colEdge(col, 110);

    auto FillTri = [&](const ImVec2& a, const ImVec2& b, const ImVec2& c) {
        if (!Valid(a) || !Valid(b) || !Valid(c)) return;
        std::vector<ImVec2> pts = { a, b, c };
        Draw::AddPolygon(pts, colFill, DRAW_POLYGON_FILLED, colEdge, true, 1.f);
    };

    // TORSO
    if (Valid(lShoulder) && Valid(rShoulder))
    {
        FillTri(neck, lShoulder, pelvis);
        FillTri(neck, rShoulder, pelvis);
    }

    // HEAD circle
    if (Valid(head) && Valid(neck))
    {
        float hs = std::sqrtf((head.x-neck.x)*(head.x-neck.x)+(head.y-neck.y)*(head.y-neck.y)) * 0.7f;
        hs = std::clamp(hs, 5.f, 25.f);
        Draw::AddCircle(head, hs, colFill, 12, DRAW_CIRCLE_FILLED, colEdge);
    }

    // LEFT ARM
    if (Valid(lShoulder) && Valid(lElbow)) FillTri(lShoulder, lElbow, ImVec2(lShoulder.x+3,lShoulder.y+3));
    if (Valid(lElbow) && Valid(lHand)) FillTri(lElbow, lHand, ImVec2(lElbow.x+2,lElbow.y+2));

    // RIGHT ARM
    if (Valid(rShoulder) && Valid(rElbow)) FillTri(rShoulder, rElbow, ImVec2(rShoulder.x-3,rShoulder.y+3));
    if (Valid(rElbow) && Valid(rHand)) FillTri(rElbow, rHand, ImVec2(rElbow.x-2,rElbow.y+2));

    // LEFT LEG
    if (Valid(lThigh) && Valid(lKnee)) FillTri(pelvis, lThigh, ImVec2(lThigh.x+4,(pelvis.y+lThigh.y)*0.5f));
    if (Valid(lKnee) && Valid(lFoot)) { FillTri(lThigh, lKnee, ImVec2(lThigh.x+3,(lThigh.y+lKnee.y)*0.5f)); FillTri(lKnee, lFoot, ImVec2(lKnee.x+2,(lKnee.y+lFoot.y)*0.5f)); }

    // RIGHT LEG
    if (Valid(rThigh) && Valid(rKnee)) FillTri(pelvis, rThigh, ImVec2(rThigh.x-4,(pelvis.y+rThigh.y)*0.5f));
    if (Valid(rKnee) && Valid(rFoot)) { FillTri(rThigh, rKnee, ImVec2(rThigh.x-3,(rThigh.y+rKnee.y)*0.5f)); FillTri(rKnee, rFoot, ImVec2(rKnee.x-2,(rKnee.y+rFoot.y)*0.5f)); }
}

// -----------------------------------------------------------------------
// Draw: Off-Screen ESP (Arrows pointing to enemies outside the screen)
// -----------------------------------------------------------------------
void ESP::DrawOffScreenESP(C_CSPlayerPawn* pPawn, const Color& col)
{
    C_CSPlayerPawn* pLocal = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocal || !pPawn) return;

    CGameSceneNode* pLocalNode = pLocal->m_pGameSceneNode();
    CGameSceneNode* pEnemyNode = pPawn->m_pGameSceneNode();
    if (!pLocalNode || !pEnemyNode) return;

    Vector vecLocalOrigin = pLocalNode->m_vecAbsOrigin();
    Vector vecEnemyOrigin = pEnemyNode->m_vecAbsOrigin();

    QAngle angView = g_Interfaces.m_CSGOInput.m_angViewAngle;
    
    Vector vecDelta = vecEnemyOrigin - vecLocalOrigin;
    float flYaw = atan2(vecDelta.y, vecDelta.x) * (180.f / M_PI);
    float flYawDelta = flYaw - angView.y;

    // Normalize flYawDelta
    while (flYawDelta > 180.f) flYawDelta -= 360.f;
    while (flYawDelta < -180.f) flYawDelta += 360.f;

    float flYawRad = flYawDelta * (M_PI / 180.f);
    
    float flCX = Window::m_iWidth * 0.5f;
    float flCY = Window::m_iHeight * 0.5f;
    
    float flRadius = std::min(Window::m_iWidth, Window::m_iHeight) * 0.35f;
    
    ImVec2 tip(flCX - sin(flYawRad) * (flRadius + 15.f), flCY - cos(flYawRad) * (flRadius + 15.f));
    ImVec2 base1(flCX - sin(flYawRad + 0.08f) * flRadius, flCY - cos(flYawRad + 0.08f) * flRadius);
    ImVec2 base2(flCX - sin(flYawRad - 0.08f) * flRadius, flCY - cos(flYawRad - 0.08f) * flRadius);
    
    Draw::AddTriangle(tip, base1, base2, col, DRAW_TRIANGLE_FILLED, Color(0, 0, 0, 150), 1.f);
}

// -----------------------------------------------------------------------
// Main per-player render
// -----------------------------------------------------------------------
void ESP::RenderPlayer(CCSPlayerController* pController, C_CSPlayerPawn* pPawn)
{
    ImVec2 vecMin, vecMax;
    if (!GetBoundingBox(pPawn, vecMin, vecMax))
    {
        return;
    }

    Color col = GetPlayerColor(pController, pPawn);

    // ignore teammates?
    bool bIgnoreTeam = CONFIG_GET_ARRAY(bool, g_Variables.m_PlayerVisuals.m_vecVisualsModifiers, VISUALS_IGNORE_TEAMMATES);
    if (bIgnoreTeam && pPawn->m_iTeamNum() == g_Globals.m_LocalPlayer.m_pPlayerPawn->m_iTeamNum())
        return;

    int iBoxType = CONFIG_GET(int, g_Variables.m_PlayerVisuals.m_iBoxType);

    // --- Filled Body (draw FIRST so other elements appear on top) ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawFilledBody))
        DrawFilledBody(pPawn, col);

    // --- Box ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBox))
    {
        if (iBoxType == BOX_TYPE_2D || iBoxType == BOX_TYPE_BOTH)
            DrawBox2D(vecMin, vecMax, col);
        if (iBoxType == BOX_TYPE_CORNER || iBoxType == BOX_TYPE_BOTH)
            DrawBoxCorner(vecMin, vecMax, col);
    }

    // --- Health bar ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHealthBar))
        DrawHealthBar(vecMin, vecMax, pPawn->m_iHealth(), pPawn->m_iMaxHealth());

    // --- Name ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawName))
    {
        std::string strName = pController->m_strSanitizedPlayerName();
        strName.erase(std::remove_if(strName.begin(), strName.end(), [](unsigned char c) {
            return c < 32 || c > 126;
        }), strName.end());

        DrawName(vecMin, vecMax, strName);
    }

    // --- Weapon ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawWeapon))
        DrawWeapon(vecMin, vecMax, pPawn->m_strActiveWeaponName());

    // --- Distance ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawDistance))
    {
        C_CSPlayerPawn* pLocal = g_Globals.m_LocalPlayer.m_pPlayerPawn;
        if (pLocal)
        {
            float flDist = (pPawn->m_pGameSceneNode()->m_vecAbsOrigin() - pLocal->m_pGameSceneNode()->m_vecAbsOrigin()).Length();
            DrawDistance(vecMin, vecMax, flDist);
        }
    }

    // --- Has C4 Warning ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHasC4))
    {
        CCSPlayer_WeaponServices* pWeaponServices = pPawn->m_pWeaponServices();
        if (pWeaponServices && reinterpret_cast<std::uintptr_t>(pWeaponServices) > 0x1000)
        {
            C_NetworkUtlVectorBaseSimple hWeapons = pWeaponServices->m_hMyWeapons();
            if (hWeapons.m_nSize > 0 && hWeapons.m_nSize <= 10 && hWeapons.m_pData > 0x1000)
            {
                bool bHasC4 = false;
                for (int i = 0; i < hWeapons.m_nSize; i++)
                {
                    CHandle<C_BasePlayerWeapon> hWeaponHandle = g_Memory.ReadMemory<CHandle<C_BasePlayerWeapon>>(hWeapons.m_pData + (i * 0x4));
                    if (hWeaponHandle.IsValid())
                    {
                        C_BasePlayerWeapon* pWeapon = hWeaponHandle.Get();
                        if (pWeapon && reinterpret_cast<std::uintptr_t>(pWeapon) > 0x1000)
                        {
                            if (pWeapon->GetSchemaName() == "C_C4" || pWeapon->GetItemDefinitionIndex() == 49) // C4 Check
                            {
                                bHasC4 = true;
                                break;
                            }
                        }
                    }
                }

                if (bHasC4)
                {
                    ImVec2 textSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize + 2.f, FLT_MAX, 0.f, "BOMBA");
                    float cx = (vecMin.x + vecMax.x) * 0.5f - textSize.x * 0.5f;
                    // Draw red 'BOMBA' text above the player name
                    float flY = vecMin.y - textSize.y - 1.f;
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawName))
                        flY -= Fonts::ESP->FontSize + 2.f;

                    Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize + 2.f,
                        ImVec2(cx, flY),
                        "BOMBA", Color(255, 30, 30, 255),
                        DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));
                }
            }
        }
    }

    // --- Head dot ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHeadDot))
        DrawHeadDot(pPawn);

    // --- Snapline ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSnaplines))
        DrawSnapline(vecMin, vecMax);

    // --- Skeleton ---
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSkeleton))
        DrawSkeleton(pPawn, Color(col, 160));
}

// -----------------------------------------------------------------------
// RenderGlowInfo: Draw HP and Weapon for Glow mode
// -----------------------------------------------------------------------
void ESP::RenderGlowInfo(CCSPlayerController* pController, C_CSPlayerPawn* pPawn)
{
    ImVec2 vecMin, vecMax;
    if (!GetBoundingBox(pPawn, vecMin, vecMax))
        return;

    // ignore teammates?
    bool bIgnoreTeam = CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowEnemyOnly);
    if (bIgnoreTeam && pPawn->m_iTeamNum() == g_Globals.m_LocalPlayer.m_pPlayerPawn->m_iTeamNum())
        return;

    // Head text: [ 100 HP ]
    int iHP = std::clamp(pPawn->m_iHealth(), 0, 100);
    char szHP[32];
    snprintf(szHP, sizeof(szHP), "[ %d HP ]", iHP);
    ImVec2 hpSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, szHP);
    float cx = (vecMin.x + vecMax.x) * 0.5f;
    
    Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, ImVec2(cx - hpSize.x * 0.5f, vecMin.y - 18.f), 
                  szHP, iHP > 40 ? Color(100, 255, 100, 255) : Color(255, 100, 100, 255), 
                  DRAW_TEXT_DROPSHADOW, Color(0,0,0,200));

    // Weapon
    std::string szWeapon = pPawn->m_strActiveWeaponName();
    if (!szWeapon.empty())
        DrawWeapon(vecMin, ImVec2(vecMax.x, vecMax.y), szWeapon);
}

// -----------------------------------------------------------------------
// Render global map grenades (Smoke, Molotov, HE)
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// Grenade position tracking for trajectory prediction
// -----------------------------------------------------------------------
struct GrenadeTrack_t
{
    std::uintptr_t  m_uEntityPtr = 0;
    Vector          m_vecPrevPos = {};
    Vector          m_vecVelocity = {};
    bool            m_bHasPrev = false;
    int             m_nTicksAlive = 0;
};
static std::vector<GrenadeTrack_t> s_vecGrenadeTracker;

static GrenadeTrack_t* FindOrCreateTracker(std::uintptr_t uEntityPtr)
{
    for (auto& t : s_vecGrenadeTracker)
    {
        if (t.m_uEntityPtr == uEntityPtr)
            return &t;
    }
    // Eski trackerlar uchun limit
    if (s_vecGrenadeTracker.size() > 32)
        s_vecGrenadeTracker.erase(s_vecGrenadeTracker.begin());

    s_vecGrenadeTracker.push_back({ uEntityPtr, {}, {}, false, 0 });
    return &s_vecGrenadeTracker.back();
}

void ESP::RenderGrenades(const std::vector<EntityObject_t>& vecEntities)
{
    if (!CONFIG_GET(bool, g_Variables.m_Misc.m_bGrenadeWarning)) return;

    // Barcha trackerlarni "eskirgan" deb belgilash
    for (auto& t : s_vecGrenadeTracker)
        t.m_nTicksAlive++;

    // 120 tickdan eski trackerlarni o'chirish
    s_vecGrenadeTracker.erase(
        std::remove_if(s_vecGrenadeTracker.begin(), s_vecGrenadeTracker.end(),
            [](const GrenadeTrack_t& t) { return t.m_nTicksAlive > 120; }),
        s_vecGrenadeTracker.end());

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_pEntity == nullptr || obj.m_eType != EEntityType::ENTITY_GRENADE)
            continue;

        bool bIsSmoke = (obj.m_uHashedName == FNV1A::HashConst("C_SmokeGrenadeProjectile") || obj.m_uHashedName == FNV1A::HashConst("smokegrenade_projectile"));
        bool bIsMolotov = (obj.m_uHashedName == FNV1A::HashConst("C_MolotovProjectile") || obj.m_uHashedName == FNV1A::HashConst("molotov_projectile"));
        bool bIsHE = (obj.m_uHashedName == FNV1A::HashConst("C_HEGrenadeProjectile") || obj.m_uHashedName == FNV1A::HashConst("hegrenade_projectile"));
        bool bIsFlash = (obj.m_uHashedName == FNV1A::HashConst("C_FlashbangProjectile") || obj.m_uHashedName == FNV1A::HashConst("flashbang_projectile"));
        bool bIsDecoy = (obj.m_uHashedName == FNV1A::HashConst("C_DecoyProjectile") || obj.m_uHashedName == FNV1A::HashConst("decoy_projectile"));

        if (!bIsSmoke && !bIsMolotov && !bIsHE && !bIsFlash && !bIsDecoy)
            continue;

        static std::uint32_t uGameSceneNodeOffset = 0;
        static std::uint32_t uOriginOffset = 0;
        static bool bResolved = false;
        
        if (!bResolved)
        {
            uGameSceneNodeOffset = SchemaSystem::m_mapSchemaOffsets[FNV1A::Hash("C_BaseEntity->m_pGameSceneNode")];
            uOriginOffset = SchemaSystem::m_mapSchemaOffsets[FNV1A::Hash("CGameSceneNode->m_vecAbsOrigin")];
            bResolved = true;
        }

        if (uGameSceneNodeOffset == 0 || uOriginOffset == 0) continue;

        std::uintptr_t pEntityPtr = reinterpret_cast<std::uintptr_t>(obj.m_pEntity);
        std::uintptr_t uSceneNode = g_Memory.ReadMemory<std::uintptr_t>(pEntityPtr + uGameSceneNodeOffset);
        if (uSceneNode < 0x1000) continue;

        Vector vecOrigin = g_Memory.ReadMemory<Vector>(uSceneNode + uOriginOffset);
        if (!std::isfinite(vecOrigin.x) || !std::isfinite(vecOrigin.y) || !std::isfinite(vecOrigin.z))
            continue;

        // === Tracker: pozitsiyadan velocity hisoblash ===
        GrenadeTrack_t* pTracker = FindOrCreateTracker(pEntityPtr);
        pTracker->m_nTicksAlive = 0; // bu entity hali tirik

        if (pTracker->m_bHasPrev)
        {
            Vector vecDelta;
            vecDelta.x = vecOrigin.x - pTracker->m_vecPrevPos.x;
            vecDelta.y = vecOrigin.y - pTracker->m_vecPrevPos.y;
            vecDelta.z = vecOrigin.z - pTracker->m_vecPrevPos.z;

            // ~60fps atrofida, har frame = ~0.016s
            float flScale = 60.f; // frames -> units/sec
            pTracker->m_vecVelocity.x = vecDelta.x * flScale;
            pTracker->m_vecVelocity.y = vecDelta.y * flScale;
            pTracker->m_vecVelocity.z = vecDelta.z * flScale;
        }
        pTracker->m_vecPrevPos = vecOrigin;
        pTracker->m_bHasPrev = true;

        // Rang va nom
        const char* szName = "GRENADE";
        Color colLine(255, 255, 255, 255);

        if (bIsSmoke)       { szName = "SMOKE";   colLine = Color(100, 180, 255, 255); }
        else if (bIsMolotov){ szName = "MOLOTOV";  colLine = Color(255, 100, 30, 255);  }
        else if (bIsHE)     { szName = "HE";       colLine = Color(255, 180, 0, 255);   }
        else if (bIsFlash)  { szName = "FLASH";    colLine = Color(255, 255, 100, 255);  }
        else if (bIsDecoy)  { szName = "DECOY";    colLine = Color(150, 150, 150, 255);  }

        Vector vecVelocity = pTracker->m_vecVelocity;
        float flSpeed = std::sqrtf(vecVelocity.x * vecVelocity.x + vecVelocity.y * vecVelocity.y + vecVelocity.z * vecVelocity.z);

        // ============================================================
        // TRAEKTORIYA CHIZISH
        // ============================================================
        if (flSpeed > 30.f)
        {
            const float flGravity = 400.f;
            const float flDt = 0.025f;
            const int nMaxSteps = 100;

            Vector vecPos = vecOrigin;
            Vector vecVel = vecVelocity;
            ImVec2 prevScreen;
            bool bPrevValid = false;
            Vector vecLandPos = vecOrigin;

            for (int step = 0; step <= nMaxSteps; step++)
            {
                ImVec2 curScreen;
                bool bCurValid = Draw::WorldToScreen(vecPos, curScreen);

                if (bCurValid && bPrevValid)
                {
                    int alpha = 255 - (step * 2);
                    if (alpha < 50) alpha = 50;
                    Color colSeg(static_cast<int>(colLine.r()), static_cast<int>(colLine.g()), static_cast<int>(colLine.b()), alpha);
                    Draw::AddLine(prevScreen, curScreen, colSeg, 2.5f);
                }

                prevScreen = curScreen;
                bPrevValid = bCurValid;

                vecVel.z -= flGravity * flDt;
                vecPos.x += vecVel.x * flDt;
                vecPos.y += vecVel.y * flDt;
                vecPos.z += vecVel.z * flDt;
                vecLandPos = vecPos;

                if (vecPos.z < vecOrigin.z - 600.f)
                    break;
            }

            // === TUSHISH JOYI ===
            ImVec2 landScreen;
            if (Draw::WorldToScreen(vecLandPos, landScreen))
            {
                Draw::AddCircle(landScreen, 18.f, colLine, 24, DRAW_CIRCLE_NONE, Color(0,0,0,0), 2.5f);
                Draw::AddLine(ImVec2(landScreen.x - 8.f, landScreen.y - 8.f),
                              ImVec2(landScreen.x + 8.f, landScreen.y + 8.f), colLine, 2.5f);
                Draw::AddLine(ImVec2(landScreen.x + 8.f, landScreen.y - 8.f),
                              ImVec2(landScreen.x - 8.f, landScreen.y + 8.f), colLine, 2.5f);

                char szLand[64];
                snprintf(szLand, sizeof(szLand), "%s TUSHADI!", szName);
                Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                    ImVec2(landScreen.x + 22.f, landScreen.y - 6.f),
                    szLand, colLine, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));
            }
        }

        // ============================================================
        // HOZIRGI JOYI
        // ============================================================
        ImVec2 screenPos;
        if (Draw::WorldToScreen(vecOrigin, screenPos))
        {
            Draw::AddRing(vecOrigin, 15.0f, colLine, 32, 0, 2.0f);

            float dist = 0.f;
            C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
            if (pLocalPawn) {
                std::uintptr_t uLocalScene = g_Memory.ReadMemory<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(pLocalPawn) + uGameSceneNodeOffset);
                if (uLocalScene > 0x1000) {
                    Vector vecLocalOrigin = g_Memory.ReadMemory<Vector>(uLocalScene + uOriginOffset);
                    dist = vecLocalOrigin.DistTo(vecOrigin) * 0.0254f;
                }
            }

            char szInfo[64];
            if (dist > 0.f)
                snprintf(szInfo, sizeof(szInfo), "%s [%.0fm]", szName, dist);
            else
                snprintf(szInfo, sizeof(szInfo), "%s", szName);

            ImVec2 textSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, szInfo);
            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                          ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y - 15.f),
                          szInfo, colLine, DRAW_TEXT_DROPSHADOW, Color(0,0,0,200));
        }
    }
}

void ESP::RenderWeapons(const std::vector<EntityObject_t>& vecEntities)
{
    bool bDrawWeapons = CONFIG_GET(bool, g_Variables.m_ESP.m_bDroppedWeapons);
    bool bDrawC4Timer = CONFIG_GET(bool, g_Variables.m_Misc.m_bC4Timer);

    if (!bDrawWeapons && !bDrawC4Timer)
        return;

    static std::uintptr_t uGameSceneNodeOffset = 0;
    static std::uintptr_t uOriginOffset = 0;
    static std::uintptr_t uC4Blow = 0;
    static std::uintptr_t uC4Defused = 0;
    static bool bOffsetsResolved = false;

    if (!bOffsetsResolved) {
        auto mapCpy = SchemaSystem::m_mapSchemaOffsets;
        uGameSceneNodeOffset = mapCpy[FNV1A::Hash("C_BaseEntity->m_pGameSceneNode")];
        uOriginOffset = mapCpy[FNV1A::Hash("CGameSceneNode->m_vecAbsOrigin")];
        uC4Blow = mapCpy[FNV1A::HashConst("C_PlantedC4->m_flC4Blow")];
        uC4Defused = mapCpy[FNV1A::HashConst("C_PlantedC4->m_bBombDefused")];
        bOffsetsResolved = true;
    }

    if (uGameSceneNodeOffset == 0 || uOriginOffset == 0) return;

    float flMaxDist = CONFIG_GET(float, g_Variables.m_ESP.m_flWeaponDistance);

    for (const EntityObject_t& obj : vecEntities)
    {
        if (obj.m_pEntity == nullptr || (obj.m_eType != EEntityType::ENTITY_WEAPON && obj.m_eType != EEntityType::ENTITY_PLANTEDC4))
            continue;

        std::string sSchemaName = obj.m_pEntity->GetSchemaName();
        bool bIsC4 = (sSchemaName == "C_C4" || sSchemaName == "weapon_c4");

        if (obj.m_eType == EEntityType::ENTITY_WEAPON)
        {
            if (!bDrawWeapons && !bIsC4) continue;
            if (bIsC4 && !bDrawWeapons && !bDrawC4Timer) continue;
        }

        std::uintptr_t pEntityPtr = reinterpret_cast<std::uintptr_t>(obj.m_pEntity);
        std::uintptr_t uSceneNode = g_Memory.ReadMemory<std::uintptr_t>(pEntityPtr + uGameSceneNodeOffset);

        if (uSceneNode > 0x1000)
        {
            Vector vecOrigin = g_Memory.ReadMemory<Vector>(uSceneNode + uOriginOffset);
            
            float dist = 0.f;
            C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
            if (pLocalPawn) {
                // Read local origin to check distance
                std::uintptr_t uLocalScene = g_Memory.ReadMemory<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(pLocalPawn) + uGameSceneNodeOffset);
                if (uLocalScene > 0x1000) {
                    Vector vecLocalOrigin = g_Memory.ReadMemory<Vector>(uLocalScene + uOriginOffset);
                    dist = vecLocalOrigin.DistTo(vecOrigin) * 0.0254f; // units to meters
                    if (dist > flMaxDist) continue;
                }
            }

            ImVec2 screenPos;
            if (Draw::WorldToScreen(vecOrigin, screenPos))
            {
                std::string sName = obj.m_pEntity->GetSchemaName();
                if (obj.m_eType == EEntityType::ENTITY_PLANTEDC4) sName = "C4 PLANTED!";

                if (!sName.empty()) {
                    if (sName.find("Weapon") != std::string::npos || sName == "C_DEagle" || sName == "C_AK47" || sName == "C_C4" || obj.m_eType == EEntityType::ENTITY_PLANTEDC4) {
                        if (sName.find("C_Weapon") != std::string::npos) sName = sName.substr(8);
                        else if (sName.find("CWeapon") != std::string::npos) sName = sName.substr(7);
                        else if (sName.find("C_") != std::string::npos) sName = sName.substr(2);

                        std::string sLowerName = sName;
                        std::transform(sLowerName.begin(), sLowerName.end(), sLowerName.begin(), ::tolower);

                        Color colWeapon = (sName == "C4" || obj.m_eType == EEntityType::ENTITY_PLANTEDC4) ? Color(255, 50, 50, 255) : GetWeaponColor(sLowerName);

                        float flBottomY = screenPos.y; // Track bottom of drawn element

                        bool bDrawnIcon = false;
                        if (WeaponIcons::HasIcon(sLowerName))
                        {
                            ImTextureID tex = WeaponIcons::GetIcon(sLowerName);
                            if (tex)
                            {
                                int iTexW = 0, iTexH = 0;
                                WeaponIcons::GetIconSize(sLowerName, iTexW, iTexH);
                                float flAspect = (iTexH > 0) ? (float)iTexW / (float)iTexH : 2.67f;
                                
                                // Dynamic scaling based on distance
                                float flIconW = std::clamp(250.f / std::max(dist, 1.f), 15.f, 45.f);
                                float flIconH = flIconW / flAspect;

                                Draw::AddImage(tex,
                                    ImVec2(screenPos.x - flIconW * 0.5f, screenPos.y),
                                    ImVec2(screenPos.x + flIconW * 0.5f, screenPos.y + flIconH),
                                    colWeapon);
                                
                                flBottomY = screenPos.y + flIconH;
                                bDrawnIcon = true;
                            }
                        }

                        if (!bDrawnIcon)
                        {
                            std::transform(sName.begin(), sName.end(), sName.begin(), ::toupper);
                            std::string strLabel = "[" + sName + "]";

                            // Add C4 timer logic if it's planted C4
                            if (obj.m_eType == EEntityType::ENTITY_PLANTEDC4)
                            {
                                float flC4Blow = g_Memory.ReadMemory<float>(pEntityPtr + uC4Blow);
                                float flCur = g_Interfaces.m_GlobalVars.m_flCurrentTime;
                                float flTimeLeft = flC4Blow - flCur;
                                if (flTimeLeft < 0.f) flTimeLeft = 0.f;
                                
                                char szTimer[64];
                                snprintf(szTimer, sizeof(szTimer), "[C4 PLANTED! - %.1f s]", flTimeLeft);
                                strLabel = szTimer;
                                
                                bool bDefused = g_Memory.ReadMemory<bool>(pEntityPtr + uC4Defused);
                                if (bDefused) {
                                    strLabel = "[C4 DEFUSED!]";
                                    colWeapon = Color(100, 255, 100, 255);
                                }
                                else if (flTimeLeft < 10.0f && flTimeLeft > 0.0f) {
                                    // Flash red/yellow
                                    if ((int)(flCur * 10) % 2 == 0) colWeapon = Color(255, 255, 0, 255);
                                    else colWeapon = Color(255, 0, 0, 255);
                                }
                            }

                            ImVec2 textSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, strLabel.c_str());
                            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize, 
                                          ImVec2(screenPos.x - textSize.x * 0.5f, screenPos.y), 
                                          strLabel, colWeapon, DRAW_TEXT_DROPSHADOW, Color(0,0,0,200));
                            
                            flBottomY = screenPos.y + Fonts::ESP->FontSize;
                        }

                        // Draw distance below
                        if (dist > 0.f)
                        {
                            char szDist[32];
                            snprintf(szDist, sizeof(szDist), "%.0fm", dist);
                            
                            // Make distance text slightly smaller if possible, but keep consistent with font
                            ImVec2 distSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize * 0.85f, FLT_MAX, 0.f, szDist);
                            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize * 0.85f,
                                          ImVec2(screenPos.x - distSize.x * 0.5f, flBottomY + 2.f),
                                          szDist, Color(200, 200, 200, 200), DRAW_TEXT_DROPSHADOW, Color(0,0,0, 150));
                        }
                    }
                }
            }
        }
    }
}

std::vector<ESP::DamageIndicator_t> ESP::g_vecDamageIndicators;

void ESP::AddDamageIndicator(Vector vecPos, int iDamage)
{
    DamageIndicator_t text;
    text.m_vecPos = vecPos;
    // Kichik tasodifiy siljish (overlap bo'lmasligi uchun)
    text.m_vecPos.x += (rand() % 20) - 10.f;
    text.m_vecPos.y += (rand() % 20) - 10.f;
    text.m_vecPos.z += (rand() % 10) - 5.f;
    text.m_iDamage = iDamage;
    text.m_flTimeCreated = (float)ImGui::GetTime(); // Use ImGui time which is safe in RenderThread

    g_vecDamageIndicators.push_back(text);
}

void ESP::RenderDamageIndicators()
{
    if (!CONFIG_GET(bool, g_Variables.m_Misc.m_bDamageIndicator))
        return;

    float flCurrentTime = (float)ImGui::GetTime();
    float flLifetime = 1.5f;

    for (auto it = g_vecDamageIndicators.begin(); it != g_vecDamageIndicators.end(); )
    {
        float flTimeDelta = flCurrentTime - it->m_flTimeCreated;
        if (flTimeDelta > flLifetime)
        {
            it = g_vecDamageIndicators.erase(it);
            continue;
        }

        Vector vecRenderPos = it->m_vecPos;
        vecRenderPos.z += flTimeDelta * 40.0f; // float upwards

        ImVec2 vecScreen;
        if (Draw::WorldToScreen(vecRenderPos, vecScreen))
        {
            float flAlpha = 1.0f;
            if (flTimeDelta > (flLifetime - 0.5f))
            {
                flAlpha = (flLifetime - flTimeDelta) / 0.5f;
            }

            Color baseCol = CONFIG_GET(Color, g_Variables.m_Misc.m_colDamageIndicator);
            Color renderCol = Color((int)(baseCol.rBase() * 255.f), (int)(baseCol.gBase() * 255.f), (int)(baseCol.bBase() * 255.f), (int)(flAlpha * 255.f));
            Color shadowCol = Color(0, 0, 0, (int)(flAlpha * 255.f));

            char buf[32];
            snprintf(buf, sizeof(buf), "-%d", it->m_iDamage);

            // Kattaroq va qalinroq font uchun default emas, balki shunchaki kattalashtiramiz
            float flFontSize = Fonts::Default->FontSize * 1.5f;
            ImVec2 textSize = Fonts::Default->CalcTextSizeA(flFontSize, FLT_MAX, 0.0f, buf);

            Draw::AddText(Fonts::Default, flFontSize, ImVec2(vecScreen.x - textSize.x / 2.f, vecScreen.y - textSize.y / 2.f), buf, renderCol, DRAW_TEXT_OUTLINE, shadowCol);
        }

        ++it;
    }
}
