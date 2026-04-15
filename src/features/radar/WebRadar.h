#pragma once
#include "../../Includes.h"
#include <vector>

namespace WebRadar
{
    void Initialize();
    void Shutdown();
    void SetEntities(const std::vector<EntityObject_t>& vecEntities);
}
