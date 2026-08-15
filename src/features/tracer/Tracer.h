#pragma once
#include "../../Includes.h"

// =====================================================================
//  O'Q IZI (BULLET TRACER)
//  Otganingizda qurol og'zidan chiqadigan chiziq / chaqmoq effekti.
//  Otish m_iShotsFired o'zgarishi orqali aniqlanadi, yo'nalish esa
//  qarash burchagidan hisoblanadi.
// =====================================================================

namespace Tracer
{
    enum EStyle : int
    {
        STYLE_LINE = 0,   // oddiy chiziq
        STYLE_BOLT,       // chaqmoq
        STYLE_BEAM,       // yorug' nur
        STYLE_SMOKE,      // haqiqiy: ingichka tutun ipi, pastga cho'kadi
    };

    // TickThread: otishni aniqlaydi
    void Run();

    // RenderThread: izlarni chizadi
    void Render();

    // Xarita almashganda / o'lganda tozalash
    void Clear();
}
