#include "World.h"
#include "../../valve/SchemaSystem.h"

namespace World
{
    // Post-processing volume'lar — tonemap topilmasa shular ishlatiladi
    static std::vector<C_BaseEntity*> s_vecPostVolumes;

    // -------------------------------------------------------------------
    //  Tonemap controller(lar)ni topish.
    //  CS2 da klass nomi "C_TonemapController2" (eski nomi C_EnvTonemapController).
    //  Xaritada bittadan ko'p bo'lishi mumkin — hammasiga yozamiz.
    // -------------------------------------------------------------------
    static const std::vector<C_BaseEntity*>& GetTonemaps(const std::vector<EntityObject_t>& vecEntities)
    {
        static std::vector<C_BaseEntity*> s_vecTonemaps;
        static int  s_nTick        = 0;
        static bool s_bScannedOnce = false;

        // eski (xarita almashgan) pointerlarga yozib qo'ymaslik uchun
        // har tick birinchi entity'ni tekshiramiz — arzon
        bool bStale = false;
        if (!s_vecTonemaps.empty())
        {
            const std::string strFirst = s_vecTonemaps.front()->GetSchemaName();
            bStale = (strFirst.find("Tonemap") == std::string::npos &&
                      strFirst.find("tonemap") == std::string::npos);
        }

        const bool bRescan = !s_bScannedOnce || bStale || (++s_nTick % 128) == 0;
        if (!bRescan)
            return s_vecTonemaps;

        s_bScannedOnce = true;
        s_vecTonemaps.clear();

        // entity list allaqachon topgan bo'lsa — o'shani olamiz
        for (const EntityObject_t& object : vecEntities)
        {
            if (object.m_pEntity && object.m_eType == EEntityType::ENTITY_WORLD)
                s_vecTonemaps.push_back(object.m_pEntity);
        }

        if (!s_vecTonemaps.empty())
            return s_vecTonemaps;

        // entity list tanimadi — nom bo'yicha o'zimiz qidiramiz.
        // CS2 da klass nomi: C_TonemapController2 / env_tonemap_controller2,
        // ba'zi xaritalarda esa faqat C_PostProcessingVolume bo'ladi.
        m_Status.m_nScanned    = 0;
        m_Status.m_nPostVolumes = 0;
        s_vecPostVolumes.clear();

        for (int i = 0; i <= 4096; ++i)
        {
            C_BaseEntity* pEntity = C_BaseEntity::GetBaseEntity(i);
            if (!pEntity || reinterpret_cast<std::uintptr_t>(pEntity) < 0x10000)
                continue;

            m_Status.m_nScanned++;

            const std::string strName = pEntity->GetSchemaName();
            if (strName.empty())
                continue;

            if (strName.find("Tonemap") != std::string::npos ||
                strName.find("tonemap") != std::string::npos)
            {
                if (s_vecTonemaps.size() < 8)
                    s_vecTonemaps.push_back(pEntity);
            }
            else if (strName.find("PostProcess")  != std::string::npos ||
                     strName.find("postprocess")  != std::string::npos ||
                     strName.find("post_process") != std::string::npos)
            {
                if (s_vecPostVolumes.size() < 8)
                    s_vecPostVolumes.push_back(pEntity);
            }
        }

        m_Status.m_nPostVolumes = static_cast<int>(s_vecPostVolumes.size());
        return s_vecTonemaps;
    }

    std::string DumpEntities()
    {
        std::map<std::string, int> mapNames;
        int nTotal = 0;

        for (int i = 0; i <= 8192; ++i)
        {
            C_BaseEntity* pEntity = C_BaseEntity::GetBaseEntity(i);
            if (!pEntity || reinterpret_cast<std::uintptr_t>(pEntity) < 0x10000)
                continue;

            const std::string strName = pEntity->GetSchemaName();
            if (strName.empty())
                continue;

            mapNames[strName]++;
            nTotal++;
        }

        char szExe[MAX_PATH];
        if (!GetModuleFileNameA(NULL, szExe, MAX_PATH))
            return "";

        std::string strPath(szExe);
        const size_t uPos = strPath.find_last_of("\\/");
        if (uPos != std::string::npos)
            strPath = strPath.substr(0, uPos + 1);
        strPath += "kael_entities.txt";

        std::ofstream file(strPath, std::ios::trunc);
        if (!file)
            return "";

        file << "Jami entity: " << nTotal << ", turli nomlar: " << mapNames.size() << "\n\n";
        for (const auto& entry : mapNames)
            file << entry.second << " x  " << entry.first << "\n";

        return strPath;
    }

