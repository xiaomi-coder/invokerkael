#include "Tracer.h"

// =====================================================================
//  O'Q IZI (BULLET TRACER)
//
//  Haqiqiy o'q izi qanday ko'rinadi:
//    1. Qurol og'zidan chiqadi, lekin bir zumda o'qning haqiqiy
//       yo'nalishiga "yopishadi" — shuning uchun boshi egilgan bo'ladi.
//    2. Butun yo'l emas — kalta yorqin chiziqcha uchadi: oldida oq-issiq
//       bosh, orqasida so'nuvchi dum.
//    3. Devorga tegib to'xtaydi — xarita geometriyasi bo'yicha.
//    4. Ortidan bir necha o'ndan bir soniya turadigan tutun izi qoladi.
//    5. Har o'q biroz tarqoq uchadi — ikkitasi bir xil bo'lmaydi.
// =====================================================================

namespace Tracer
{
    struct Shot_t
    {
        Vector        m_vecMuzzle{ 0.f, 0.f, 0.f }; // qurol og'zi (viewmodel suyagi)
        bool          m_bHasMuzzle = false;
        Vector        m_vecNear{ 0.f, 0.f, 0.f };   // ko'zdan biroz oldinda (yo'l boshi)
        Vector        m_vecEnd{ 0.f, 0.f, 0.f };    // o'q tekkan joy
        float         m_flSpawnTime = 0.f;
        std::uint32_t m_uSeed       = 0U;
    };

    static std::vector<Shot_t> s_vecShots;
    static std::mutex          s_mtxShots;
    static const size_t        kMaxShots = 48;

    // Yo'l boshi ko'zdan shuncha unit oldinda olinadi
    static const float kNearDist = 40.f;

    // Color konstruktorining noaniqligini oldini olish uchun
    static Color RGBA(int r, int g, int b, int a)
    {
        return Color(static_cast<std::uint8_t>(ImClamp(r, 0, 255)),
                     static_cast<std::uint8_t>(ImClamp(g, 0, 255)),
                     static_cast<std::uint8_t>(ImClamp(b, 0, 255)),
                     static_cast<std::uint8_t>(ImClamp(a, 0, 255)));
    }

    static Color FadeCol(const Color& col, float flAlpha)
    {
        return RGBA(col.r(), col.g(), col.b(), static_cast<int>(col.a() * ImClamp(flAlpha, 0.f, 1.f)));
    }

    static float NowSeconds()
    {
        return static_cast<float>(GetTickCount64()) / 1000.f;
    }

    // ---------------------------------------------------------------
    //  Kamera yo'nalishi — view matritsasidan (qarash burchagi offseti
    //  bu bazada eskirgan, matritsa esa har kadr to'g'ri keladi).
    // ---------------------------------------------------------------
    static bool CameraVectors(Vector& vecForward, Vector& vecRight, Vector& vecUp)
    {
        const ViewMatrix_t& mat = g_Globals.m_matViewMatrix;

        vecForward = Vector(mat[3][0], mat[3][1], mat[3][2]);
        if (!std::isfinite(vecForward.x) || vecForward.Length() < 1e-6f)
            return false;

        vecForward.Normalize();

        vecRight = Vector(vecForward.y, -vecForward.x, 0.f);
        if (vecRight.Length() < 1e-4f)
            vecRight = Vector(1.f, 0.f, 0.f);
        vecRight.Normalize();

        vecUp = Vector(
            vecRight.y * vecForward.z - vecRight.z * vecForward.y,
            vecRight.z * vecForward.x - vecRight.x * vecForward.z,
            vecRight.x * vecForward.y - vecRight.y * vecForward.x);
        vecUp.Normalize();
        return true;
    }

