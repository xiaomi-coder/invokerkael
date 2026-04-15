#pragma once

namespace Radar
{
    // Thread-safe overlay radar rendering (called from RenderThread)
    void Render(const std::vector<EntityObject_t>& vecEntities);

    // Force all enemies to show on CS2's built-in radar
    // by writing m_bSpotted = true (called from TickThread)
    void ForceRadarSpotted(const std::vector<EntityObject_t>& vecEntities);

    // Beeps when aiming near an enemy through walls
    void AuditorySonar(const std::vector<EntityObject_t>& vecEntities);
}
