#pragma once
#include "../../Includes.h"

// =====================================================================
//  UCHINCHI SHAXS (THIRD PERSON)
//  Observer services orqali kamerani orqaga olib chiqamiz.
//  Faqat bizning klientda ko'rinadi ' server baribir birinchi shaxsda.
// =====================================================================

namespace ThirdPerson
{
    struct Status_t
    {
        bool m_bActive       = false;   // hozir yoqilganmi
        bool m_bServicesOk   = false;   // observer services topildimi
        int  m_iCurrentMode  = -1;      // o'yindagi joriy rejim
        std::uint32_t  m_uOffset   = 0U;   // ishlatilgan offset
        std::uintptr_t m_uServices = 0U;   // o'qilgan pointer
    };
    inline Status_t m_Status;

    void Run();      // TickThread
    void Restore();  // birinchi shaxsga qaytarish
}