    // ---------------------------------------------------------------
    //  QUROL OG'ZI — avtomatik.
    //  Qo'ldagi qurol modeli (viewmodel) suyaklarini o'qib, kameradan
    //  eng oldinda turgan suyakni olamiz — bu deyarli har doim
    //  stvol uchi bo'ladi. Shu sababli har bir qurolga alohida sozlash
    //  kerak emas: AWP uzun, pistolet kalta — o'zi to'g'ri chiqadi.
    // ---------------------------------------------------------------
    static bool GetMuzzleWorld(C_CSPlayerPawn* pPawn, const Vector& vecEye,
                               const Vector& vecForward, Vector& vecOut)
    {
        if (!pPawn) return false;

        CCSPlayer_ViewModelServices* pServices = pPawn->m_pViewModelServices();
        if (!pServices || reinterpret_cast<std::uintptr_t>(pServices) < 0x10000)
            return false;

        C_BaseModelEntity* pViewModel = pServices->m_hViewModel().Get();
        if (!pViewModel || reinterpret_cast<std::uintptr_t>(pViewModel) < 0x10000)
            return false;

        CGameSceneNode* pNode = pViewModel->m_pGameSceneNode();
        if (!pNode || reinterpret_cast<std::uintptr_t>(pNode) < 0x10000)
            return false;

        BoneData_t* pBones = pNode->m_pBoneCache();
        if (!pBones || reinterpret_cast<std::uintptr_t>(pBones) < 0x10000)
            return false;

        float  flBest  = -1.f;
        Vector vecBest{ 0.f, 0.f, 0.f };
        bool   bFound  = false;

        for (int i = 0; i < 48; i++)
        {
            const BoneData_t bone = g_Memory.ReadMemory<BoneData_t>(
                reinterpret_cast<std::uintptr_t>(pBones) + static_cast<std::uintptr_t>(i) * sizeof(BoneData_t));

            const Vector& vecPos = bone.m_vecPosition;
            if (!std::isfinite(vecPos.x) || vecPos.IsZero())
                continue;

            const Vector vecDelta = vecPos - vecEye;
            const float  flLen    = vecDelta.Length();
            if (flLen < 2.f || flLen > 150.f)   // viewmodel ko'zga yaqin turadi
                continue;

            const float flFwd = vecDelta.x * vecForward.x
                              + vecDelta.y * vecForward.y
                              + vecDelta.z * vecForward.z;

            if (flFwd > flBest)
            {
                flBest  = flFwd;
                vecBest = vecPos;
                bFound  = true;
            }
        }

        if (!bFound || flBest < 4.f)
            return false;

        vecOut = vecBest;
        return true;
    }

    // ---------------------------------------------------------------
    //  Qurol EKRANDA qayerda ko'rinadi?
    //
    //  Viewmodel dunyoda ko'zdan atigi ~25 unit narida turadi, shuning
    //  uchun uni oddiy proyeksiya bilan chizsak — nishon markazida
    //  chiqadi. O'yin esa uni ALOHIDA FOV bilan chizadi (viewmodel_fov),
    //  shuning uchun qurol ekranning o'ng-pastida ko'rinadi.
    //  Shu FOV bilan qayta hisoblasak — stvol uchi aynan o'z joyida
    //  chiqadi va hech narsani qo'lda sozlash kerak bo'lmaydi.
    // ---------------------------------------------------------------
    static bool ApparentMuzzleScreen(ImVec2& vecOut)
    {
        C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
        if (!pLocalPawn || reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000)
            return false;

        Vector vecForward, vecRight, vecUp;
        if (!CameraVectors(vecForward, vecRight, vecUp))
            return false;

        const Vector vecEye = pLocalPawn->GetEyePosition();
        Vector vecBone;
        if (!GetMuzzleWorld(pLocalPawn, vecEye, vecForward, vecBone))
            return false;

        const Vector vecDelta = vecBone - vecEye;

        const float flFwd = vecDelta.x * vecForward.x + vecDelta.y * vecForward.y + vecDelta.z * vecForward.z;
        if (flFwd < 1.f)
            return false;

        const float flRight = vecDelta.x * vecRight.x + vecDelta.y * vecRight.y + vecDelta.z * vecRight.z;
        const float flUp    = vecDelta.x * vecUp.x    + vecDelta.y * vecUp.y    + vecDelta.z * vecUp.z;

        const float flW = static_cast<float>(Window::m_iWidth);
        const float flH = static_cast<float>(Window::m_iHeight);
        const float flAspect = (flH > 1.f) ? (flW / flH) : 1.78f;

        const float flVmFov   = ImClamp(CONFIG_GET(float, g_Variables.m_Tracer.m_flViewmodelFov), 20.f, 120.f);
        const float flTanHorz = tanf(M_DEG2RAD(flVmFov * 0.5f));
        const float flTanVert = flTanHorz / flAspect;

        vecOut.x = flW * 0.5f * (1.f + (flRight / flFwd) / flTanHorz);
        vecOut.y = flH * 0.5f * (1.f - (flUp    / flFwd) / flTanVert);

        // ekrandan chiqib ketmasin
        if (!std::isfinite(vecOut.x) || !std::isfinite(vecOut.y)) return false;
        if (vecOut.x < -flW || vecOut.x > flW * 2.f) return false;
        if (vecOut.y < -flH || vecOut.y > flH * 2.f) return false;
        return true;
    }

