#pragma once

namespace Aimbot
{
    void Run(const std::vector<EntityObject_t>& vecEntities);

    // Returns best target pawn (closest to crosshair within FOV), nullptr if none
    C_CSPlayerPawn* FindBestTarget(const std::vector<EntityObject_t>& vecEntities, float& flBestFOV);

    // Gets the world-space position of the target hitbox
    Vector GetHitboxPosition(C_CSPlayerPawn* pPawn, int iHitbox);

    // Smooth the view angle towards target
    QAngle SmoothAngle(const QAngle& angCurrent, const QAngle& angTarget, float flSmooth);

    // Draw FOV circle on screen
    void DrawFOVCircle();
}
