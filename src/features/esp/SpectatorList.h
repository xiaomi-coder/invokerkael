#pragma once

namespace SpectatorList
{
    // Render the spectator list overlay
    // Call from RenderThread
    void Render(const std::vector<EntityObject_t>& vecEntities);
}
