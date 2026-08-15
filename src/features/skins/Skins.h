#pragma once
#include "../../Includes.h"

// =====================================================================
//  SKIN CHANGER  ::  ma'lumot bazasi + qo'llash mantig'i
//  Skinlar ro'yxati va suratlari internetdan olinadi, keyin diskda
//  keshlanadi. Qo'llash mantig'i o'yin xotirasiga yozadi.
// =====================================================================

namespace Skins
{
    enum EKind : int
    {
        KIND_WEAPON = 0,
        KIND_KNIFE,
        KIND_GLOVE,
    };

    enum EImageState : int
    {
        IMG_NONE = 0,   // hali so'ralmagan
        IMG_QUEUED,     // navbatda / yuklanmoqda
        IMG_DECODE,     // baytlar tayyor, tekstura kutilmoqda
        IMG_READY,      // tekstura tayyor
        IMG_FAILED,     // yuklab bo'lmadi
    };

    // ---------------------------------------------------------------
    //  Bitta skin yozuvi
    // ---------------------------------------------------------------
    struct SkinEntry_t
    {
        std::string m_strId;
        std::string m_strName;        // "AK-47 | Redline"
        std::string m_strShort;       // "Redline"
        std::string m_strWeapon;      // "AK-47"
        std::string m_strRarity;      // "Covert"
        std::string m_strImageUrl;

        int   m_nWeaponId = 0;        // item definition index
        int   m_nPaintKit = 0;
        int   m_eKind     = KIND_WEAPON;
        bool  m_bLegacy   = false;
        float m_flMinFloat = 0.f;
        float m_flMaxFloat = 1.f;
        ImU32 m_colRarity  = IM_COL32(138, 138, 154, 255);

        // surat holati
        std::atomic<int> m_nImgState{ IMG_NONE };
        ImTextureID      m_pTexture = nullptr;
        int              m_nTexW = 0, m_nTexH = 0;
    };

    // ---------------------------------------------------------------
    //  Qurol guruhi (chapdagi ro'yxat)
    // ---------------------------------------------------------------
    struct WeaponGroup_t
    {
        int                       m_nDefIndex = 0;
        int                       m_eKind     = KIND_WEAPON;
        std::string               m_strName;
        std::vector<SkinEntry_t*> m_vecSkins;
    };

    // ---------------------------------------------------------------
    //  Tanlangan skin sozlamasi
    // ---------------------------------------------------------------
    struct SkinConfig_t
    {
        int         m_nPaintKit = 0;
        float       m_flWear    = 0.0001f;
        int         m_nSeed     = 0;
        int         m_nStatTrak = -1;      // -1 = o'chirilgan
        bool        m_bLegacy   = false;
        int         m_nDefIndex = 0;       // qaysi qurolga tegishli
        std::string m_strName;             // ko'rsatish uchun
    };

    // ================= MA'LUMOT BAZASI (SkinsDB.cpp) =================
    inline std::deque<SkinEntry_t>   g_dqSkins;      // barcha skinlar
    inline std::vector<WeaponGroup_t> g_vecWeapons;  // qurollar bo'yicha guruhlangan
    inline std::atomic<bool>          g_bReady{ false };
    inline std::atomic<bool>          g_bLoading{ false };
    inline std::string                g_strDbStatus = "kutilmoqda...";

    void Initialize();                      // fonda yuklashni boshlaydi
    void Shutdown();                        // oqimlarni to'xtatadi
    void RequestImage(SkinEntry_t* pSkin);  // suratni navbatga qo'yadi
    void PumpTextures();                    // render oqimida teksturalarni yaratadi

    // ================= QO'LLASH (SkinsApply.cpp) =====================
    inline bool                g_bEnabled     = false;   // umumiy yoqish
    inline bool                g_bSubclass    = false;   // "weapon subclass" (eksperimental)
    inline std::atomic<bool>   g_bForce{ false };        // majburiy qayta qo'llash
    inline std::string         g_strStatus    = "kutilmoqda...";

    inline std::map<int, SkinConfig_t> g_mapWeapons;     // defIndex -> config (pichoq/qo'lqopdan tashqari)
    inline SkinConfig_t                g_KnifeConfig;
    inline int                         g_nKnifeDefIndex = 0;
    inline SkinConfig_t                g_GloveConfig;
    inline int                         g_nGloveDefIndex = 0;
    inline std::mutex                  g_mtxConfig;

    void Run();          // TickThread dan chaqiriladi
    void ClearAll();     // barcha tanlovlarni tozalaydi
    void ForceReapply(); // keyingi tickda hammasini qayta yozadi

    bool IsKnifeDef(int nDefIndex);
    bool IsGloveDef(int nDefIndex);
}
