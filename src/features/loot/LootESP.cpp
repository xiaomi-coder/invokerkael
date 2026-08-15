#include "LootESP.h"

// =====================================================================
//  Yerdagi qurollarni o'zi topib chizadi.
//  Eski ESP::RenderWeapons entity ro'yxatining tasnifiga tayanardi;
//  u yerda ba'zi qurol klasslari (C_M4A1, C_WeaponAWP...) e'tibordan
//  chetda qolardi. Bu yerda nom bo'yicha keng qidiruv qilinadi.
// =====================================================================

namespace LootESP
{
    struct Entry_t
    {
        C_BaseEntity* m_pEntity = nullptr;
        std::string   m_strName;      // "ak47", "awp" ...
    };

    static std::vector<Entry_t> s_vecEntries;
    static float                s_flLastScan = 0.f;

    static float NowSeconds()
    {
        return static_cast<float>(GetTickCount64()) / 1000.f;
    }

    // "C_WeaponAWP" / "weapon_awp" / "C_AK47"  ->  "awp" / "ak47"
    static std::string PrettyName(std::string strRaw)
    {
        if (strRaw.rfind("C_Weapon", 0) == 0)       strRaw = strRaw.substr(8);
        else if (strRaw.rfind("C_", 0) == 0)        strRaw = strRaw.substr(2);
        else if (strRaw.rfind("weapon_", 0) == 0)   strRaw = strRaw.substr(7);

        std::transform(strRaw.begin(), strRaw.end(), strRaw.begin(), ::tolower);
        return strRaw;
    }

    // Qurol turiga qarab rang (o'yinchi ESP sidagi bilan bir xil)
    static Color GetWeaponColor(const std::string& w)
    {
        if (w == "ak47" || w == "m4a1" || w == "m4a1_silencer" || w == "m4a4" || w == "aug" ||
            w == "sg556" || w == "famas" || w == "galilar")
            return Color(255, 80, 80, 255);
        if (w == "awp" || w == "ssg08" || w == "scar20" || w == "g3sg1")
            return Color(255, 50, 200, 255);
        if (w == "mp9" || w == "mac10" || w == "mp7" || w == "mp5sd" || w == "ump45" ||
            w == "p90" || w == "bizon")
            return Color(100, 200, 255, 255);
        if (w == "deagle" || w == "elite" || w == "fiveseven" || w == "glock" || w == "hkp2000" ||
            w == "usp_silencer" || w == "p250" || w == "tec9" || w == "cz75a" || w == "revolver")
            return Color(255, 200, 50, 255);
        if (w == "nova" || w == "xm1014" || w == "sawedoff" || w == "mag7")
            return Color(255, 140, 50, 255);
        if (w == "m249" || w == "negev")
            return Color(200, 100, 255, 255);
        if (w == "c4")
            return Color(255, 90, 90, 255);
        return Color(200, 200, 110, 230);
    }

    static bool IsWeaponName(const std::string& strName)
    {
        if (strName.empty()) return false;

        // pichoqlar kerak emas
        if (strName.find("Knife")   != std::string::npos ||
            strName.find("knife")   != std::string::npos ||
            strName.find("Bayonet") != std::string::npos ||
            strName.find("bayonet") != std::string::npos)
            return false;

        // granatalar ham ko'rsatilmaydi (ular uchun alohida feature bor)
        if (strName.find("Projectile") != std::string::npos ||
            strName.find("projectile") != std::string::npos)
            return false;

        return strName.find("C_Weapon") != std::string::npos ||
               strName.find("weapon_")  != std::string::npos ||
               strName == "C_AK47"      || strName == "C_DEagle" ||
               strName == "C_C4"        || strName == "C_M4A1"   ||
               strName.rfind("C_CSWeaponBase", 0) == 0;
    }

    // Entity ro'yxatini qayta skanerlash (sekundiga bir marta yetarli)
    static void Rescan()
    {
        s_vecEntries.clear();
        m_Status.m_nScanned = 0;

        for (int i = 0; i <= 2048; ++i)
        {
            C_BaseEntity* pEntity = C_BaseEntity::GetBaseEntity(i);
            if (!pEntity || reinterpret_cast<std::uintptr_t>(pEntity) < 0x10000)
                continue;

            m_Status.m_nScanned++;

            const std::string strSchema = pEntity->GetSchemaName();
            if (!IsWeaponName(strSchema))
                continue;

            // faqat yerda yotgan qurollar (egasi yo'q)
            if (pEntity->m_hOwnerEntity().IsValid())
                continue;

            Entry_t entry;
            entry.m_pEntity = pEntity;
            entry.m_strName = PrettyName(strSchema);
            s_vecEntries.push_back(entry);

            if (s_vecEntries.size() >= 64)
                break;
        }

        m_Status.m_nFound = static_cast<int>(s_vecEntries.size());
    }

