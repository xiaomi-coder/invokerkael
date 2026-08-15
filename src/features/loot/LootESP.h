#pragma once
#include "../../Includes.h"

// =====================================================================
//  YERDAGI QUROLLAR (LOOT ESP)
//  Mustaqil modul: entity ro'yxatiga tayanmaydi, o'zi skanerlaydi.
// =====================================================================

namespace LootESP
{
    struct Status_t
    {
        int  m_nScanned    = 0;   // skanerlangan entity
        int  m_nFound      = 0;   // topilgan yerdagi qurol
        int  m_nDrawn      = 0;   // ekranga chizilgani
        bool m_bOffsetsOk  = false;
    };
    inline Status_t m_Status;

    void Render();
}