    // Barqaror tasodifiy son (bir xil o'q — bir xil shakl)
    static float Noise(std::uint32_t uSeed, int nIndex, int nFrame)
    {
        std::uint32_t h = uSeed * 747796405u + static_cast<std::uint32_t>(nIndex) * 2891336453u
                        + static_cast<std::uint32_t>(nFrame) * 668265263u;
        h ^= h >> 15; h *= 2246822519u;
        h ^= h >> 13; h *= 3266489917u;
        h ^= h >> 16;
        return (static_cast<float>(h & 0xFFFF) / 32767.5f) - 1.f;   // -1 .. 1
    }

    // =================================================================
    //  TICK — otishni aniqlash
    // =================================================================
    void Run()
    {
        if (!CONFIG_GET(bool, g_Variables.m_Tracer.m_bEnable))
            return;

        C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
        if (!pLocalPawn || reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000 || !pLocalPawn->IsAlive())
            return;

        static int            s_nLastShots = 0;
        static std::uintptr_t s_uLastPawn  = 0U;

        const std::uintptr_t uPawn  = reinterpret_cast<std::uintptr_t>(pLocalPawn);
        const int            nShots = pLocalPawn->m_iShotsFired();

        if (uPawn != s_uLastPawn)
        {
            s_uLastPawn  = uPawn;
            s_nLastShots = nShots;
            return;
        }

        if (nShots <= s_nLastShots)
        {
            s_nLastShots = nShots;
            return;
        }

        s_nLastShots = nShots;

        // --------- yo'nalish ---------
        Vector vecForward, vecRight, vecUp;
        if (!CameraVectors(vecForward, vecRight, vecUp))
            return;

        const Vector vecEye = pLocalPawn->GetEyePosition();
        if (!std::isfinite(vecEye.x))
            return;

        // --------- tarqoqlik: har o'q biroz boshqacha uchadi ---------
        const std::uint32_t uSeed = static_cast<std::uint32_t>(GetTickCount64())
                                  ^ static_cast<std::uint32_t>(nShots * 2654435761u);

        const float flSpread = CONFIG_GET(float, g_Variables.m_Tracer.m_flSpread);
        if (flSpread > 0.001f)
        {
            const float flRad = M_DEG2RAD(flSpread);
            vecForward = vecForward
                + vecRight * (Noise(uSeed, 1, 0) * flRad)
                + vecUp    * (Noise(uSeed, 2, 0) * flRad);
            vecForward.Normalize();
        }

        const float flLength = CONFIG_GET(float, g_Variables.m_Tracer.m_flLength);

        // --------- o'q qayerga tegdi? ---------
        // Xarita geometriyasi bo'yicha ikkilik qidiruv: iz devorda to'xtaydi.
        float flHitDist = flLength;
        if (g_MapParser.m_bSetup)
        {
            const Vector vecFar = vecEye + vecForward * flLength;
            if (!g_MapParser.IsVisible(vecEye, vecFar))
            {
                float flLo = 0.f, flHi = flLength;
                for (int i = 0; i < 14; i++)
                {
                    const float flMid = (flLo + flHi) * 0.5f;
                    if (g_MapParser.IsVisible(vecEye, vecEye + vecForward * flMid))
                        flLo = flMid;
                    else
                        flHi = flMid;
                }
                flHitDist = ImMax(kNearDist + 8.f, flHi);
            }
        }

        Shot_t shot;

        // qurol og'zi — avtomatik (viewmodel suyagi)
        if (CONFIG_GET(bool, g_Variables.m_Tracer.m_bAutoMuzzle))
            shot.m_bHasMuzzle = GetMuzzleWorld(pLocalPawn, vecEye, vecForward, shot.m_vecMuzzle);

        shot.m_vecNear     = vecEye + vecForward * kNearDist;
        shot.m_vecEnd      = vecEye + vecForward * flHitDist;
        shot.m_flSpawnTime = NowSeconds();
        shot.m_uSeed       = uSeed;

        std::lock_guard<std::mutex> lock(s_mtxShots);
        if (s_vecShots.size() >= kMaxShots)
            s_vecShots.erase(s_vecShots.begin());
        s_vecShots.push_back(shot);
    }