    void Render()
    {
        if (!CONFIG_GET(bool, g_Variables.m_ESP.m_bDroppedWeapons))
            return;

        // ---- offsetlar ----
        static std::uint32_t s_uSceneNode = 0U, s_uOrigin = 0U;
        static bool          s_bResolved  = false;
        if (!s_bResolved)
        {
            s_bResolved = true;
            auto Get = [](const char* szField) -> std::uint32_t
            {
                auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash(szField));
                return (it != SchemaSystem::m_mapSchemaOffsets.end()) ? it->second : 0U;
            };
            s_uSceneNode = Get("C_BaseEntity->m_pGameSceneNode");
            s_uOrigin    = Get("CGameSceneNode->m_vecAbsOrigin");
        }

        m_Status.m_bOffsetsOk = (s_uSceneNode != 0U && s_uOrigin != 0U);
        if (!m_Status.m_bOffsetsOk)
            return;

        C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
        if (!pLocalPawn || reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000)
            return;

        // ---- ro'yxatni yangilash ----
        const float flNow = NowSeconds();
        if (flNow - s_flLastScan > 1.0f)
        {
            s_flLastScan = flNow;
            Rescan();
        }

        if (s_vecEntries.empty())
        {
            m_Status.m_nDrawn = 0;
            return;
        }

        // ---- o'zimizning joyimiz ----
        const std::uintptr_t uLocalNode = g_Memory.ReadMemory<std::uintptr_t>(
            reinterpret_cast<std::uintptr_t>(pLocalPawn) + s_uSceneNode);
        if (uLocalNode < 0x10000)
            return;

        const Vector vecLocal = g_Memory.ReadMemory<Vector>(uLocalNode + s_uOrigin);
        if (!std::isfinite(vecLocal.x))
            return;

        const float flMaxMeters = CONFIG_GET(float, g_Variables.m_ESP.m_flWeaponDistance);
        const float flMaxUnits  = flMaxMeters * 52.49f;   // metr -> unit (CS2)

        int nDrawn = 0;
        for (const Entry_t& entry : s_vecEntries)
        {
            const std::uintptr_t uEntity = reinterpret_cast<std::uintptr_t>(entry.m_pEntity);
            const std::uintptr_t uNode   = g_Memory.ReadMemory<std::uintptr_t>(uEntity + s_uSceneNode);
            if (uNode < 0x10000)
                continue;

            const Vector vecPos = g_Memory.ReadMemory<Vector>(uNode + s_uOrigin);
            if (!std::isfinite(vecPos.x) || vecPos.IsZero())
                continue;

            const float flDist = vecLocal.DistTo(vecPos);
            if (flDist > flMaxUnits)
                continue;

            ImVec2 vecScreen;
            if (!Draw::WorldToScreen(vecPos, vecScreen))
                continue;

            // ---- chizamiz ----
            const Color colWeapon = GetWeaponColor(entry.m_strName);
            float flBelowY = vecScreen.y + 3.f;

            // 1) qurol surati (o'yinchi ESP sidagi kabi)
            int iTexW = 0, iTexH = 0;
            if (WeaponIcons::HasIcon(entry.m_strName) &&
                WeaponIcons::GetIconSize(entry.m_strName, iTexW, iTexH) && iTexH > 0)
            {
                // uzoqlashgani sari kichrayadi
                const float flScale  = ImClamp(1.f - (flDist / flMaxUnits) * 0.55f, 0.45f, 1.f);
                const float flIconW  = 42.f * flScale;
                const float flIconH  = flIconW / (static_cast<float>(iTexW) / static_cast<float>(iTexH));

                Draw::AddImage(WeaponIcons::GetIcon(entry.m_strName),
                    ImVec2(vecScreen.x - flIconW * 0.5f, flBelowY),
                    ImVec2(vecScreen.x + flIconW * 0.5f, flBelowY + flIconH),
                    colWeapon);

                flBelowY += flIconH + 1.f;
            }
            else
            {
                // surat yo'q — nomini yozamiz
                std::string strLabel = entry.m_strName;
                std::transform(strLabel.begin(), strLabel.end(), strLabel.begin(), ::toupper);

                const ImVec2 vecSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize, FLT_MAX, 0.f, strLabel.c_str());
                Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize,
                    ImVec2(vecScreen.x - vecSize.x * 0.5f, flBelowY),
                    strLabel, colWeapon, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 200));

                flBelowY += vecSize.y + 1.f;
            }

            // 2) masofa
            char szDist[16];
            snprintf(szDist, sizeof(szDist), "%.0fm", flDist / 52.49f);
            const ImVec2 vecDistSize = Fonts::ESP->CalcTextSizeA(Fonts::ESP->FontSize * 0.85f, FLT_MAX, 0.f, szDist);
            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize * 0.85f,
                ImVec2(vecScreen.x - vecDistSize.x * 0.5f, flBelowY),
                std::string(szDist), Color(150, 150, 150, 190), DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 180));

            nDrawn++;
        }

        m_Status.m_nDrawn = nDrawn;
    }
}