    void Run(const std::vector<EntityObject_t>& vecEntities)
    {
        // ---------------- schema offsetlari (bir marta) ----------------
        static std::uint32_t s_uDesiredFOV = 0U;
        // yangi CS2 klassi
        static std::uint32_t s_uAutoExpMin = 0U;
        static std::uint32_t s_uAutoExpMax = 0U;
        // eski klass (zaxira)
        static std::uint32_t s_uUseCustomMin = 0U;
        static std::uint32_t s_uUseCustomMax = 0U;
        static std::uint32_t s_uCustomMin    = 0U;
        static std::uint32_t s_uCustomMax    = 0U;
        // post-processing volume
        static std::uint32_t s_uPostExposureCtl = 0U;
        static std::uint32_t s_uPostMinExposure = 0U;
        static std::uint32_t s_uPostMaxExposure = 0U;
        static std::uint32_t s_uPostMaster      = 0U;
        static std::uint32_t s_uPostFade        = 0U;
        static std::uint32_t s_uPostComp        = 0U;
        static bool          s_bResolved     = false;

        if (!s_bResolved)
        {
            s_bResolved = true;
            auto Get = [](const char* szField) -> std::uint32_t
            {
                auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash(szField));
                return (it != SchemaSystem::m_mapSchemaOffsets.end()) ? it->second : 0U;
            };

            s_uDesiredFOV   = Get("CBasePlayerController->m_iDesiredFOV");

            s_uAutoExpMin   = Get("C_TonemapController2->m_flAutoExposureMin");
            s_uAutoExpMax   = Get("C_TonemapController2->m_flAutoExposureMax");

            s_uPostExposureCtl = Get("C_PostProcessingVolume->m_bExposureControl");
            s_uPostMinExposure = Get("C_PostProcessingVolume->m_flMinExposure");
            s_uPostMaxExposure = Get("C_PostProcessingVolume->m_flMaxExposure");
            s_uPostMaster      = Get("C_PostProcessingVolume->m_bMaster");
            s_uPostFade        = Get("C_PostProcessingVolume->m_flFadeDuration");
            s_uPostComp        = Get("C_PostProcessingVolume->m_flExposureCompensation");

            s_uUseCustomMin = Get("C_EnvTonemapController->m_bUseCustomAutoExposureMin");
            s_uUseCustomMax = Get("C_EnvTonemapController->m_bUseCustomAutoExposureMax");
            s_uCustomMin    = Get("C_EnvTonemapController->m_flCustomAutoExposureMin");
            s_uCustomMax    = Get("C_EnvTonemapController->m_flCustomAutoExposureMax");

            m_Status.m_uFovOffset      = s_uDesiredFOV;
            m_Status.m_uExposureOffset = (s_uAutoExpMin != 0U) ? s_uAutoExpMin : s_uCustomMin;
        }

        // ==================== FOV CHANGER ====================
        {
            static bool s_bFovApplied  = false;
            static int  s_iOriginalFov = -1;

            const bool bEnabled = CONFIG_GET(bool, g_Variables.m_World.m_bFOVChanger);

            CCSPlayerController* pController = g_Globals.m_LocalPlayer.m_pController;
            if (s_uDesiredFOV != 0U && pController && reinterpret_cast<std::uintptr_t>(pController) > 0x10000)
            {
                const std::uintptr_t uAddress = reinterpret_cast<std::uintptr_t>(pController) + s_uDesiredFOV;
                const int iCurrent = g_Memory.ReadMemory<int>(uAddress);
                m_Status.m_iCurrentFov = iCurrent;

                if (bEnabled)
                {
                    if (!s_bFovApplied)
                    {
                        s_iOriginalFov = (iCurrent >= 0 && iCurrent <= 180) ? iCurrent : 90;
                        s_bFovApplied  = true;
                    }

                    const int iTarget = CONFIG_GET(int, g_Variables.m_World.m_iFOV);
                    if (iCurrent != iTarget)
                        g_Memory.WriteMemory<int>(uAddress, iTarget);

                    m_Status.m_bFovApplied = true;
                }
                else if (s_bFovApplied)
                {
                    g_Memory.WriteMemory<int>(uAddress, (s_iOriginalFov > 0) ? s_iOriginalFov : 90);
                    s_bFovApplied          = false;
                    m_Status.m_bFovApplied = false;
                }
            }
            else if (!bEnabled)
            {
                m_Status.m_bFovApplied = false;
            }
        }

