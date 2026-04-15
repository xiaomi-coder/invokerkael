#include "World.h"
#include "../../valve/SchemaSystem.h"

namespace World
{
    void Run(const std::vector<EntityObject_t>& vecEntities)
    {
        // --- FOV Changer ---
        if (CONFIG_GET(bool, g_Variables.m_World.m_bFOVChanger))
        {
            C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
            if (pLocalPawn && reinterpret_cast<std::uintptr_t>(pLocalPawn) > 0x1000)
            {
                // We'll read the CBasePlayerController from globals to set FOV securely
                CCSPlayerController* pController = g_Globals.m_LocalPlayer.m_pController;
                if (pController && reinterpret_cast<std::uintptr_t>(pController) > 0x1000)
                {
                    static std::uint32_t uDesiredFOVOffset = 0;
                    static bool bFOVResolved = false;
                    if (!bFOVResolved)
                    {
                        auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash("CBasePlayerController->m_iDesiredFOV"));
                        if (it != SchemaSystem::m_mapSchemaOffsets.end())
                            uDesiredFOVOffset = it->second;
                        bFOVResolved = true;
                    }

                    if (uDesiredFOVOffset > 0)
                    {
                        int targetFov = CONFIG_GET(int, g_Variables.m_World.m_iFOV);
                        int currentFov = g_Memory.ReadMemory<int>(reinterpret_cast<std::uintptr_t>(pController) + uDesiredFOVOffset);
                        if (currentFov != targetFov)
                        {
                            g_Memory.WriteMemory<uint32_t>(reinterpret_cast<std::uintptr_t>(pController) + uDesiredFOVOffset, targetFov);
                        }
                    }
                }
            }
        }

        // --- Night Mode ---
        if (CONFIG_GET(bool, g_Variables.m_World.m_bNightMode))
        {
            // The user views 'slider' as Darkness (0.0=bright, 1.0=pitch black).
            // But AutoExposure is reversed: 1.0=bright, 0.01=pitch black.
            float rawSlider = CONFIG_GET(float, g_Variables.m_World.m_flNightModeValue);
            float nightValue = 1.01f - rawSlider;
            if (nightValue < 0.01f) nightValue = 0.01f;
            
            static std::uint32_t uUseCustomAutoExpMin = 0;
            static std::uint32_t uUseCustomAutoExpMax = 0;
            static std::uint32_t uCustomAutoExpMin = 0;
            static std::uint32_t uCustomAutoExpMax = 0;
            static bool bNightOffsetsResolved = false;

            if (!bNightOffsetsResolved)
            {
                auto mapCpy = SchemaSystem::m_mapSchemaOffsets;
                uUseCustomAutoExpMin = mapCpy[FNV1A::Hash("C_EnvTonemapController->m_bUseCustomAutoExposureMin")];
                uUseCustomAutoExpMax = mapCpy[FNV1A::Hash("C_EnvTonemapController->m_bUseCustomAutoExposureMax")];
                uCustomAutoExpMin = mapCpy[FNV1A::Hash("C_EnvTonemapController->m_flCustomAutoExposureMin")];
                uCustomAutoExpMax = mapCpy[FNV1A::Hash("C_EnvTonemapController->m_flCustomAutoExposureMax")];
                bNightOffsetsResolved = true;
            }

            if (uCustomAutoExpMin > 0)
            {
                for (const auto& ent : vecEntities)
                {
                    // We identify the Tonemap controller via its class name or simple existence of these fields.
                    if (ent.m_pEntity && ent.m_eType == EEntityType::ENTITY_WORLD && ent.m_uHashedName == FNV1A::HashConst("C_EnvTonemapController"))
                    {
                        std::uintptr_t pTonemap = reinterpret_cast<std::uintptr_t>(ent.m_pEntity);
                        g_Memory.WriteMemory<bool>(pTonemap + uUseCustomAutoExpMin, true);
                        g_Memory.WriteMemory<bool>(pTonemap + uUseCustomAutoExpMax, true);
                        g_Memory.WriteMemory<float>(pTonemap + uCustomAutoExpMin, nightValue);
                        g_Memory.WriteMemory<float>(pTonemap + uCustomAutoExpMax, nightValue);
                        break;
                    }
                }
            }
        }
    }
}
