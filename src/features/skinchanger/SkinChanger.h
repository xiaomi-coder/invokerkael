#pragma once

// -----------------------------------------------------------------------
// Skin database entry
// -----------------------------------------------------------------------
struct SkinInfo_t
{
    int         m_nPaintKit;
    const char* m_szName;
};

// -----------------------------------------------------------------------
// Per-weapon config (what skin is selected)
// -----------------------------------------------------------------------
struct WeaponSkinConfig_t
{
    int   m_nPaintKit   = 0;      // 0 = default (no change)
    float m_flWear      = 0.0001f; // Factory New
    int   m_nSeed       = 0;
    int   m_nStatTrak   = -1;     // -1 = disabled
};

// -----------------------------------------------------------------------
// Weapon category for GUI grouping
// -----------------------------------------------------------------------
struct WeaponCategory_t
{
    const char*             m_szName;
    std::uint16_t           m_nDefIndex;
    const SkinInfo_t*       m_pSkins;
    int                     m_nSkinCount;
};

namespace SkinChanger
{
    // Initialize skin database
    void Initialize();

    // Apply skins to the local player's weapons (call from TickThread)
    void Run();

    // Get the currently selected config for a weapon definition index
    WeaponSkinConfig_t& GetWeaponConfig(std::uint16_t nDefIndex);

    // Skin databases per weapon
    extern const SkinInfo_t g_AK47Skins[];
    extern const int        g_nAK47SkinCount;

    extern const SkinInfo_t g_M4A4Skins[];
    extern const int        g_nM4A4SkinCount;

    extern const SkinInfo_t g_M4A1SSkins[];
    extern const int        g_nM4A1SSkinCount;

    extern const SkinInfo_t g_AWPSkins[];
    extern const int        g_nAWPSkinCount;

    extern const SkinInfo_t g_DeagleSkins[];
    extern const int        g_nDeagleSkinCount;

    extern const SkinInfo_t g_GlockSkins[];
    extern const int        g_nGlockSkinCount;

    extern const SkinInfo_t g_USPSSkins[];
    extern const int        g_nUSPSSkinCount;

    extern const SkinInfo_t g_AUGSkins[];
    extern const int        g_nAUGSkinCount;

    extern const SkinInfo_t g_SSG08Skins[];
    extern const int        g_nSSG08SkinCount;

    extern const SkinInfo_t g_FAMASSkins[];
    extern const int        g_nFAMASSkinCount;

    extern const SkinInfo_t g_GALILSkins[];
    extern const int        g_nGALILSkinCount;

    extern const SkinInfo_t g_SG553Skins[];
    extern const int        g_nSG553SkinCount;

    extern const SkinInfo_t g_SCAR20Skins[];
    extern const int        g_nSCAR20SkinCount;

    extern const SkinInfo_t g_G3SG1Skins[];
    extern const int        g_nG3SG1SkinCount;

    extern const SkinInfo_t g_P90Skins[];
    extern const int        g_nP90SkinCount;

    extern const SkinInfo_t g_MP9Skins[];
    extern const int        g_nMP9SkinCount;

    extern const SkinInfo_t g_MAC10Skins[];
    extern const int        g_nMAC10SkinCount;

    extern const SkinInfo_t g_UMP45Skins[];
    extern const int        g_nUMP45SkinCount;

    extern const SkinInfo_t g_MP7Skins[];
    extern const int        g_nMP7SkinCount;

    extern const SkinInfo_t g_FiveSevenSkins[];
    extern const int        g_nFiveSevenSkinCount;

    extern const SkinInfo_t g_TEC9Skins[];
    extern const int        g_nTEC9SkinCount;

    extern const SkinInfo_t g_P250Skins[];
    extern const int        g_nP250SkinCount;

    extern const SkinInfo_t g_CZ75Skins[];
    extern const int        g_nCZ75SkinCount;

    extern const SkinInfo_t g_DualBerettasSkins[];
    extern const int        g_nDualBerettasSkinCount;

    extern const SkinInfo_t g_P2000Skins[];
    extern const int        g_nP2000SkinCount;

    extern const SkinInfo_t g_R8Skins[];
    extern const int        g_nR8SkinCount;

    extern const SkinInfo_t g_KnifeSkins[];
    extern const int        g_nKnifeSkinCount;

    // Weapon list for GUI
    extern WeaponCategory_t g_vecWeaponCategories[];
    extern int              g_nWeaponCategoryCount;

    // Runtime storage: weapon def index -> config
    inline std::unordered_map<std::uint16_t, WeaponSkinConfig_t> m_mapWeaponConfigs;

    // Master enable
    inline bool m_bEnabled = false;
}