        // ==================== NIGHT MODE ====================
        {
            static bool  s_bNightApplied = false;
            static float s_flOrigMin     = 0.f;
            static float s_flOrigMax     = 0.f;
            static bool  s_bOrigSaved    = false;

            const bool bEnabled = CONFIG_GET(bool, g_Variables.m_World.m_bNightMode);

            if (!bEnabled && !s_bNightApplied)
                return;

            const std::vector<C_BaseEntity*>& vecTonemaps = GetTonemaps(vecEntities);
            m_Status.m_bTonemapFound = !vecTonemaps.empty();

            // slayder: 0.0 = yorug', 1.0 = qop-qorong'i
            float flRaw   = CONFIG_GET(float, g_Variables.m_World.m_flNightModeValue);
            float flValue = 1.01f - flRaw;
            if (flValue < 0.01f) flValue = 0.01f;

            // ---- tonemap yo'q bo'lsa: post-processing volume orqali ----
            if (vecTonemaps.empty())
            {
                if (s_uPostMinExposure == 0U || s_vecPostVolumes.empty())
                    return;

                for (C_BaseEntity* pVolume : s_vecPostVolumes)
                {
                    if (!pVolume || reinterpret_cast<std::uintptr_t>(pVolume) < 0x10000)
                        continue;

                    const std::uintptr_t uVolume = reinterpret_cast<std::uintptr_t>(pVolume);

                    if (s_uPostExposureCtl != 0U)
                        g_Memory.WriteMemory<bool>(uVolume + s_uPostExposureCtl, bEnabled);
                    if (s_uPostMaster != 0U && bEnabled)
                        g_Memory.WriteMemory<bool>(uVolume + s_uPostMaster, true);

                    if (bEnabled)
                    {
                        // darhol qo'llanishi uchun fade'ni nolga tushiramiz
                        if (s_uPostFade != 0U)
                            g_Memory.WriteMemory<float>(uVolume + s_uPostFade, 0.f);

                        g_Memory.WriteMemory<float>(uVolume + s_uPostMinExposure, flValue);
                        if (s_uPostMaxExposure != 0U)
                            g_Memory.WriteMemory<float>(uVolume + s_uPostMaxExposure, flValue);

                        // qo'shimcha qorong'ulashtirish
                        if (s_uPostComp != 0U)
                            g_Memory.WriteMemory<float>(uVolume + s_uPostComp, -2.f * (1.f - flValue));
                    }
                    else
                    {
                        if (s_uPostComp != 0U)
                            g_Memory.WriteMemory<float>(uVolume + s_uPostComp, 0.f);
                    }
                }

                m_Status.m_flExposure = bEnabled ? flValue : 0.f;
                s_bNightApplied       = bEnabled;
                return;
            }

            for (C_BaseEntity* pTonemap : vecTonemaps)
            {
                if (!pTonemap || reinterpret_cast<std::uintptr_t>(pTonemap) < 0x10000)
                    continue;

                const std::uintptr_t uTonemap = reinterpret_cast<std::uintptr_t>(pTonemap);

                // ---- yangi CS2 klassi: C_TonemapController2 ----
                if (s_uAutoExpMin != 0U && s_uAutoExpMax != 0U)
                {
                    if (bEnabled)
                    {
                        if (!s_bOrigSaved)
                        {
                            s_flOrigMin  = g_Memory.ReadMemory<float>(uTonemap + s_uAutoExpMin);
                            s_flOrigMax  = g_Memory.ReadMemory<float>(uTonemap + s_uAutoExpMax);
                            if (!std::isfinite(s_flOrigMin) || s_flOrigMin <= 0.f) s_flOrigMin = 0.2f;
                            if (!std::isfinite(s_flOrigMax) || s_flOrigMax <= 0.f) s_flOrigMax = 2.0f;
                            s_bOrigSaved = true;
                        }

                        g_Memory.WriteMemory<float>(uTonemap + s_uAutoExpMin, flValue);
                        g_Memory.WriteMemory<float>(uTonemap + s_uAutoExpMax, flValue);
                    }
                    else
                    {
                        g_Memory.WriteMemory<float>(uTonemap + s_uAutoExpMin, s_bOrigSaved ? s_flOrigMin : 0.2f);
                        g_Memory.WriteMemory<float>(uTonemap + s_uAutoExpMax, s_bOrigSaved ? s_flOrigMax : 2.0f);
                    }
                }

                // ---- eski klass (agar mavjud bo'lsa) ----
                if (s_uCustomMin != 0U && s_uUseCustomMin != 0U)
                {
                    g_Memory.WriteMemory<bool>(uTonemap + s_uUseCustomMin, bEnabled);
                    if (s_uUseCustomMax != 0U) g_Memory.WriteMemory<bool>(uTonemap + s_uUseCustomMax, bEnabled);

                    if (bEnabled)
                    {
                        g_Memory.WriteMemory<float>(uTonemap + s_uCustomMin, flValue);
                        if (s_uCustomMax != 0U) g_Memory.WriteMemory<float>(uTonemap + s_uCustomMax, flValue);
                    }
                }
            }

            m_Status.m_flExposure = bEnabled ? flValue : 0.f;
            s_bNightApplied       = bEnabled;
            if (!bEnabled)
                s_bOrigSaved = false;
        }
    }
}
