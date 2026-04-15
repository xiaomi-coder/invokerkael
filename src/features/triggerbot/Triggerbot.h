#pragma once

namespace Triggerbot
{
    void Run(const std::vector<EntityObject_t>& vecEntities);

    // Returns true if an enemy pawn is under our crosshair
    bool IsEnemyUnderCrosshair(const std::vector<EntityObject_t>& vecEntities);
}
