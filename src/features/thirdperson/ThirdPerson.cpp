#include "ThirdPerson.h"

// =====================================================================
//  Uchinchi shaxs kamerasi.
//
//  CS2 da o'yinchining pawn'ida CPlayer_ObserverServices bor. Unga
//  "kuzatuv rejimi" (m_iObserverMode) va "kimni kuzatyapmiz"
//  (m_hObserverTarget) yoziladi. O'zimizni o'zimiz kuzatsak '
//  kamera orqaga chiqadi. Bu faqat klient tomonda ' server uchun biz
//  hamon birinchi shaxsdamiz.
// =====================================================================

namespace ThirdPerson
{
    static bool  s_bToggled     = false;   // tugma bilan yoqilganmi
    static bool  s_bApplied     = false;   // hozir yozilgan holatdami
    static int   s_iOrigMode    = 0;
    static int   s_iOrigTarget  = -1;
    static float s_flOrigDist   = 0.f;
    static bool  s_bOrigSaved   = false;

    static std::uint32_t SchemaOffset(const char* szField, std::uint32_t uFallback)
    {
        auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash(szField));
        const std::uint32_t uValue = (it != SchemaSystem::m_mapSchemaOffsets.end()) ? it->second : 0U;
        return (uValue != 0U) ? uValue : uFallback;
    }

    void Restore()
    {
        s_bToggled = false;
        s_bApplied = false;
        m_Status.m_bActive = false;
    }

    void Run()
    {
        // ---------------- offsetlar ----------------
        static std::uint32_t s_uObserverServices = 0U;
        static std::uint32_t s_uObserverMode     = 0U;
        static std::uint32_t s_uObserverTarget   = 0U;
        static std::uint32_t s_uChaseDistance    = 0U;
        static std::uint32_t s_uPlayerPawn       = 0U;
        static bool          s_bResolved         = false;

        if (!s_bResolved)
        {
            s_bResolved         = true;
            s_uObserverServices = SchemaOffset("C_BasePlayerPawn->m_pObserverServices", 4640);
            s_uObserverMode     = SchemaOffset("CPlayer_ObserverServices->m_iObserverMode", 72);
            s_uObserverTarget   = SchemaOffset("CPlayer_ObserverServices->m_hObserverTarget", 76);
            s_uChaseDistance    = SchemaOffset("CPlayer_ObserverServices->m_flObserverChaseDistance", 88);
            s_uPlayerPawn       = SchemaOffset("CCSPlayerController->m_hPlayerPawn", 2324);
        }

        const bool bFeatureOn = CONFIG_GET(bool, g_Variables.m_ThirdPerson.m_bEnable);

        // ---------------- tugma ----------------
        {
            static bool s_bKeyWasDown = false;
            const int   iKey = CONFIG_GET(int, g_Variables.m_ThirdPerson.m_iKey);
            const bool  bKeyDown = bFeatureOn && iKey != 0 && (GetAsyncKeyState(iKey) & 0x8000) != 0;

            if (bKeyDown && !s_bKeyWasDown)
                s_bToggled = !s_bToggled;

            s_bKeyWasDown = bKeyDown;
        }

        if (!bFeatureOn)
            s_bToggled = false;

        C_CSPlayerPawn*      pLocalPawn  = g_Globals.m_LocalPlayer.m_pPlayerPawn;
        CCSPlayerController* pController = g_Globals.m_LocalPlayer.m_pController;

        if (!pLocalPawn || reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000 ||
            !pController || reinterpret_cast<std::uintptr_t>(pController) < 0x10000)
        {
            m_Status.m_bServicesOk = false;
            m_Status.m_bActive     = false;
            s_bApplied             = false;
            s_bOrigSaved           = false;
            return;
        }

        const std::uintptr_t uServices = g_Memory.ReadMemory<std::uintptr_t>(
            reinterpret_cast<std::uintptr_t>(pLocalPawn) + s_uObserverServices);

        m_Status.m_uOffset     = s_uObserverServices;
        m_Status.m_uServices   = uServices;
        m_Status.m_bServicesOk = (uServices > 0x10000);
        if (!m_Status.m_bServicesOk)
            return;

        m_Status.m_iCurrentMode = g_Memory.ReadMemory<int>(uServices + s_uObserverMode);

        // o'lgan bo'lsak ' tegmaymiz (o'yin o'zi kuzatuv rejimiga o'tadi)
        const bool bAlive = pLocalPawn->IsAlive();

        if (s_bToggled && bAlive)
        {
            if (!s_bOrigSaved)
            {
                s_iOrigMode   = m_Status.m_iCurrentMode;
                s_iOrigTarget = g_Memory.ReadMemory<int>(uServices + s_uObserverTarget);
                s_flOrigDist  = g_Memory.ReadMemory<float>(uServices + s_uChaseDistance);
                s_bOrigSaved  = true;
            }

            const int   iMode = CONFIG_GET(int,   g_Variables.m_ThirdPerson.m_iMode);
            const float flDist = CONFIG_GET(float, g_Variables.m_ThirdPerson.m_flDistance);

            // o'zimizni kuzatamiz
            const int iOwnHandle = g_Memory.ReadMemory<int>(
                reinterpret_cast<std::uintptr_t>(pController) + s_uPlayerPawn);

            if (iOwnHandle != 0 && iOwnHandle != -1)
                g_Memory.WriteMemory<int>(uServices + s_uObserverTarget, iOwnHandle);

            g_Memory.WriteMemory<int>(uServices + s_uObserverMode, iMode);

            if (s_uChaseDistance != 0U)
                g_Memory.WriteMemory<float>(uServices + s_uChaseDistance, flDist);

            s_bApplied         = true;
            m_Status.m_bActive = true;
        }
        else if (s_bApplied)
        {
            // birinchi shaxsga qaytaramiz
            g_Memory.WriteMemory<int>(uServices + s_uObserverMode, s_bOrigSaved ? s_iOrigMode : 0);
            g_Memory.WriteMemory<int>(uServices + s_uObserverTarget, s_bOrigSaved ? s_iOrigTarget : -1);
            if (s_uChaseDistance != 0U && s_bOrigSaved)
                g_Memory.WriteMemory<float>(uServices + s_uChaseDistance, s_flOrigDist);

            s_bApplied         = false;
            s_bOrigSaved       = false;
            m_Status.m_bActive = false;
        }
    }
}
