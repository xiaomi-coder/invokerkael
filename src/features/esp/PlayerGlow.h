#pragma once

namespace PlayerGlow
{
    // Apply native CS2 glow to enemy players (called from TickThread)
    void Run(const std::vector<EntityObject_t>& vecEntities);
}
