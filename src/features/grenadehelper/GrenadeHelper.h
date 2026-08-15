#pragma once

// -----------------------------------------------------------------------
// Grenade type enum
// -----------------------------------------------------------------------
enum EGrenadeType : int
{
    GRENADE_SMOKE = 0,
    GRENADE_FLASH,
    GRENADE_MOLOTOV,
    GRENADE_HE,
    GRENADE_MAX
};

// -----------------------------------------------------------------------
// Single grenade lineup entry
// -----------------------------------------------------------------------
struct GrenadeLineup_t
{
    const char*     m_szName;           // e.g. "A Site - CT Smoke"
    const char*     m_szDescription;    // e.g. "Oldindan chiqib smoke tashlang"
    EGrenadeType    m_eType;            // Smoke, Flash, etc.
    Vector          m_vecStandPos;      // Where to stand (world coords)
    QAngle          m_angAimAngle;      // Where to aim (pitch, yaw)
    Vector          m_vecLandPos;       // Where the grenade lands (for 3D marker)
    float           m_flRadius;         // How close the player must be to trigger (units)
    bool            m_bJumpThrow;       // Does it require jump throw?
    bool            m_bRunThrow;        // Does it require running?
};

// -----------------------------------------------------------------------
// Map with its lineups
// -----------------------------------------------------------------------
struct GrenadeMap_t
{
    const char*             m_szMapName;      // e.g. "de_mirage"
    const GrenadeLineup_t*  m_pLineups;
    int                     m_nLineupCount;
};

// -----------------------------------------------------------------------
// Grenade Helper namespace
// -----------------------------------------------------------------------
namespace GrenadeHelper
{
    // Render grenade helpers on screen (call from RenderThread)
    void Render();

    // Get current map lineups (nullptr if no data for current map)
    const GrenadeMap_t* GetCurrentMapData();

    // Lineup databases per map
    extern const GrenadeLineup_t g_MirageLineups[];
    extern const int             g_nMirageLineupCount;

    extern const GrenadeLineup_t g_Dust2Lineups[];
    extern const int             g_nDust2LineupCount;

    extern const GrenadeLineup_t g_InfernoLineups[];
    extern const int             g_nInfernoLineupCount;

    extern const GrenadeLineup_t g_AnubisLineups[];
    extern const int             g_nAnubisLineupCount;

    // All maps
    extern const GrenadeMap_t    g_vecMaps[];
    extern const int             g_nMapCount;

    // Cache
    inline const GrenadeMap_t*   m_pCachedMap = nullptr;
    inline int                   m_nCacheTick = 0;
}
