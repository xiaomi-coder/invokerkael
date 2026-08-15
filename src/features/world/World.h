#pragma once

#include "../../Includes.h"
#include "../../config/Variables.h"
#include "../../valve/Entity.h"

namespace World
{
    // Menyudagi diagnostika uchun holat (nima ishlayapti, nima yo'q)
    struct Status_t
    {
        std::uint32_t m_uFovOffset      = 0U;      // CBasePlayerController->m_iDesiredFOV
        std::uint32_t m_uExposureOffset = 0U;      // C_EnvTonemapController->m_flCustomAutoExposureMin
        bool          m_bTonemapFound   = false;   // tonemap controller topildimi
        bool          m_bFovApplied     = false;   // FOV yozilyaptimi
        int           m_iCurrentFov     = -1;      // o'yindagi joriy qiymat
        float         m_flExposure      = 0.f;     // qo'llangan yorug'lik qiymati
        int           m_nScanned        = 0;       // skanerlangan entity soni
        int           m_nPostVolumes    = 0;       // topilgan post-processing volume
    };
    inline Status_t m_Status;

    void Run(const std::vector<EntityObject_t>& vecEntities);

    // Xaritadagi barcha entity nomlarini exe yonidagi faylga yozadi.
    // Tungi rejim ishlamasa — shu ro'yxat kerak bo'ladi.
    // Qaytaradi: yozilgan fayl yo'li (yoki bo'sh matn).
    std::string DumpEntities();
}
