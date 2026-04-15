#pragma once

namespace ESP
{
    // Main entry – call once per player in RenderThread
    void RenderPlayer(CCSPlayerController* pController, C_CSPlayerPawn* pPawn);
    void RenderGlowInfo(CCSPlayerController* pController, C_CSPlayerPawn* pPawn);
    void RenderGrenades(const std::vector<EntityObject_t>& vecEntities);
    void RenderWeapons(const std::vector<EntityObject_t>& vecEntities);

    // Helpers
    bool GetBoundingBox(C_CSPlayerPawn* pPawn, ImVec2& vecMin, ImVec2& vecMax);
    Color GetPlayerColor(CCSPlayerController* pController, C_CSPlayerPawn* pPawn);

    // Draw sub-routines
    void DrawBox2D(const ImVec2& vecMin, const ImVec2& vecMax, const Color& col);
    void DrawBoxCorner(const ImVec2& vecMin, const ImVec2& vecMax, const Color& col);
    void DrawHealthBar(const ImVec2& vecMin, const ImVec2& vecMax, int iHealth, int iMaxHealth);
    void DrawName(const ImVec2& vecMin, const ImVec2& vecMax, const std::string& szName);
    void DrawWeapon(const ImVec2& vecMin, const ImVec2& vecMax, const std::string& szWeapon);
    void DrawDistance(const ImVec2& vecMin, const ImVec2& vecMax, float flDist);
    void DrawHeadDot(C_CSPlayerPawn* pPawn);
    void DrawSnapline(const ImVec2& vecMin, const ImVec2& vecMax);
    void DrawSkeleton(C_CSPlayerPawn* pPawn, const Color& col);
    void DrawFilledBody(C_CSPlayerPawn* pPawn, const Color& col);
}