    // =================================================================
    //  RENDER
    // =================================================================
    void Render()
    {
        if (!CONFIG_GET(bool, g_Variables.m_Tracer.m_bEnable))
            return;

        // ---- Menyu ochiq bo'lsa: qurol og'zi qayerdaligini ko'rsatamiz ----
        // Foydalanuvchi X/Y slayderlarini surib, belgini quroli og'ziga
        // aniq moslashi uchun kerak.
        if (Gui::m_bOpen && !CONFIG_GET(bool, g_Variables.m_Tracer.m_bAutoMuzzle))
        {
            const ImVec2 vecMark(
                static_cast<float>(Window::m_iWidth)  * CONFIG_GET(float, g_Variables.m_Tracer.m_flMuzzleX),
                static_cast<float>(Window::m_iHeight) * CONFIG_GET(float, g_Variables.m_Tracer.m_flMuzzleY));

            const Color colMark = RGBA(255, 214, 140, 230);

            Draw::AddCircle(vecMark, 9.f, colMark, 16, DRAW_CIRCLE_OUTLINE, Color(0, 0, 0, 200), 1.6f);
            Draw::AddLine(ImVec2(vecMark.x - 16.f, vecMark.y), ImVec2(vecMark.x - 4.f, vecMark.y), colMark, 1.6f);
            Draw::AddLine(ImVec2(vecMark.x + 4.f, vecMark.y), ImVec2(vecMark.x + 16.f, vecMark.y), colMark, 1.6f);
            Draw::AddLine(ImVec2(vecMark.x, vecMark.y - 16.f), ImVec2(vecMark.x, vecMark.y - 4.f), colMark, 1.6f);
            Draw::AddLine(ImVec2(vecMark.x, vecMark.y + 4.f), ImVec2(vecMark.x, vecMark.y + 16.f), colMark, 1.6f);

            Draw::AddText(Fonts::ESP, Fonts::ESP->FontSize + 2.f,
                ImVec2(vecMark.x + 20.f, vecMark.y - 8.f),
                std::string("QUROL OG'ZI"), colMark, DRAW_TEXT_DROPSHADOW, Color(0, 0, 0, 220));
        }

        std::vector<Shot_t> vecCopy;
        {
            std::lock_guard<std::mutex> lock(s_mtxShots);
            if (s_vecShots.empty())
                return;
            vecCopy = s_vecShots;
        }

        const float flNow       = NowSeconds();
        const float flLife      = ImMax(0.05f, CONFIG_GET(float, g_Variables.m_Tracer.m_flLife));
        const float flThickness = CONFIG_GET(float, g_Variables.m_Tracer.m_flThickness);
        const int   iStyle      = CONFIG_GET(int,   g_Variables.m_Tracer.m_iStyle);
        const bool  bMuzzle     = CONFIG_GET(bool,  g_Variables.m_Tracer.m_bMuzzleFlash);
        const bool  bImpact     = CONFIG_GET(bool,  g_Variables.m_Tracer.m_bImpact);
        const bool  bTravel     = CONFIG_GET(bool,  g_Variables.m_Tracer.m_bTravel);
        const bool  bSmoke      = CONFIG_GET(bool,  g_Variables.m_Tracer.m_bSmoke);
        const float flSpeed     = ImMax(500.f, CONFIG_GET(float, g_Variables.m_Tracer.m_flSpeed));
        const float flDashLen   = ImMax(20.f,  CONFIG_GET(float, g_Variables.m_Tracer.m_flDash));
        const Color colBase     = CONFIG_GET(Color, g_Variables.m_Tracer.m_colTracer);

        const int nFrame = static_cast<int>(flNow * 26.f);

        // qurol og'zi ekranda: avval avtomatik, bo'lmasa qo'lda sozlangan nuqta
        ImVec2 vecMuzzleScreen(
            static_cast<float>(Window::m_iWidth)  * CONFIG_GET(float, g_Variables.m_Tracer.m_flMuzzleX),
            static_cast<float>(Window::m_iHeight) * CONFIG_GET(float, g_Variables.m_Tracer.m_flMuzzleY));

        if (CONFIG_GET(bool, g_Variables.m_Tracer.m_bAutoMuzzle))
        {
            ImVec2 vecAuto;
            if (ApparentMuzzleScreen(vecAuto))
                vecMuzzleScreen = vecAuto;
        }

        for (const Shot_t& shot : vecCopy)
        {
            const float flAge = flNow - shot.m_flSpawnTime;
            if (flAge < 0.f || flAge > flLife)
                continue;

            const float flTotal = (shot.m_bHasMuzzle ? shot.m_vecMuzzle : shot.m_vecNear).DistTo(shot.m_vecEnd);
            if (flTotal < 1.f)
                continue;

            // ---- yo'lni ekranga tushirish ----
            // Yo'l dunyoda hisoblanadi (perspektiva to'g'ri chiqadi), lekin
            // boshlanishi qurol og'ziga "tortiladi" — shuning uchun iz
            // quroldan chiqib, keyin o'qning haqiqiy yo'liga qo'shiladi.
            // Avtomatik rejimda yo'l aynan qurol og'zidan boshlanadi — hech
            // qanday ekran tuzatishi kerak emas. Aks holda (suyak topilmasa)
            // eski usul: yo'l boshi ekrandagi belgilangan nuqtaga tortiladi.
            const Vector vecPathStart = shot.m_bHasMuzzle ? shot.m_vecMuzzle : shot.m_vecNear;

            // Yo'l dunyoda hisoblanadi, lekin boshi ekranda qurol og'ziga
            // tortiladi — chunki viewmodel alohida FOV bilan chiziladi.
            ImVec2 vecPathStartScreen;
            if (!Draw::WorldToScreen(vecPathStart, vecPathStartScreen))
                continue;

            const ImVec2 vecPull(vecMuzzleScreen.x - vecPathStartScreen.x,
                                 vecMuzzleScreen.y - vecPathStartScreen.y);

            auto ProjectAt = [&](float t, ImVec2& vecOut) -> bool
            {
                const Vector vecWorld = vecPathStart + (shot.m_vecEnd - vecPathStart) * t;
                if (!Draw::WorldToScreen(vecWorld, vecOut))
                    return false;

                const float flPull = (1.f - t) * (1.f - t) * (1.f - t);
                vecOut.x += vecPull.x * flPull;
                vecOut.y += vecPull.y * flPull;
                return true;
            };

            // ---- so'nish ----
            const float flFade  = ImClamp(1.f - (flAge / flLife), 0.f, 1.f);
            const float flAlpha = flFade * flFade;

            // ============ 1) TUTUN IPI ============
            // Haqiqiy o'q izi: ingichka oq ip, biroz to'lqinlanadi va
            // vaqt o'tishi bilan pastga cho'kib yo'qoladi.
            if (bSmoke || iStyle == STYLE_SMOKE)
            {
                // yo'lga perpendikulyar ikkita o'q
                Vector vecDir = shot.m_vecEnd - vecPathStart;
                const float flDirLen = vecDir.Length();
                if (flDirLen > 1.f)
                {
                    vecDir = vecDir / flDirLen;

                    Vector vecRight(vecDir.y, -vecDir.x, 0.f);
                    if (vecRight.Length() < 1e-3f) vecRight = Vector(1.f, 0.f, 0.f);
                    vecRight.Normalize();

                    Vector vecUp(
                        vecRight.y * vecDir.z - vecRight.z * vecDir.y,
                        vecRight.z * vecDir.x - vecRight.x * vecDir.z,
                        vecRight.x * vecDir.y - vecRight.y * vecDir.x);
                    vecUp.Normalize();

                    const float flSag  = CONFIG_GET(float, g_Variables.m_Tracer.m_flSag);
                    const float flWave = CONFIG_GET(float, g_Variables.m_Tracer.m_flWave);

                    const int nPoints = 18;
                    ImVec2 vecPrev;
                    bool   bPrevOk = false;

                    for (int i = 0; i <= nPoints; i++)
                    {
                        const float t = static_cast<float>(i) / nPoints;

                        Vector vecPoint = vecPathStart + (shot.m_vecEnd - vecPathStart) * t;

                        // to'lqinlanish — quroldan uzoqlashgani sari kuchayadi
                        const float flPhase = static_cast<float>(shot.m_uSeed % 628) * 0.01f;
                        const float flAmp   = flWave * t * (0.6f + 0.4f * flFade);
                        vecPoint = vecPoint
                            + vecRight * (sinf(t * 11.f + flPhase + flAge * 1.6f) * flAmp)
                            + vecUp    * (cosf(t * 9.f  + flPhase * 1.7f) * flAmp * 0.7f);

                        // pastga cho'kish — vaqt kvadratiga proporsional
                        vecPoint.z -= flSag * flAge * flAge * (0.25f + 0.75f * t);

                        ImVec2 vecCur;
                        const bool bOk = Draw::WorldToScreen(vecPoint, vecCur);

                        if (bPrevOk && bOk)
                        {
                            // ingichka, oqish, asta so'nadi
                            const float flA = flFade * flFade * (0.35f + 0.65f * t);
                            const int   nA  = static_cast<int>(150.f * flA);
                            if (nA > 2)
                            {
                                Draw::AddLine(vecPrev, vecCur, RGBA(235, 240, 245, nA / 3),
                                    ImMax(1.f, 2.2f * (1.f + (1.f - flFade))));
                                Draw::AddLine(vecPrev, vecCur, RGBA(245, 248, 252, nA), 1.f);
                            }
                        }

                        vecPrev = vecCur;
                        bPrevOk = bOk;
                    }
                }
            }

            // ============ 2) UCHAYOTGAN CHIZIQCHA ============
            const float flHeadDist = bTravel ? (flAge * flSpeed) : (flTotal + flDashLen);
            const float flTailDist = bTravel ? ImMax(0.f, flHeadDist - flDashLen) : 0.f;

            const float t1 = ImClamp(flHeadDist / flTotal, 0.f, 1.f);
            const float t0 = ImClamp(flTailDist / flTotal, 0.f, 1.f);

            const bool bArrived = (flHeadDist >= flTotal);
            const bool bGone    = (flTailDist >= flTotal);

            if (!bGone && t1 > t0)
            {
                const int nSegments = (iStyle == STYLE_BOLT) ? 14 : 10;

                // chaqmoq uchun perpendikulyar yo'nalish
                ImVec2 vecEndA, vecEndB;
                const bool bEndsOk = ProjectAt(t0, vecEndA) && ProjectAt(t1, vecEndB);
                float px = 0.f, py = 0.f;
                if (bEndsOk)
                {
                    float dx = vecEndB.x - vecEndA.x, dy = vecEndB.y - vecEndA.y;
                    const float flLen = ImMax(1.f, sqrtf(dx * dx + dy * dy));
                    dx /= flLen; dy /= flLen;
                    px = -dy; py = dx;
                }

                ImVec2 vecPrev;
                bool   bPrevOk = false;

                for (int i = 0; i <= nSegments; i++)
                {
                    const float k = static_cast<float>(i) / nSegments;
                    const float t = t0 + (t1 - t0) * k;

                    ImVec2 vecCur;
                    if (!ProjectAt(t, vecCur))
                    {
                        bPrevOk = false;
                        continue;
                    }

                    // chaqmoq uslubida zigzag
                    if (iStyle == STYLE_BOLT && bEndsOk && i > 0 && i < nSegments)
                    {
                        const float flAmp  = ImClamp(12.f * (1.f - t), 1.5f, 12.f);
                        const float flSign = (i % 2 == 0) ? 1.f : -1.f;
                        const float flMag  = 0.5f + 0.5f * fabsf(Noise(shot.m_uSeed, i, nFrame));
                        vecCur.x += px * flSign * flMag * flAmp;
                        vecCur.y += py * flSign * flMag * flAmp;
                    }

                    if (bPrevOk)
                    {
                        // dum shaffof, bosh yorqin
                        const float flSegA = flAlpha * (0.10f + 0.90f * k * k);
                        // uzoqlashgani sari ingichka
                        const float flW    = ImMax(0.8f, flThickness * (1.f - t * 0.6f));

                        if (iStyle == STYLE_SMOKE)
                        {
                            // faqat uchayotgan o'qning o'zi — juda ingichka va oq
                            Draw::AddLine(vecPrev, vecCur, RGBA(255, 250, 240, static_cast<int>(220 * flSegA)), 1.4f);
                        }
                        else if (iStyle == STYLE_LINE)
                        {
                            Draw::AddLine(vecPrev, vecCur, FadeCol(colBase, flSegA), flW);
                        }
                        else
                        {
                            Draw::AddLine(vecPrev, vecCur, FadeCol(colBase, flSegA * 0.22f), ImMax(1.f, flW * 3.6f));
                            Draw::AddLine(vecPrev, vecCur, FadeCol(colBase, flSegA * 0.85f), ImMax(1.f, flW * 1.7f));
                            Draw::AddLine(vecPrev, vecCur,
                                RGBA(255, 250, 235, static_cast<int>(235 * flSegA)), ImMax(1.f, flW * 0.7f));
                        }
                    }

                    vecPrev = vecCur;
                    bPrevOk = true;
                }

                // ---- uchayotgan o'qning boshi ----
                if (bTravel && !bArrived && bPrevOk)
                {
                    const float flR = ImMax(1.2f, flThickness * (1.f - t1 * 0.6f));
                    Draw::AddCircle(vecPrev, flR * 2.6f, FadeCol(colBase, flAlpha * 0.30f), 12, DRAW_CIRCLE_FILLED);
                    Draw::AddCircle(vecPrev, flR,
                        RGBA(255, 250, 235, static_cast<int>(240 * flAlpha)), 10, DRAW_CIRCLE_FILLED);
                }
            }

            // ============ 3) QUROL OG'ZIDAGI CHAQNASH ============
            if (bMuzzle && flAge < 0.05f)
            {
                ImVec2 vecFlashPos = vecMuzzleScreen;
                if (shot.m_bHasMuzzle)
                    Draw::WorldToScreen(shot.m_vecMuzzle, vecFlashPos);

                const float flFlash = 1.f - (flAge / 0.05f);
                Draw::AddCircle(vecFlashPos, 2.f + 5.f * flFlash,
                    RGBA(255, 235, 170, static_cast<int>(200 * flFlash)), 12, DRAW_CIRCLE_FILLED);
                Draw::AddCircle(vecFlashPos, 7.f + 10.f * flFlash,
                    RGBA(255, 190, 90, static_cast<int>(60 * flFlash)), 14, DRAW_CIRCLE_FILLED);
            }

            // ============ 4) TEGGAN JOY ============
            if (bImpact && bArrived)
            {
                ImVec2 vecHit;
                if (ProjectAt(1.f, vecHit))
                {
                    const float flSince = bTravel ? (flAge - flTotal / flSpeed) : flAge;
                    const float flBurst = ImClamp(1.f - flSince / 0.22f, 0.f, 1.f);

                    if (flBurst > 0.02f)
                    {
                        // uchqunlar
                        for (int i = 0; i < 5; i++)
                        {
                            const float a  = Noise(shot.m_uSeed, 40 + i, 0) * 3.14159265f;
                            const float fl = (4.f + 9.f * fabsf(Noise(shot.m_uSeed, 60 + i, 0))) * flBurst;
                            Draw::AddLine(vecHit,
                                ImVec2(vecHit.x + cosf(a) * fl, vecHit.y + sinf(a) * fl),
                                RGBA(255, 225, 160, static_cast<int>(220 * flBurst)), 1.2f);
                        }

                        // chang bulutchasi
                        Draw::AddCircle(vecHit, 2.f + 4.f * (1.f - flBurst),
                            RGBA(200, 190, 175, static_cast<int>(70 * flBurst)), 12, DRAW_CIRCLE_FILLED);
                    }
                }
            }
        }

        // eskirganlarini tozalash
        {
            std::lock_guard<std::mutex> lock(s_mtxShots);
            s_vecShots.erase(
                std::remove_if(s_vecShots.begin(), s_vecShots.end(),
                    [&](const Shot_t& sh) { return (flNow - sh.m_flSpawnTime) > flLife; }),
                s_vecShots.end());
        }
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(s_mtxShots);
        s_vecShots.clear();
    }
}
