#include "../Includes.h"
#include "AgentImage.h"
#include "../features/skins/Skins.h"
#include "../features/loot/LootESP.h"
#include "../features/thirdperson/ThirdPerson.h"

// =====================================================================
//  KAEL_CHEAT  ::  CYBERPUNK MENU
//  Layout:  [ header ] [ nav rail | page ] [ status bar ]
// =====================================================================

// -----------------------------------------------------------------------
//  Local shorthands
// -----------------------------------------------------------------------
static inline ImU32 U32(const ImVec4& v) { return ImGui::ColorConvertFloat4ToU32(v); }
static inline ImU32 U32(const Color& c)   { return IM_COL32(c.r(), c.g(), c.b(), c.a()); }

// =======================================================================
//  ESP / GLOW  ::  JONLI KO'RINISH (live preview)
//  Sozlamalarni haqiqiy agent surati ustida ko'rsatadi.
// =======================================================================
namespace AgentPreview
{
    static ImTextureID s_pAgent      = nullptr;   // to'liq rangli surat
    static ImTextureID s_pSilhouette = nullptr;   // oq siluet (glow uchun)
    static int         s_iW = 0, s_iH = 0;
    static ImVec2      s_vecBodyMin(0.f, 0.f);    // ko'rinadigan piksellar chegarasi (0..1)
    static ImVec2      s_vecBodyMax(1.f, 1.f);
    static bool        s_bLoaded = false;
    static bool        s_bFailed = false;

    static ImTextureID CreateTexture(const unsigned char* pRGBA, int iW, int iH)
    {
        if (!Window::m_pDevice || !pRGBA)
            return nullptr;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width            = iW;
        desc.Height           = iH;
        desc.MipLevels        = 1;
        desc.ArraySize        = 1;
        desc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage            = D3D11_USAGE_DEFAULT;
        desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA sub = {};
        sub.pSysMem     = pRGBA;
        sub.SysMemPitch = iW * 4;

        ID3D11Texture2D* pTex = nullptr;
        if (FAILED(Window::m_pDevice->CreateTexture2D(&desc, &sub, &pTex)) || !pTex)
            return nullptr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Format                    = desc.Format;
        srv.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MipLevels       = 1;

        ID3D11ShaderResourceView* pSRV = nullptr;
        HRESULT hr = Window::m_pDevice->CreateShaderResourceView(pTex, &srv, &pSRV);
        pTex->Release();

        if (FAILED(hr) || !pSRV)
            return nullptr;

        return reinterpret_cast<ImTextureID>(pSRV);
    }

    static void Load()
    {
        if (s_bLoaded || s_bFailed || !Window::m_pDevice)
            return;

        int iW = 0, iH = 0, iCh = 0;
        unsigned char* pData = nullptr;

        // 1) exe yonidagi ct_image.png (foydalanuvchi almashtira oladi)
        char szExe[MAX_PATH];
        if (GetModuleFileNameA(NULL, szExe, MAX_PATH))
        {
            std::string strPath(szExe);
            const size_t pos = strPath.find_last_of("\\/");
            if (pos != std::string::npos)
                strPath = strPath.substr(0, pos + 1);
            strPath += "ct_image.png";

            std::error_code ec;
            if (std::filesystem::exists(strPath, ec))
                pData = stbi_load(strPath.c_str(), &iW, &iH, &iCh, 4);
        }

        // 2) ichiga qurilgan surat
        if (!pData)
            pData = stbi_load_from_memory(g_AgentImagePng, static_cast<int>(g_AgentImagePngSize), &iW, &iH, &iCh, 4);

        if (!pData || iW <= 0 || iH <= 0)
        {
            s_bFailed = true;
            return;
        }

        s_iW = iW;
        s_iH = iH;
        s_pAgent = CreateTexture(pData, iW, iH);

        // oq siluet + tananing haqiqiy chegarasi
        std::vector<unsigned char> vecSil(static_cast<size_t>(iW) * iH * 4);
        int iMinX = iW, iMaxX = 0, iMinY = iH, iMaxY = 0;

        for (int y = 0; y < iH; y++)
        {
            for (int x = 0; x < iW; x++)
            {
                const size_t i = (static_cast<size_t>(y) * iW + x) * 4;
                const unsigned char uAlpha = pData[i + 3];

                vecSil[i + 0] = 255;
                vecSil[i + 1] = 255;
                vecSil[i + 2] = 255;
                vecSil[i + 3] = uAlpha;

                if (uAlpha > 100)   // soyani hisobga olmaymiz
                {
                    if (x < iMinX) iMinX = x;
                    if (x > iMaxX) iMaxX = x;
                    if (y < iMinY) iMinY = y;
                    if (y > iMaxY) iMaxY = y;
                }
            }
        }

        if (iMaxX > iMinX && iMaxY > iMinY)
        {
            s_vecBodyMin = ImVec2(static_cast<float>(iMinX) / iW, static_cast<float>(iMinY) / iH);
            s_vecBodyMax = ImVec2(static_cast<float>(iMaxX) / iW, static_cast<float>(iMaxY) / iH);
        }

        s_pSilhouette = CreateTexture(vecSil.data(), iW, iH);
        stbi_image_free(pData);

        s_bLoaded = (s_pAgent != nullptr);
        s_bFailed = !s_bLoaded;
    }
}

static void DrawEspPreview(ImVec2 mn, ImVec2 size)
{
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 mx(mn.x + size.x, mn.y + size.y);

    // ---------------- panel chrome ----------------
    dl->AddRectFilled(mn, mx, IM_COL32(9, 12, 19, 255), 4.f);
    dl->AddRect(mn, mx, IM_COL32(26, 36, 52, 255), 4.f, 0, 1.f);
    UI::Grid(dl, ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), IM_COL32(34, 226, 255, 10), 24.f);
    dl->AddRectFilledMultiColor(mn, ImVec2(mn.x + size.x * 0.6f, mn.y + 1.f),
        UI::Fade(UI::COL_CYAN, 0.85f), UI::Fade(UI::COL_CYAN, 0.f),
        UI::Fade(UI::COL_CYAN, 0.f), UI::Fade(UI::COL_CYAN, 0.85f));
    UI::Brackets(dl, ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), UI::Fade(UI::COL_CYAN, 0.35f), 12.f, 1.3f);

    dl->AddRectFilled(ImVec2(mn.x + 10.f, mn.y + 11.f), ImVec2(mn.x + 13.f, mn.y + 25.f), UI::COL_CYAN, 1.f);
    dl->AddText(ImVec2(mn.x + 20.f, mn.y + 10.f), UI::COL_TEXT, X("JONLI KO'RINISH"));

    {
        const char* szLive = "LIVE";
        ImVec2 ts = ImGui::CalcTextSize(szLive);
        float  a  = 0.35f + 0.65f * UI::Pulse(2.6f);
        dl->AddCircleFilled(ImVec2(mx.x - ts.x - 26.f, mn.y + 18.f), 3.5f, UI::Fade(UI::COL_RED, a), 10);
        dl->AddText(ImVec2(mx.x - ts.x - 16.f, mn.y + 10.f), UI::COL_TEXT_MUTE, szLive);
    }

    AgentPreview::Load();

    const ImVec2 viewMin(mn.x + 16.f, mn.y + 46.f);
    const ImVec2 viewMax(mx.x - 16.f, mx.y - 30.f);

    if (!AgentPreview::s_pAgent)
    {
        const char* szErr = X("surat yuklanmadi");
        ImVec2 ts = ImGui::CalcTextSize(szErr);
        dl->AddText(ImVec2((viewMin.x + viewMax.x - ts.x) * 0.5f, (viewMin.y + viewMax.y) * 0.5f), UI::COL_TEXT_FAINT, szErr);
        return;
    }

    // ---------------- surat o'lchami ----------------
    const float flAvailW = viewMax.x - viewMin.x;
    const float flAvailH = viewMax.y - viewMin.y;
    const float flAspect = static_cast<float>(AgentPreview::s_iW) / static_cast<float>(AgentPreview::s_iH);

    const float flLabelSpace = 46.f;   // quti ostidagi yozuvlar uchun joy

    float flImgH = flAvailH - flLabelSpace;
    float flImgW = flImgH * flAspect;
    if (flImgW > flAvailW * 0.82f)
    {
        flImgW = flAvailW * 0.82f;
        flImgH = flImgW / flAspect;
    }

    // surat + yozuvlar blokini vertikal markazlaymiz
    const float flBlockTop = viewMin.y + ImMax(0.f, (flAvailH - (flImgH + flLabelSpace)) * 0.5f);
    const ImVec2 imgMin((viewMin.x + viewMax.x) * 0.5f - flImgW * 0.5f, flBlockTop);
    const ImVec2 imgMax(imgMin.x + flImgW, imgMin.y + flImgH);

    // tananing chegarasi (ESP box shu joyga tushadi)
    const ImVec2 boxMin(imgMin.x + flImgW * AgentPreview::s_vecBodyMin.x,
                        imgMin.y + flImgH * AgentPreview::s_vecBodyMin.y);
    const ImVec2 boxMax(imgMin.x + flImgW * AgentPreview::s_vecBodyMax.x,
                        imgMin.y + flImgH * AgentPreview::s_vecBodyMax.y);
    const float  flBoxW = boxMax.x - boxMin.x;
    const float  flBoxH = boxMax.y - boxMin.y;
    const float  flCx   = (boxMin.x + boxMax.x) * 0.5f;

    const bool  bVisuals = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals);
    const Color colEnemy = CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyVisible);
    const ImU32 uEnemy   = U32(colEnemy);

    // ---------------- GLOW (surat ortida) ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow) && AgentPreview::s_pSilhouette)
    {
        const int   iType = CONFIG_GET(int, g_Variables.m_PlayerGlow.m_iGlowType);
        const Color colGlow = CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowEnemy);

        if (iType != 0)
        {
            const float flPulse = (iType == 2) ? (0.40f + 0.60f * UI::Pulse(3.2f)) : 1.f;

            struct Layer_t { float flRadius; float flAlpha; };
            const Layer_t kOutline[] = { { 2.f, 0.55f }, { 4.f, 0.28f } };
            const Layer_t kSoft[]    = { { 4.f, 0.22f }, { 9.f, 0.14f }, { 15.f, 0.08f } };

            const Layer_t* pLayers = (iType == 3) ? kOutline : kSoft;
            const int      nLayers = (iType == 3) ? 2 : 3;

            for (int l = 0; l < nLayers; l++)
            {
                const float flR = pLayers[l].flRadius;
                const int   iA  = static_cast<int>(pLayers[l].flAlpha * flPulse * colGlow.a());
                if (iA <= 2) continue;

                const ImU32 uCol = IM_COL32(colGlow.r(), colGlow.g(), colGlow.b(), iA);

                for (int d = 0; d < 8; d++)
                {
                    const float flAng = static_cast<float>(d) * (IM_PI / 4.f);
                    const ImVec2 off(cosf(flAng) * flR, sinf(flAng) * flR);
                    dl->AddImage(AgentPreview::s_pSilhouette,
                        ImVec2(imgMin.x + off.x, imgMin.y + off.y),
                        ImVec2(imgMax.x + off.x, imgMax.y + off.y),
                        ImVec2(0, 0), ImVec2(1, 1), uCol);
                }
            }
        }
    }

    // ---------------- agentning o'zi ----------------
    dl->AddImage(AgentPreview::s_pAgent, imgMin, imgMax);

    // ---------------- tana bo'yash (model ustidan) ----------------
    if (bVisuals && CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawFilledBody) && AgentPreview::s_pSilhouette)
        dl->AddImage(AgentPreview::s_pSilhouette, imgMin, imgMax, ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(colEnemy.r(), colEnemy.g(), colEnemy.b(), 110));

    if (!bVisuals)
    {
        const char* szOff = X("ESP o'chirilgan");
        ImVec2 ts = ImGui::CalcTextSize(szOff);
        dl->AddText(ImVec2((viewMin.x + viewMax.x - ts.x) * 0.5f, viewMin.y + 4.f), UI::COL_TEXT_FAINT, szOff);
        return;
    }

    // ---------------- snapline ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSnaplines))
        dl->AddLine(ImVec2((viewMin.x + viewMax.x) * 0.5f, viewMax.y), ImVec2(flCx, boxMax.y),
            IM_COL32(colEnemy.r(), colEnemy.g(), colEnemy.b(), 160), 1.2f);

    // ---------------- skelet ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSkeleton))
    {
        auto P = [&](float fx, float fy) { return ImVec2(boxMin.x + flBoxW * fx, boxMin.y + flBoxH * fy); };
        const ImU32 uBone = IM_COL32(colEnemy.r(), colEnemy.g(), colEnemy.b(), 190);

        const ImVec2 head(P(0.50f, 0.07f)), neck(P(0.50f, 0.15f)), chest(P(0.50f, 0.26f)), hips(P(0.50f, 0.50f));
        dl->AddLine(head, neck, uBone, 1.4f);
        dl->AddLine(neck, chest, uBone, 1.4f);
        dl->AddLine(chest, hips, uBone, 1.4f);

        dl->AddLine(neck, P(0.30f, 0.20f), uBone, 1.2f);
        dl->AddLine(P(0.30f, 0.20f), P(0.24f, 0.34f), uBone, 1.2f);
        dl->AddLine(P(0.24f, 0.34f), P(0.40f, 0.42f), uBone, 1.2f);

        dl->AddLine(neck, P(0.70f, 0.20f), uBone, 1.2f);
        dl->AddLine(P(0.70f, 0.20f), P(0.76f, 0.33f), uBone, 1.2f);
        dl->AddLine(P(0.76f, 0.33f), P(0.60f, 0.41f), uBone, 1.2f);

        dl->AddLine(hips, P(0.38f, 0.72f), uBone, 1.3f);
        dl->AddLine(P(0.38f, 0.72f), P(0.36f, 0.98f), uBone, 1.3f);
        dl->AddLine(hips, P(0.62f, 0.72f), uBone, 1.3f);
        dl->AddLine(P(0.62f, 0.72f), P(0.64f, 0.98f), uBone, 1.3f);
    }

    // ---------------- quti ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBox))
    {
        const int  iType    = CONFIG_GET(int, g_Variables.m_PlayerVisuals.m_iBoxType);
        const bool bOutline = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline);

        if (bOutline)
        {
            dl->AddRect(ImVec2(boxMin.x - 1.f, boxMin.y - 1.f), ImVec2(boxMax.x + 1.f, boxMax.y + 1.f), IM_COL32(0, 0, 0, 190), 0.f, 0, 1.f);
            dl->AddRect(ImVec2(boxMin.x + 1.f, boxMin.y + 1.f), ImVec2(boxMax.x - 1.f, boxMax.y - 1.f), IM_COL32(0, 0, 0, 190), 0.f, 0, 1.f);
        }

        if (iType == BOX_TYPE_2D || iType == BOX_TYPE_BOTH)
            dl->AddRect(boxMin, boxMax, uEnemy, 0.f, 0, 1.4f);

        if (iType == BOX_TYPE_CORNER || iType == BOX_TYPE_BOTH)
        {
            const float flLen = ImMin(flBoxW, flBoxH) * 0.28f;
            dl->AddLine(boxMin, ImVec2(boxMin.x + flLen, boxMin.y), uEnemy, 1.8f);
            dl->AddLine(boxMin, ImVec2(boxMin.x, boxMin.y + flLen), uEnemy, 1.8f);
            dl->AddLine(ImVec2(boxMax.x - flLen, boxMin.y), ImVec2(boxMax.x, boxMin.y), uEnemy, 1.8f);
            dl->AddLine(ImVec2(boxMax.x, boxMin.y), ImVec2(boxMax.x, boxMin.y + flLen), uEnemy, 1.8f);
            dl->AddLine(ImVec2(boxMin.x, boxMax.y - flLen), ImVec2(boxMin.x, boxMax.y), uEnemy, 1.8f);
            dl->AddLine(ImVec2(boxMin.x, boxMax.y), ImVec2(boxMin.x + flLen, boxMax.y), uEnemy, 1.8f);
            dl->AddLine(ImVec2(boxMax.x - flLen, boxMax.y), boxMax, uEnemy, 1.8f);
            dl->AddLine(ImVec2(boxMax.x, boxMax.y - flLen), boxMax, uEnemy, 1.8f);
        }
    }

    // ---------------- jon paneli ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHealthBar))
    {
        const int   iHealth = 87;
        const float flFrac  = iHealth / 100.f;
        const ImU32 uHP     = IM_COL32(static_cast<int>((1.f - flFrac) * 255.f), static_cast<int>(flFrac * 255.f), 0, 255);

        const ImVec2 barMin(boxMin.x - 8.f, boxMin.y);
        const ImVec2 barMax(boxMin.x - 3.f, boxMax.y);
        dl->AddRectFilled(barMin, barMax, IM_COL32(0, 0, 0, 190));
        dl->AddRectFilled(ImVec2(barMin.x, barMax.y - (barMax.y - barMin.y) * flFrac), barMax, uHP);

        if (Fonts::Small) ImGui::PushFont(Fonts::Small);
        dl->AddText(ImVec2(barMin.x - 4.f, barMax.y + 2.f), IM_COL32(255, 255, 255, 220), "87");
        if (Fonts::Small) ImGui::PopFont();
    }

    if (Fonts::Small) ImGui::PushFont(Fonts::Small);

    // ---------------- ism ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawName))
    {
        const char* szName = "Dushman";
        ImVec2 ts = ImGui::CalcTextSize(szName);
        dl->AddText(ImVec2(flCx - ts.x * 0.5f + 1.f, boxMin.y - ts.y - 1.f), IM_COL32(0, 0, 0, 220), szName);
        dl->AddText(ImVec2(flCx - ts.x * 0.5f, boxMin.y - ts.y - 2.f), IM_COL32(255, 255, 255, 255), szName);
    }

    // ---------------- bomba belgisi ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHasC4))
    {
        const char* szC4 = "C4";
        ImVec2 ts = ImGui::CalcTextSize(szC4);
        ImVec2 p(boxMax.x + 6.f, boxMin.y + 2.f);
        dl->AddRectFilled(ImVec2(p.x - 3.f, p.y - 2.f), ImVec2(p.x + ts.x + 3.f, p.y + ts.y + 2.f), IM_COL32(255, 190, 60, 40), 2.f);
        dl->AddRect(ImVec2(p.x - 3.f, p.y - 2.f), ImVec2(p.x + ts.x + 3.f, p.y + ts.y + 2.f), IM_COL32(255, 190, 60, 200), 2.f, 0, 1.f);
        dl->AddText(p, IM_COL32(255, 190, 60, 255), szC4);
    }

    float flBelowY = boxMax.y + 3.f;

    // ---------------- qurol ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawWeapon))
    {
        const std::string strWeapon = "ak47";
        const ImU32       uWeapon   = IM_COL32(255, 80, 80, 255);

        int iTexW = 0, iTexH = 0;
        if (WeaponIcons::HasIcon(strWeapon) && WeaponIcons::GetIconSize(strWeapon, iTexW, iTexH) && iTexH > 0)
        {
            const float flIconW = ImClamp(flBoxW * 0.75f, 34.f, 74.f);
            const float flIconH = flIconW / (static_cast<float>(iTexW) / static_cast<float>(iTexH));
            dl->AddImage(WeaponIcons::GetIcon(strWeapon),
                ImVec2(flCx - flIconW * 0.5f, flBelowY),
                ImVec2(flCx + flIconW * 0.5f, flBelowY + flIconH), ImVec2(0, 0), ImVec2(1, 1), uWeapon);
            flBelowY += flIconH + 2.f;
        }
        else
        {
            const char* szWeapon = "AK47";
            ImVec2 ts = ImGui::CalcTextSize(szWeapon);
            dl->AddText(ImVec2(flCx - ts.x * 0.5f + 1.f, flBelowY + 1.f), IM_COL32(0, 0, 0, 200), szWeapon);
            dl->AddText(ImVec2(flCx - ts.x * 0.5f, flBelowY), uWeapon, szWeapon);
            flBelowY += ts.y + 2.f;
        }
    }

    // ---------------- masofa ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawDistance))
    {
        const char* szDist = "14m";
        ImVec2 ts = ImGui::CalcTextSize(szDist);
        dl->AddText(ImVec2(flCx - ts.x * 0.5f + 1.f, flBelowY + 1.f), IM_COL32(0, 0, 0, 190), szDist);
        dl->AddText(ImVec2(flCx - ts.x * 0.5f, flBelowY), IM_COL32(180, 180, 180, 220), szDist);
    }

    if (Fonts::Small) ImGui::PopFont();

    // ---------------- bosh nuqta ----------------
    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHeadDot))
    {
        const ImVec2 vecHead(flCx, boxMin.y + flBoxH * 0.055f);
        dl->AddCircleFilled(vecHead, 3.2f, uEnemy, 12);
        dl->AddCircle(vecHead, 5.2f, IM_COL32(0, 0, 0, 170), 12, 1.f);
    }

    // ---------------- panel izohi ----------------
    if (Fonts::Small) ImGui::PushFont(Fonts::Small);
    {
        const char* szHint = X("sozlamalar shu yerda jonli ko'rinadi");
        ImVec2 ts = ImGui::CalcTextSize(szHint);
        dl->AddText(ImVec2((mn.x + mx.x - ts.x) * 0.5f, mx.y - 20.f), UI::COL_TEXT_FAINT, szHint);
    }
    if (Fonts::Small) ImGui::PopFont();
}

// =======================================================================
//  PAGE :: VIZUAL
// =======================================================================
static void DrawVisualSettings()
{
    bool& bVisuals = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals);

    UI::BeginCard(X("DEVOR ORTI KO'RISH  ·  WALLHACK"));
    {
        UI::Toggle(X("Devordan ko'rishni yoqish"), &bVisuals);
        UI::Hint(X("O'yinchilarni devor ortidan ko'rsatadi (ESP)."));

        if (bVisuals)
        {
            UI::Gap(4.f);
            auto& vecMod = Config::Get<std::vector<bool>>(g_Variables.m_PlayerVisuals.m_vecVisualsModifiers);
            bool bIgnoreTeam = vecMod[VISUALS_IGNORE_TEAMMATES];
            bool bOnlyVis    = vecMod[VISUALS_ONLY_WHEN_VISIBLE];
            if (UI::Toggle(X("Jamoani e'tiborsiz"), &bIgnoreTeam)) vecMod[VISUALS_IGNORE_TEAMMATES] = bIgnoreTeam;
            UI::Col2();
            if (UI::Toggle(X("Faqat ko'ringanlar"), &bOnlyVis))    vecMod[VISUALS_ONLY_WHEN_VISIBLE] = bOnlyVis;
        }
    }
    UI::EndCard();

    if (bVisuals)
    {
        UI::BeginCard(X("QUTI  ·  BOX"));
        {
            bool& bBox = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBox);
            UI::Toggle(X("Quti chizish"), &bBox);
            if (bBox)
            {
                UI::Col2();
                UI::Toggle(X("Quti chegarasi"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline));

                UI::Gap(4.f);
                static const char* boxTypes[] = { "2D Quti", "Burchak", "Ikkisi" };
                UI::Combo(X("Quti turi"), "##boxtype", &CONFIG_GET(int, g_Variables.m_PlayerVisuals.m_iBoxType), boxTypes, 3, 190.f);
            }
        }
        UI::EndCard();

        UI::BeginCard(X("MA'LUMOTLAR  ·  OVERLAY"));
        {
            UI::Toggle(X("Jon paneli"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHealthBar));
            UI::Col2();
            UI::Toggle(X("Ism"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawName));

            UI::Toggle(X("Qurol nomi"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawWeapon));
            UI::Col2();
            UI::Toggle(X("Masofa"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawDistance));

            UI::Toggle(X("Skelet"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSkeleton));
            UI::Col2();
            UI::Toggle(X("Bosh nuqta"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHeadDot));

            UI::Toggle(X("Tana bo'yash"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawFilledBody));
            UI::Col2();
            UI::Toggle(X("Chiziqlar"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSnaplines));

            UI::Toggle(X("Bomba ogohlantirish"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHasC4));
            UI::Col2();
            UI::Toggle(X("Ekran tashqarisi belgilari"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawOffScreen));

            UI::Gap(4.f);
            UI::Toggle(X("3D zarar ko'rsatkichi"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bDamageIndicator));
            UI::Col2();
            UI::ColorPick(X("Zarar rangi"), CONFIG_GET(Color, g_Variables.m_Misc.m_colDamageIndicator));
        }
        UI::EndCard();

        UI::BeginCard(X("RANGLAR  ·  PALETTE"));
        {
            UI::ColorPick(X("Dushman (ko'ringan)"), CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyVisible));
            UI::ColorPick(X("Dushman (yashirin)"),  CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyOccluded));
            UI::ColorPick(X("Jamoadosh"),           CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colTeammate));
        }
        UI::EndCard();
    }

    UI::BeginCard(X("YERDAGI QUROLLAR  ·  LOOT"));
    {
        bool& bLoot = CONFIG_GET(bool, g_Variables.m_ESP.m_bDroppedWeapons);
        UI::Toggle(X("Loot ESP yoqish"), &bLoot);
        if (bLoot)
        {
            UI::Gap(3.f);
            UI::SliderF(X("Ko'rsatish masofasi"), "##lootdist",
                &CONFIG_GET(float, g_Variables.m_ESP.m_flWeaponDistance), 5.f, 200.f, "%.0f m", 240.f);

            UI::Gap(3.f);
            if (!LootESP::m_Status.m_bOffsetsOk)
                UI::Notice(UI::COL_RED, X("Offsetlar topilmadi."));
            else if (LootESP::m_Status.m_nFound == 0)
                UI::Notice(UI::COL_AMBER, X("Yerda qurol topilmadi (skan: %d entity)."), LootESP::m_Status.m_nScanned);
            else
                UI::Hint(X("OK  ·  topildi: %d,  ekranda: %d"),
                    LootESP::m_Status.m_nFound, LootESP::m_Status.m_nDrawn);
        }
    }
    UI::EndCard();

    UI::BeginCard(X("ODAMNI YORITISH  ·  GLOW"));
    {
        bool& bGlow = CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow);
        UI::Toggle(X("Yoritishni yoqish"), &bGlow);
        UI::Hint(X("Dushmanni (yoki jamoadoshni) fosfordek yoritib turadi."));

        if (bGlow)
        {
            UI::Gap(4.f);
            UI::Toggle(X("Jon va qurolni ko'rsatish"), &CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowInfo));
            UI::Col2();
            UI::Toggle(X("Faqat dushmanlar"), &CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowEnemyOnly));

            UI::Gap(4.f);
            static const char* glowTypes[] = { "O'chirilgan", "Oddiy", "Pulsating", "Tashqi kontur" };
            UI::Combo(X("Yoritish turi"), "##glowtype", &CONFIG_GET(int, g_Variables.m_PlayerGlow.m_iGlowType), glowTypes, 4, 200.f);

            UI::Gap(3.f);
            UI::ColorPick(X("Dushman rangi"), CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowEnemy));
            if (!CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowEnemyOnly))
                UI::ColorPick(X("Jamoadosh rangi"), CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowTeam));
        }
    }
    UI::EndCard();

    UI::BeginCard(X("O'Q IZI  ·  TRACER"));
    {
        bool& bTracer = CONFIG_GET(bool, g_Variables.m_Tracer.m_bEnable);
        UI::Toggle(X("O'q izini yoqish"), &bTracer);
        UI::Hint(X("Otganingizda qurol og'zidan chiqadigan chiziq / chaqmoq."));

        if (bTracer)
        {
            UI::Gap(4.f);
            if (UI::Button(X("REALISTIK"), ImVec2(150.f, 26.f), UI::BTN_PRIMARY))
            {
                CONFIG_GET(int,   g_Variables.m_Tracer.m_iStyle)       = 3;      // haqiqiy (tutun)
                CONFIG_GET(float, g_Variables.m_Tracer.m_flLife)       = 1.6f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flThickness)  = 1.2f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flSag)        = 55.f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flWave)       = 2.6f;
                CONFIG_GET(bool,  g_Variables.m_Tracer.m_bTravel)      = true;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flSpeed)      = 11000.f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flDash)       = 380.f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flSpread)     = 0.35f;
                CONFIG_GET(bool,  g_Variables.m_Tracer.m_bSmoke)       = true;
                CONFIG_GET(bool,  g_Variables.m_Tracer.m_bMuzzleFlash) = false;   // o'yinning o'z chaqnashi bor
                CONFIG_GET(Color, g_Variables.m_Tracer.m_colTracer)    = Color(255, 214, 140, 255);
            }
            ImGui::SameLine(0.f, 8.f);
            if (UI::Button(X("CHAQMOQ"), ImVec2(150.f, 26.f)))
            {
                CONFIG_GET(int,   g_Variables.m_Tracer.m_iStyle)       = 1;      // chaqmoq
                CONFIG_GET(float, g_Variables.m_Tracer.m_flLife)       = 0.45f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flThickness)  = 2.0f;
                CONFIG_GET(bool,  g_Variables.m_Tracer.m_bTravel)      = true;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flSpeed)      = 5000.f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flDash)       = 900.f;
                CONFIG_GET(float, g_Variables.m_Tracer.m_flSpread)     = 0.f;
                CONFIG_GET(bool,  g_Variables.m_Tracer.m_bSmoke)       = false;
                CONFIG_GET(bool,  g_Variables.m_Tracer.m_bMuzzleFlash) = true;
                CONFIG_GET(Color, g_Variables.m_Tracer.m_colTracer)    = Color(34, 226, 255, 255);
            }
            UI::Hint(X("REALISTIK — iliq rang, tez uchadi, devorga tegib to'xtaydi, ortidan tutun qoladi."));

            UI::Gap(4.f);
            static const char* kStyles[] = { "Chiziq", "Chaqmoq", "Nur", "Haqiqiy (tutun)" };
            UI::Combo(X("Ko'rinishi"), "##tracerstyle", &CONFIG_GET(int, g_Variables.m_Tracer.m_iStyle), kStyles, 4, 190.f);
            UI::Col2();
            UI::ColorPick(X("Rangi"), CONFIG_GET(Color, g_Variables.m_Tracer.m_colTracer));

            UI::Gap(4.f);
            UI::SliderF(X("Ko'rinish vaqti"), "##tracerlife", &CONFIG_GET(float, g_Variables.m_Tracer.m_flLife), 0.1f, 3.0f, "%.1f s", 210.f);
            UI::Col2();
            UI::SliderF(X("Qalinligi"), "##tracerthick", &CONFIG_GET(float, g_Variables.m_Tracer.m_flThickness), 1.0f, 6.0f, "%.1f px", 210.f);

            UI::Gap(4.f);
            UI::SliderF(X("Uzunligi"), "##tracerlen", &CONFIG_GET(float, g_Variables.m_Tracer.m_flLength), 512.f, 16384.f, "%.0f unit", 240.f);

            UI::Gap(4.f);
            UI::Toggle(X("O'q bilan birga uchsin"), &CONFIG_GET(bool, g_Variables.m_Tracer.m_bTravel));
            UI::Hint(X("Iz qurol og'zidan chiqib nishonga qarab uchadi. O'chirilsa — butun chiziq birdan chiqadi."));

            if (CONFIG_GET(bool, g_Variables.m_Tracer.m_bTravel))
            {
                UI::Gap(3.f);
                UI::SliderF(X("O'q tezligi"), "##trspeed", &CONFIG_GET(float, g_Variables.m_Tracer.m_flSpeed), 1000.f, 30000.f, "%.0f u/s", 210.f);
                UI::Col2();
                UI::SliderF(X("Chiziqcha uzunligi"), "##trdash", &CONFIG_GET(float, g_Variables.m_Tracer.m_flDash), 50.f, 3000.f, "%.0f unit", 210.f);
            }

            UI::Gap(3.f);
            UI::SliderF(X("Tarqoqlik"), "##trspread", &CONFIG_GET(float, g_Variables.m_Tracer.m_flSpread), 0.f, 2.f, "%.2f deg", 210.f);
            UI::Col2();
            UI::Toggle(X("Tutun izi"), &CONFIG_GET(bool, g_Variables.m_Tracer.m_bSmoke));

            if (CONFIG_GET(bool, g_Variables.m_Tracer.m_bSmoke) || CONFIG_GET(int, g_Variables.m_Tracer.m_iStyle) == 3)
            {
                UI::Gap(3.f);
                UI::SliderF(X("Pastga cho'kishi"), "##trsag", &CONFIG_GET(float, g_Variables.m_Tracer.m_flSag), 0.f, 200.f, "%.0f", 210.f);
                UI::Col2();
                UI::SliderF(X("To'lqinlanishi"), "##trwave", &CONFIG_GET(float, g_Variables.m_Tracer.m_flWave), 0.f, 10.f, "%.1f", 210.f);
            }

            UI::Gap(4.f);
            bool& bAutoMuzzle = CONFIG_GET(bool, g_Variables.m_Tracer.m_bAutoMuzzle);
            UI::Toggle(X("Qurol og'zini avtomatik topish"), &bAutoMuzzle);
            UI::Hint(X("Qo'lingizdagi qurol modelining stvol uchi o'zi topiladi — har bir qurol uchun alohida sozlash shart emas."));

            if (bAutoMuzzle)
            {
                UI::Gap(3.f);
                UI::SliderF(X("Viewmodel FOV"), "##vmfov", &CONFIG_GET(float, g_Variables.m_Tracer.m_flViewmodelFov), 30.f, 90.f, "%.0f", 210.f);
                UI::Hint(X("O'yindagi viewmodel_fov bilan bir xil qiling (CS2 da odatda 60)."));
            }

            if (!bAutoMuzzle)
            {
            UI::Gap(4.f);
            UI::SliderF(X("Qurol og'zi — X"), "##muzx", &CONFIG_GET(float, g_Variables.m_Tracer.m_flMuzzleX), 0.f, 1.f, "%.2f", 210.f);
            UI::Col2();
            UI::SliderF(X("Qurol og'zi — Y"), "##muzy", &CONFIG_GET(float, g_Variables.m_Tracer.m_flMuzzleY), 0.f, 1.f, "%.2f", 210.f);
            UI::Notice(UI::COL_AMBER, X("Menyu ochiq turganda ekranda sariq nishon ko'rinadi — uni quroling og'ziga to'g'ri keltiring."));
            }

            UI::Gap(4.f);
            UI::Toggle(X("Qurol og'zida chaqnash"), &CONFIG_GET(bool, g_Variables.m_Tracer.m_bMuzzleFlash));
            UI::Col2();
            UI::Toggle(X("Tegish nuqtasi"), &CONFIG_GET(bool, g_Variables.m_Tracer.m_bImpact));
        }
    }
    UI::EndCard();

    UI::BeginCard(X("UCHINCHI SHAXS  ·  THIRD PERSON"));
    {
        bool& bTP = CONFIG_GET(bool, g_Variables.m_ThirdPerson.m_bEnable);
        UI::Toggle(X("Uchinchi shaxsni yoqish"), &bTP);
        UI::Hint(X("Kamerani orqaga olib chiqadi. Faqat sizda ko'rinadi — server uchun siz baribir birinchi shaxsdasiz."));

        if (bTP)
        {
            UI::Gap(4.f);
            UI::Keybind(X("Tugma"), CONFIG_GET(int, g_Variables.m_ThirdPerson.m_iKey), "tpkey");

            UI::Gap(4.f);
            UI::SliderF(X("Kamera masofasi"), "##tpdist",
                &CONFIG_GET(float, g_Variables.m_ThirdPerson.m_flDistance), 30.f, 400.f, "%.0f", 220.f);
            UI::Col2();
            static const char* kModes[] = { "Chase (5)", "Deathcam (1)", "Roaming (6)", "In-eye (4)" };
            static const int   kModeVals[] = { 5, 1, 6, 4 };
            int& iMode = CONFIG_GET(int, g_Variables.m_ThirdPerson.m_iMode);
            int  iSel  = 0;
            for (int i = 0; i < 4; i++) if (kModeVals[i] == iMode) iSel = i;
            if (UI::Combo(X("Rejim"), "##tpmode", &iSel, kModes, 4, 170.f))
                iMode = kModeVals[iSel];

            UI::Gap(3.f);
            if (!ThirdPerson::m_Status.m_bServicesOk)
                UI::Notice(UI::COL_AMBER, X("Observer services topilmadi (offset 0x%X, qiymat 0x%llX)"),
                    ThirdPerson::m_Status.m_uOffset, (unsigned long long)ThirdPerson::m_Status.m_uServices);
            else
                UI::Hint(X("Holat: %s  ·  o'yindagi rejim: %d"),
                    ThirdPerson::m_Status.m_bActive ? "YOQILGAN" : "o'chiq",
                    ThirdPerson::m_Status.m_iCurrentMode);

            UI::Hint(X("Agar kamera qimirlamasa — boshqa rejimni sinab ko'ring."));
        }
    }
    UI::EndCard();

    UI::BeginCard(X("DUNYO  ·  WORLD"));
    {
        bool& bNight = CONFIG_GET(bool, g_Variables.m_World.m_bNightMode);
        UI::Toggle(X("Tungi rejim"), &bNight);
        if (bNight)
        {
            UI::SliderF(X("Qorong'ulik"), "##night", &CONFIG_GET(float, g_Variables.m_World.m_flNightModeValue), 0.01f, 1.0f, "%.2f", 240.f);

            if (World::m_Status.m_uExposureOffset == 0U)
                UI::Notice(UI::COL_RED, X("Tonemap offseti topilmadi (schema)."));
            else if (!World::m_Status.m_bTonemapFound && World::m_Status.m_nPostVolumes > 0)
                UI::Hint(X("Tonemap yo'q — post-processing volume orqali (%d ta)"),
                    World::m_Status.m_nPostVolumes);
            else if (!World::m_Status.m_bTonemapFound)
                UI::Notice(UI::COL_AMBER, X("Tonemap ham, post-volume ham topilmadi (skan: %d)."),
                    World::m_Status.m_nScanned);
            else
                UI::Hint(X("OK  ·  tonemap topildi, exposure = %.2f"), World::m_Status.m_flExposure);

            UI::Gap(4.f);
            static std::string s_strDumpResult;
            if (UI::Button(X("ENTITY RO'YXATINI YOZIB OLISH"), ImVec2(250.f, 26.f)))
            {
                const std::string strPath = World::DumpEntities();
                s_strDumpResult = strPath.empty() ? "yozib bo'lmadi" : strPath;
            }

            if (!s_strDumpResult.empty())
                UI::Hint(X("Fayl: %s"), s_strDumpResult.c_str());
            else
                UI::Hint(X("Tungi rejim ishlamasa — shu tugmani o'yin ichida bosing va faylni menga yuboring."));
        }

        UI::Gap(4.f);
        bool& bFov = CONFIG_GET(bool, g_Variables.m_World.m_bFOVChanger);
        UI::Toggle(X("Kamera FOV o'zgartirish"), &bFov);
        if (bFov)
        {
            UI::SliderI(X("Kamera FOV"), "##camfov", &CONFIG_GET(int, g_Variables.m_World.m_iFOV), 40, 150, "%d deg", 240.f);

            if (World::m_Status.m_uFovOffset == 0U)
                UI::Notice(UI::COL_RED, X("m_iDesiredFOV offseti topilmadi (schema)."));
            else if (World::m_Status.m_iCurrentFov < 0)
                UI::Notice(UI::COL_AMBER, X("Local controller topilmadi — o'yinga kiring."));
            else
                UI::Hint(X("OK  ·  o'yindagi joriy FOV: %d"), World::m_Status.m_iCurrentFov);
        }
    }
    UI::EndCard();

    UI::BeginCard(X("TEZKOR TUGMA  ·  HOTKEY"));
    {
        UI::Keybind(X("ESP yoqish / o'chirish"), CONFIG_GET(int, g_Variables.m_Hotkeys.m_iESPToggleKey), "espkey");
    }
    UI::EndCard();
}

// Chapda sozlamalar, o'ngda jonli ko'rinish paneli
static void PageVisuals()
{
    const float flPreviewW = 348.f;
    const float flAvailW   = ImGui::GetContentRegionAvail().x;
    const float flAvailH   = ImGui::GetContentRegionAvail().y;

    if (flAvailW < flPreviewW + 380.f)   // tor bo'lsa — faqat sozlamalar
    {
        DrawVisualSettings();
        return;
    }

    const float flColH = flAvailH - 10.f;

    ImGui::BeginChild(X("##vis_settings"), ImVec2(flAvailW - flPreviewW - 14.f, flColH), false);
    {
        DrawVisualSettings();
    }
    ImGui::EndChild();

    ImGui::SameLine(0.f, 14.f);

    const ImVec2 vecPreviewPos = ImGui::GetCursorScreenPos();
    DrawEspPreview(vecPreviewPos, ImVec2(flPreviewW, flColH));
    ImGui::Dummy(ImVec2(flPreviewW, flColH));
}

// =======================================================================
//  PAGE :: HARAKAT
// =======================================================================
static void PageMovement()
{
    if (!g_License.HasFeature(ETier::MID))
    {
        UI::Locked(X("Bunny Hop"), "MID");
        return;
    }

    UI::BeginCard(X("BUNNY HOP  ·  AUTO JUMP"));
    {
        bool& bBhop = CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableBhop);
        UI::Toggle(X("Bhop yoqish"), &bBhop);

        if (bBhop)
        {
            UI::Gap(4.f);
            UI::Toggle(X("Auto-Strafer (avtomatik sakrash)"), &CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableAutoStrafe));

            UI::Gap(6.f);
            UI::Notice(UI::COL_AMBER, X("Space (32) tugmasini tanlamang!"));
            UI::Gap(4.f);
            UI::Keybind(X("Ushlab turish"), CONFIG_GET(int, g_Variables.m_Bhop.m_iBhopKey), "bhopkey");
            UI::Hint(X("Bu tugmani ushlab turing — dastur o'zi Space bosadi."));
        }
    }
    UI::EndCard();
}

// =======================================================================
//  PAGE :: JANG
// =======================================================================
static void PageCombat()
{
    // ---------------- TRIGGERBOT (MID+) ----------------
    if (g_License.HasFeature(ETier::MID))
    {
        UI::BeginCard(X("TRIGGERBOT  ·  AUTO FIRE"));
        {
            UI::Toggle(X("Triggerbot yoqish"), &CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bEnableTriggerbot));

            UI::Gap(4.f);
            bool& bAutoShoot = CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bAutoShoot);
            UI::Toggle(X("Avto otish (tugmasiz)"), &bAutoShoot);

            if (!bAutoShoot)
            {
                UI::Gap(4.f);
                UI::Keybind(X("Ushlab turish"), CONFIG_GET(int, g_Variables.m_TriggerBot.m_iTriggerKey), "trigkey");
                UI::Hint(X("Otish kerak bo'lganda shu tugmani bosib turing."));
            }
            else
            {
                UI::Notice(UI::COL_GREEN, X("Dushman nishonga tushganda avtomatik otadi."));
            }

            UI::Gap(5.f);
            UI::SliderF(X("Otish kechikishi"), "##trigdelay",
                &CONFIG_GET(float, g_Variables.m_TriggerBot.m_flShotDelay), 0.f, 300.f, "%.0f ms", 240.f);

            UI::Gap(4.f);
            UI::Toggle(X("Jamoani e'tiborsiz"), &CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bIgnoreTeammates));
            UI::Col2();
            UI::Toggle(X("Faqat ko'ringanlar"), &CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bOnlyVisible));
        }
        UI::EndCard();

        UI::BeginCard(X("ANTI-BAN  ·  TRIGGERBOT"));
        {
            UI::SliderF(X("Hitchance"), "##trighc",
                &CONFIG_GET(float, g_Variables.m_TriggerBot.m_flHitchance), 0.f, 100.f, "%.0f%%", 240.f);
            UI::Hint(X("80%% = har 5 ta imkoniyatdan 4 tasida otadi (20%% miss)."));

            UI::Gap(5.f);
            UI::SliderI(X("Min burst"), "##trigmin", &CONFIG_GET(int, g_Variables.m_TriggerBot.m_iMinBurst), 1, 5, "%d o'q", 160.f);
            UI::Col2();
            UI::SliderI(X("Max burst"), "##trigmax", &CONFIG_GET(int, g_Variables.m_TriggerBot.m_iMaxBurst), 1, 5, "%d o'q", 160.f);
            UI::Hint(X("Bir bosishda 1-3 ta o'q — odamga o'xshash otish."));
        }
        UI::EndCard();
    }
    else
    {
        UI::Locked(X("Triggerbot"), "MID");
    }

    UI::Gap(4.f);

    // ---------------- AIMBOT (PRO) ----------------
    if (!g_License.HasFeature(ETier::PRO))
    {
        UI::Locked(X("Aimbot + RCS"), "PRO");
        return;
    }

    UI::BeginCard(X("AIMBOT  ·  TARGETING"));
    {
        UI::Toggle(X("Aimbot yoqish"), &CONFIG_GET(bool, g_Variables.m_AimBot.m_bEnableAimbot));

        UI::Gap(4.f);
        UI::Keybind(X("Nishon tugmasi"), CONFIG_GET(int, g_Variables.m_AimBot.m_iAimKey), "aimkey");

        UI::Gap(5.f);
        UI::SliderF(X("FOV"), "##aimfov", &CONFIG_GET(float, g_Variables.m_AimBot.m_flFOV), 0.5f, 30.f, "%.1f deg", 240.f);
        UI::SliderF(X("Silliqlik  (1 = tez, 20 = silliq)"), "##aimsmooth",
            &CONFIG_GET(float, g_Variables.m_AimBot.m_flSmooth), 1.f, 20.f, "%.1f", 240.f);

        UI::Gap(4.f);
        static const char* hitboxNames[] = { "Bosh", "Bo'yin", "Ko'krak" };
        UI::Combo(X("Nishon joyi"), "##hitbox", &CONFIG_GET(int, g_Variables.m_AimBot.m_iHitbox), hitboxNames, 3, 170.f);

        UI::Gap(4.f);
        UI::Toggle(X("Jamoani e'tiborsiz"), &CONFIG_GET(bool, g_Variables.m_AimBot.m_bIgnoreTeammates));
        UI::Col2();
        UI::Toggle(X("FOV doirasini chizish"), &CONFIG_GET(bool, g_Variables.m_AimBot.m_bDrawFOV));
    }
    UI::EndCard();

    UI::BeginCard(X("AIMBOT REJIMI  ·  V2.0"));
    {
        static const char* aimModes[] = { "Klassik  (eski, tez)", "Xavfsiz  (anti-ban)" };
        UI::Combo(X("Rejim"), "##aimmode", &CONFIG_GET(int, g_Variables.m_AimBot.m_iAimMode), aimModes, 2, 280.f);

        int iCurrentMode = CONFIG_GET(int, g_Variables.m_AimBot.m_iAimMode);
        UI::Gap(5.f);

        if (iCurrentMode == 0)
        {
            UI::Notice(UI::COL_AMBER, X("KLASSIK REJIM — anti-ban himoyasi o'chirilgan."));
            UI::Hint(X("Delay yo'q, jitter yo'q, visibility check yo'q. Overwatch'da ko'rinishi mumkin."));
        }
        else
        {
            UI::Notice(UI::COL_GREEN, X("XAVFSIZ REJIM — anti-ban himoyasi yoqilgan."));

            UI::Gap(5.f);
            UI::Toggle(X("Devordan aim qilmasin (visibility check)"), &CONFIG_GET(bool, g_Variables.m_AimBot.m_bVisibilityCheck));
            UI::Hint(X("Faqat ko'rinadigan dushmanlarga aim qiladi."));

            UI::Gap(5.f);
            UI::SliderF(X("Min reaktsiya"), "##aimrmin", &CONFIG_GET(float, g_Variables.m_AimBot.m_flReactionTimeMin), 0.f, 500.f, "%.0f ms", 220.f);
            UI::Col2();
            UI::SliderF(X("Max reaktsiya"), "##aimrmax", &CONFIG_GET(float, g_Variables.m_AimBot.m_flReactionTimeMax), 0.f, 500.f, "%.0f ms", 220.f);
            UI::Hint(X("Yangi nishonga o'tishdan oldin kutish — odam reaktsiyasi."));

            UI::Gap(5.f);
            UI::SliderF(X("Maks aim vaqti"), "##aimmax", &CONFIG_GET(float, g_Variables.m_AimBot.m_flMaxAimTime), 500.f, 10000.f, "%.0f ms", 240.f);
            UI::SliderF(X("Aim xatoligi"), "##aimjit", &CONFIG_GET(float, g_Variables.m_AimBot.m_flAimJitter), 0.f, 10.f, "%.1f px", 240.f);
            UI::SliderF(X("Kill kutish"), "##aimkill", &CONFIG_GET(float, g_Variables.m_AimBot.m_flKillDelay), 0.f, 1500.f, "%.0f ms", 240.f);
            UI::Hint(X("Tasodifiy xatolik va pauzalar 100%% aniqlikni yashiradi."));
        }
    }
    UI::EndCard();

    UI::BeginCard(X("RECOIL CONTROL  ·  RCS"));
    {
        bool& bRcs = CONFIG_GET(bool, g_Variables.m_RCS.m_bEnable);
        UI::Toggle(X("RCS yoqish"), &bRcs);
        if (bRcs)
        {
            UI::Hint(X("Aimbot yoqilgan paytda ishlamaydi — mustaqil tortadi."));
            UI::Gap(4.f);
            UI::SliderF(X("X (yonga) kuch"), "##rcsx", &CONFIG_GET(float, g_Variables.m_RCS.m_flScaleX), 0.f, 2.0f, "%.2f", 220.f);
            UI::Col2();
            UI::SliderF(X("Y (pastga) kuch"), "##rcsy", &CONFIG_GET(float, g_Variables.m_RCS.m_flScaleY), 0.f, 2.0f, "%.2f", 220.f);
        }
    }
    UI::EndCard();
}

// =======================================================================
//  PAGE :: RADAR
// =======================================================================
static void PageRadar()
{
    UI::BeginCard(X("2D RADAR  ·  OVERLAY"));
    {
        bool& bRadar = CONFIG_GET(bool, g_Variables.m_Radar.m_bEnableRadar);
        UI::Toggle(X("Radar yoqish"), &bRadar);

        if (bRadar)
        {
            UI::Gap(4.f);
            UI::SliderF(X("Kattaligi"), "##radsize", &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarSize), 100.f, 400.f, "%.0f px", 210.f);
            UI::Col2();
            UI::SliderF(X("Masofasi"), "##radrange", &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarRange), 500.f, 5000.f, "%.0f u", 210.f);

            UI::SliderF(X("X pozitsiya"), "##radx", &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarX), 0.f, 1920.f, "%.0f", 210.f);
            UI::Col2();
            UI::SliderF(X("Y pozitsiya"), "##rady", &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarY), 0.f, 1080.f, "%.0f", 210.f);

            UI::Gap(4.f);
            UI::Toggle(X("Qarash bilan aylantirish"), &CONFIG_GET(bool, g_Variables.m_Radar.m_bRadarRotate));
        }
    }
    UI::EndCard();

    UI::BeginCard(X("O'YIN RADARI  ·  IN-GAME"));
    {
        UI::Toggle(X("O'yin radarida ko'rsatish"), &CONFIG_GET(bool, g_Variables.m_Radar.m_bInGameRadar));
        UI::Hint(X("Dushmanlarni o'yinning chap yuqoridagi asl radarida ochib beradi."));
    }
    UI::EndCard();

    UI::BeginCard(X("OVOZLI RADAR  ·  SONAR"));
    {
        bool& bSonar = CONFIG_GET(bool, g_Variables.m_Misc.m_bEnableSonar);
        UI::Toggle(X("Ovozli radarni yoqish"), &bSonar);
        UI::Hint(X("Dushmanga qaraganingizda devor ortidan 'piip' ovozi chiqaradi."));
        if (bSonar)
        {
            UI::Gap(3.f);
            UI::SliderF(X("Sezuvchanlik radiusi"), "##sonarfov", &CONFIG_GET(float, g_Variables.m_Misc.m_flSonarFOV), 1.f, 20.f, "%.1f deg", 240.f);
        }
    }
    UI::EndCard();
}

// =======================================================================
//  PAGE :: INVENTAR  (skin changer)
// =======================================================================
static Skins::SkinEntry_t* g_pSelectedSkin   = nullptr;
static int                 g_nSelectedWeapon = -1;
static float               g_flSelWear       = 0.0001f;
static int                 g_nSelSeed        = 0;
static int                 g_nSelStatTrak    = -1;

static const char* KindName(int eKind)
{
    return (eKind == Skins::KIND_KNIFE) ? "PICHOQ"
         : (eKind == Skins::KIND_GLOVE) ? "QO'LQOP"
                                        : "QUROL";
}

// Bitta skin kartochkasi. Bosilsa — tanlanadi.
static bool SkinCard(Skins::SkinEntry_t* pSkin, ImVec2 size, bool bSelected)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    if (win->SkipItems) return false;

    const ImGuiID id  = win->GetID(pSkin);
    ImVec2        pos = win->DC.CursorPos;
    ImRect        bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));

    ImGui::ItemSize(bb, 0.f);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    // ko'rinib turgan kartochkalar uchungina surat so'raymiz
    if (ImGui::IsRectVisible(bb.Min, bb.Max))
        Skins::RequestImage(pSkin);

    bool hovered = false, held = false;
    const bool bPressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const float t = UI::Anim(id, bSelected ? 1.f : (hovered ? 0.5f : 0.f), 14.f);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(bb.Min, bb.Max, UI::Mix(IM_COL32(13, 17, 26, 255), IM_COL32(20, 28, 42, 255), t), 4.f);
    dl->AddRect(bb.Min, bb.Max, UI::Mix(IM_COL32(28, 38, 54, 255), UI::COL_CYAN, t), 4.f, 0, 1.f);

    // rarity chizig'i
    dl->AddRectFilled(ImVec2(bb.Min.x + 1.f, bb.Max.y - 3.f), ImVec2(bb.Max.x - 1.f, bb.Max.y - 1.f),
        pSkin->m_colRarity, 1.f);

    // surat
    const float flImgH = size.y - 30.f;
    const ImVec2 imgMin(bb.Min.x + 4.f, bb.Min.y + 3.f);
    const ImVec2 imgMax(bb.Max.x - 4.f, bb.Min.y + 3.f + flImgH);

    const int nState = pSkin->m_nImgState.load();
    if (nState == Skins::IMG_READY && pSkin->m_pTexture)
    {
        // nisbatni saqlab joylashtiramiz
        const float flAspect = (pSkin->m_nTexH > 0) ? (float)pSkin->m_nTexW / (float)pSkin->m_nTexH : 1.33f;
        float w = imgMax.x - imgMin.x;
        float h = w / flAspect;
        if (h > flImgH) { h = flImgH; w = h * flAspect; }
        const ImVec2 c((imgMin.x + imgMax.x) * 0.5f, (imgMin.y + imgMax.y) * 0.5f);
        dl->AddImage(pSkin->m_pTexture, ImVec2(c.x - w * 0.5f, c.y - h * 0.5f), ImVec2(c.x + w * 0.5f, c.y + h * 0.5f));
    }
    else if (nState == Skins::IMG_FAILED)
    {
        dl->AddText(ImVec2(imgMin.x + 6.f, (imgMin.y + imgMax.y) * 0.5f - 7.f), UI::COL_TEXT_FAINT, "surat yo'q");
    }
    else
    {
        const float a = 0.25f + 0.35f * UI::Pulse(3.f);
        dl->AddRectFilled(imgMin, imgMax, UI::Fade(IM_COL32(30, 40, 56, 255), a), 3.f);
    }

    // nomi
    if (Fonts::Small) ImGui::PushFont(Fonts::Small);
    {
        std::string strName = pSkin->m_strShort;
        const float flMaxW  = size.x - 10.f;
        while (!strName.empty() && ImGui::CalcTextSize(strName.c_str()).x > flMaxW)
            strName.pop_back();
        dl->AddText(ImVec2(bb.Min.x + 5.f, bb.Max.y - 24.f), bSelected ? UI::COL_CYAN : UI::COL_TEXT, strName.c_str());

        if (!pSkin->m_strRarity.empty())
            dl->AddText(ImVec2(bb.Min.x + 5.f, bb.Max.y - 12.f), UI::Fade(pSkin->m_colRarity, 0.85f), pSkin->m_strRarity.c_str());
    }
    if (Fonts::Small) ImGui::PopFont();

    if (hovered)
        ImGui::SetTooltip("%s\npaint kit: %d", pSkin->m_strName.c_str(), pSkin->m_nPaintKit);

    return bPressed;
}

static void ApplySelectedSkin()
{
    if (!g_pSelectedSkin) return;

    Skins::SkinConfig_t cfg;
    cfg.m_nPaintKit = g_pSelectedSkin->m_nPaintKit;
    cfg.m_flWear    = ImClamp(g_flSelWear, g_pSelectedSkin->m_flMinFloat, g_pSelectedSkin->m_flMaxFloat);
    cfg.m_nSeed     = g_nSelSeed;
    cfg.m_nStatTrak = g_nSelStatTrak;
    cfg.m_bLegacy   = g_pSelectedSkin->m_bLegacy;
    cfg.m_nDefIndex = g_pSelectedSkin->m_nWeaponId;
    cfg.m_strName   = g_pSelectedSkin->m_strName;

    {
        std::lock_guard<std::mutex> lock(Skins::g_mtxConfig);
        if (g_pSelectedSkin->m_eKind == Skins::KIND_KNIFE)
        {
            Skins::g_KnifeConfig    = cfg;
            Skins::g_nKnifeDefIndex = cfg.m_nDefIndex;
        }
        else if (g_pSelectedSkin->m_eKind == Skins::KIND_GLOVE)
        {
            Skins::g_GloveConfig    = cfg;
            Skins::g_nGloveDefIndex = cfg.m_nDefIndex;
        }
        else
        {
            Skins::g_mapWeapons[cfg.m_nDefIndex] = cfg;
        }
    }

    Skins::g_bEnabled = true;
    Skins::ForceReapply();
}

static void PageInventory()
{
    Skins::PumpTextures();

    // ---------------- yuqori panel ----------------
    UI::BeginCard(X("SKIN CHANGER"));
    {
        UI::Toggle(X("Skin o'zgartirishni yoqish"), &Skins::g_bEnabled);
        UI::Col2();
        UI::Toggle(X("Weapon subclass (eksperimental)"), &Skins::g_bSubclass);

        UI::Gap(3.f);
        ImVec2 p = ImGui::GetCursorScreenPos();
        const bool bReady = Skins::g_bReady.load();
        UI::Chip(bReady ? X("baza: tayyor") : X("baza: yuklanmoqda"),
            bReady ? UI::COL_GREEN : UI::COL_AMBER, p);
        const float w1 = UI::ChipWidth(bReady ? "baza: tayyor" : "baza: yuklanmoqda");
        UI::Chip(Skins::g_strDbStatus.c_str(), UI::COL_TEXT_MUTE, ImVec2(p.x + w1 + 8.f, p.y));
        ImGui::Dummy(ImVec2(0.f, ImGui::GetTextLineHeight() + 8.f));

        UI::TextC(UI::COL_TEXT_MUTE, X("Holat: %s"), Skins::g_strStatus.c_str());
    }
    UI::EndCard();

    if (!Skins::g_bReady.load())
    {
        UI::Gap(6.f);
        UI::Notice(UI::COL_AMBER, X("Skinlar bazasi yuklanmoqda — internet kerak (birinchi marta)."));
        return;
    }

    // ---------------- uch ustun ----------------
    const float flAvailW = ImGui::GetContentRegionAvail().x;
    const float flAvailH = ImGui::GetContentRegionAvail().y - 6.f;
    const float flLeftW  = 190.f;
    const float flRightW = 264.f;
    const float flMidW   = flAvailW - flLeftW - flRightW - 20.f;

    if (flMidW < 200.f || flAvailH < 200.f)
    {
        UI::Notice(UI::COL_AMBER, X("Oyna juda kichik."));
        return;
    }

    static char szWeaponFilter[64] = "";
    static char szSkinFilter[64]   = "";

    // ============ 1. QUROLLAR ============
    ImGui::BeginChild(X("##skins_weapons"), ImVec2(flLeftW, flAvailH), false);
    {
        UI::FieldLabel(X("QUROLLAR"));
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputTextWithHint(X("##wfilter"), X("qidirish..."), szWeaponFilter, sizeof(szWeaponFilter));
        UI::Gap(3.f);

        ImGui::BeginChild(X("##wlist"), ImVec2(0.f, 0.f), false);
        {
            std::string strFilter = szWeaponFilter;
            std::transform(strFilter.begin(), strFilter.end(), strFilter.begin(), ::tolower);

            int nLastKind = -1;
            for (int i = 0; i < (int)Skins::g_vecWeapons.size(); i++)
            {
                const Skins::WeaponGroup_t& group = Skins::g_vecWeapons[i];

                if (!strFilter.empty())
                {
                    std::string strLower = group.m_strName;
                    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
                    if (strLower.find(strFilter) == std::string::npos)
                        continue;
                }

                if (group.m_eKind != nLastKind)
                {
                    nLastKind = group.m_eKind;
                    UI::Gap(2.f);
                    UI::TextC(UI::COL_TEXT_FAINT, "%s", KindName(group.m_eKind));
                }

                char szLabel[96];
                snprintf(szLabel, sizeof(szLabel), "%s##w%d", group.m_strName.c_str(), group.m_nDefIndex);

                bool bHasSkin = false;
                {
                    std::lock_guard<std::mutex> lock(Skins::g_mtxConfig);
                    bHasSkin = (group.m_eKind == Skins::KIND_KNIFE) ? (Skins::g_nKnifeDefIndex == group.m_nDefIndex)
                             : (group.m_eKind == Skins::KIND_GLOVE) ? (Skins::g_nGloveDefIndex == group.m_nDefIndex)
                             : (Skins::g_mapWeapons.count(group.m_nDefIndex) > 0);
                }

                if (bHasSkin) ImGui::PushStyleColor(ImGuiCol_Text, UI::V4(UI::COL_CYAN));
                if (ImGui::Selectable(szLabel, g_nSelectedWeapon == i))
                {
                    g_nSelectedWeapon = i;
                    g_pSelectedSkin   = nullptr;
                }
                if (bHasSkin) ImGui::PopStyleColor();

                // skinlar soni
                char szCount[16];
                snprintf(szCount, sizeof(szCount), "%d", (int)group.m_vecSkins.size());
                ImVec2 ts = ImGui::CalcTextSize(szCount);
                ImGui::GetWindowDrawList()->AddText(
                    ImVec2(ImGui::GetItemRectMax().x - ts.x - 6.f, ImGui::GetItemRectMin().y),
                    UI::COL_TEXT_FAINT, szCount);
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::SameLine(0.f, 10.f);

    // ============ 2. SKINLAR ============
    ImGui::BeginChild(X("##skins_grid"), ImVec2(flMidW, flAvailH), false);
    {
        UI::FieldLabel(X("SKINLAR"));
        ImGui::SetNextItemWidth(-1.f);
        ImGui::InputTextWithHint(X("##sfilter"), X("skin nomi bo'yicha qidirish..."), szSkinFilter, sizeof(szSkinFilter));
        UI::Gap(3.f);

        ImGui::BeginChild(X("##slist"), ImVec2(0.f, 0.f), false);
        {
            if (g_nSelectedWeapon < 0 || g_nSelectedWeapon >= (int)Skins::g_vecWeapons.size())
            {
                UI::Hint(X("Chapdan qurol tanlang."));
            }
            else
            {
                const Skins::WeaponGroup_t& group = Skins::g_vecWeapons[g_nSelectedWeapon];

                std::string strFilter = szSkinFilter;
                std::transform(strFilter.begin(), strFilter.end(), strFilter.begin(), ::tolower);

                const float flCardW = 132.f;
                const float flCardH = 112.f;
                const int   nPerRow = ImMax(1, (int)((ImGui::GetContentRegionAvail().x + 8.f) / (flCardW + 8.f)));

                int nDrawn = 0;
                for (Skins::SkinEntry_t* pSkin : group.m_vecSkins)
                {
                    if (!strFilter.empty())
                    {
                        std::string strLower = pSkin->m_strShort;
                        std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
                        if (strLower.find(strFilter) == std::string::npos)
                            continue;
                    }

                    if (nDrawn % nPerRow != 0)
                        ImGui::SameLine(0.f, 8.f);

                    if (SkinCard(pSkin, ImVec2(flCardW, flCardH), g_pSelectedSkin == pSkin))
                    {
                        g_pSelectedSkin = pSkin;
                        g_flSelWear     = ImClamp(0.0001f, pSkin->m_flMinFloat, pSkin->m_flMaxFloat);
                        g_nSelSeed      = 0;
                        g_nSelStatTrak  = -1;
                    }

                    nDrawn++;
                }

                if (nDrawn == 0)
                    UI::Hint(X("Hech narsa topilmadi."));
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();

    ImGui::SameLine(0.f, 10.f);

    // ============ 3. TANLANGAN ============
    ImGui::BeginChild(X("##skins_selected"), ImVec2(flRightW, flAvailH), false);
    {
        UI::FieldLabel(X("TANLANGAN"));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        const float w = ImGui::GetContentRegionAvail().x;

        // preview
        {
            const float h = 96.f;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(11, 15, 23, 255), 4.f);
            dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(28, 38, 54, 255), 4.f, 0, 1.f);

            if (g_pSelectedSkin)
            {
                Skins::RequestImage(g_pSelectedSkin);

                if (g_pSelectedSkin->m_nImgState.load() == Skins::IMG_READY && g_pSelectedSkin->m_pTexture)
                {
                    const float flAspect = (g_pSelectedSkin->m_nTexH > 0)
                        ? (float)g_pSelectedSkin->m_nTexW / (float)g_pSelectedSkin->m_nTexH : 1.33f;
                    float iw = w - 16.f;
                    float ih = iw / flAspect;
                    if (ih > h - 12.f) { ih = h - 12.f; iw = ih * flAspect; }
                    const ImVec2 c(p.x + w * 0.5f, p.y + h * 0.5f);
                    dl->AddImage(g_pSelectedSkin->m_pTexture,
                        ImVec2(c.x - iw * 0.5f, c.y - ih * 0.5f), ImVec2(c.x + iw * 0.5f, c.y + ih * 0.5f));
                }

                dl->AddRectFilled(ImVec2(p.x + 1.f, p.y + h - 3.f), ImVec2(p.x + w - 1.f, p.y + h - 1.f),
                    g_pSelectedSkin->m_colRarity, 1.f);
            }
            else
            {
                const char* szNone = X("hech narsa tanlanmagan");
                ImVec2 ts = ImGui::CalcTextSize(szNone);
                dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + h * 0.5f - 8.f), UI::COL_TEXT_FAINT, szNone);
            }
            ImGui::Dummy(ImVec2(w, h + 6.f));
        }

        if (!g_pSelectedSkin)
        {
            UI::Hint(X("O'rtadagi ro'yxatdan skin tanlang."));
        }
        else
        {
            UI::TextC(UI::COL_TEXT, "%s", g_pSelectedSkin->m_strShort.c_str());
            UI::TextC(UI::COL_TEXT_MUTE, "%s  ·  %s", g_pSelectedSkin->m_strWeapon.c_str(), KindName(g_pSelectedSkin->m_eKind));

            UI::Gap(5.f);
            UI::FieldLabel(X("SIFAT (float)"));
            ImGui::SetNextItemWidth(-1.f);
            ImGui::SliderFloat(X("##wear"), &g_flSelWear,
                g_pSelectedSkin->m_flMinFloat, g_pSelectedSkin->m_flMaxFloat, "%.4f");

            // tez tugmalar
            struct WearPreset_t { const char* szName; float flValue; };
            static const WearPreset_t kPresets[] = {
                { "FN", 0.02f }, { "MW", 0.10f }, { "FT", 0.25f }, { "WW", 0.41f }, { "BS", 0.60f }
            };
            for (int i = 0; i < 5; i++)
            {
                if (i > 0) ImGui::SameLine(0.f, 4.f);
                char szId[24];
                snprintf(szId, sizeof(szId), "%s##wp%d", kPresets[i].szName, i);
                if (UI::Button(szId, ImVec2(44.f, 22.f)))
                    g_flSelWear = ImClamp(kPresets[i].flValue, g_pSelectedSkin->m_flMinFloat, g_pSelectedSkin->m_flMaxFloat);
            }

            UI::Gap(6.f);
            UI::FieldLabel(X("PATTERN (seed)"));
            ImGui::SetNextItemWidth(110.f);
            ImGui::InputInt(X("##seed"), &g_nSelSeed);
            g_nSelSeed = ImClamp(g_nSelSeed, 0, 1000);

            UI::Gap(4.f);
            bool bStatTrak = (g_nSelStatTrak >= 0);
            if (UI::Toggle(X("StatTrak"), &bStatTrak))
                g_nSelStatTrak = bStatTrak ? 0 : -1;
            if (g_nSelStatTrak >= 0)
            {
                ImGui::SameLine(0.f, 10.f);
                ImGui::SetNextItemWidth(90.f);
                ImGui::InputInt(X("##st"), &g_nSelStatTrak);
                if (g_nSelStatTrak < 0) g_nSelStatTrak = 0;
            }

            UI::Gap(8.f);
            if (UI::Button(X("QO'LLASH"), ImVec2(-1.f, 34.f), UI::BTN_PRIMARY))
                ApplySelectedSkin();
        }

        UI::Gap(6.f);
        if (UI::Button(X("QAYTA QO'LLASH"), ImVec2(-1.f, 26.f)))
            Skins::ForceReapply();

        UI::Gap(4.f);
        if (UI::Button(X("HAMMASINI TOZALASH"), ImVec2(-1.f, 26.f), UI::BTN_DANGER))
        {
            Skins::ClearAll();
            g_pSelectedSkin = nullptr;
        }

        // ---- faol skinlar ----
        UI::Gap(8.f);
        {
            std::lock_guard<std::mutex> lock(Skins::g_mtxConfig);
            const int nActive = (int)Skins::g_mapWeapons.size()
                              + (Skins::g_nKnifeDefIndex != 0 ? 1 : 0)
                              + (Skins::g_nGloveDefIndex != 0 ? 1 : 0);

            UI::FieldLabel(X("FAOL SKINLAR"));
            if (nActive == 0)
            {
                UI::Hint(X("Hali hech narsa qo'llanmagan."));
            }
            else
            {
                if (Fonts::Small) ImGui::PushFont(Fonts::Small);
                if (Skins::g_nKnifeDefIndex != 0)
                    UI::TextC(UI::COL_CYAN, "%s", Skins::g_KnifeConfig.m_strName.c_str());
                if (Skins::g_nGloveDefIndex != 0)
                    UI::TextC(UI::COL_CYAN, "%s", Skins::g_GloveConfig.m_strName.c_str());
                for (const auto& entry : Skins::g_mapWeapons)
                    UI::TextC(UI::COL_TEXT_MUTE, "%s", entry.second.m_strName.c_str());
                if (Fonts::Small) ImGui::PopFont();
            }
        }
    }
    ImGui::EndChild();
}

// =======================================================================
//  PAGE :: KONFIG
// =======================================================================
static void PageConfigs()
{
    UI::BeginCard(X("SOZLAMALAR BOSHQARUVI  ·  CONFIGS"));
    {
        static int nSelected = -1;
        static std::string strCfgName;

        float flAvail = ImGui::GetContentRegionAvail().x;
        float flListW = ImMin(280.f, flAvail * 0.5f);

        ImGui::BeginGroup();
        {
            UI::FieldLabel(X("Saqlangan configlar"));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, UI::V4(IM_COL32(8, 11, 17, 255)));
            if (ImGui::BeginListBox(X("##CfgList"), ImVec2(flListW, 210.f)))
            {
                for (size_t i = 0; i < Config::m_vecFileNames.size(); i++)
                {
                    if (ImGui::Selectable(Config::m_vecFileNames.at(i).c_str(),
                        i == (size_t)nSelected, ImGuiSelectableFlags_DontClosePopups))
                        nSelected = (int)i;
                }
                ImGui::EndListBox();
            }
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();

        ImGui::SameLine(0.f, 16.f);

        ImGui::BeginGroup();
        {
            UI::FieldLabel(X("Yangi config"));
            ImGui::SetNextItemWidth(220.f);
            ImGui::InputTextWithHint(X("##cfgfile"), X("config nomi..."), &strCfgName);

            UI::Gap(4.f);
            if (UI::Button(X("YARATISH"), ImVec2(140.f, 0.f), UI::BTN_PRIMARY))
            {
                Config::Save(strCfgName);
                strCfgName.clear();
                Config::Refresh();
            }
            ImGui::SameLine(0.f, 8.f);
            if (UI::Button(X("YANGILASH"), ImVec2(140.f, 0.f)))
                Config::Refresh();

            if (nSelected >= 0 && nSelected < (int)Config::m_vecFileNames.size())
            {
                UI::Gap(8.f);
                UI::TextC(UI::COL_TEXT_MUTE, X("Tanlangan: %s"), Config::m_vecFileNames.at(nSelected).c_str());
                UI::Gap(3.f);

                if (UI::Button(X("SAQLASH"), ImVec2(140.f, 0.f), UI::BTN_SUCCESS))
                    Config::Save(Config::m_vecFileNames.at(nSelected));
                ImGui::SameLine(0.f, 8.f);
                if (UI::Button(X("YUKLASH"), ImVec2(140.f, 0.f), UI::BTN_PRIMARY))
                    Config::Load(Config::m_vecFileNames.at(nSelected));

                UI::Gap(3.f);
                if (UI::Button(X("O'CHIRISH"), ImVec2(140.f, 0.f), UI::BTN_DANGER))
                {
                    Config::Remove(Config::m_vecFileNames.at(nSelected));
                    Config::Refresh();
                    nSelected = -1;
                }
            }
            else
            {
                UI::Gap(8.f);
                UI::Hint(X("Saqlash / yuklash uchun chapdan config tanlang."));
            }
        }
        ImGui::EndGroup();
    }
    UI::EndCard();
}

// =======================================================================
//  PAGE :: TIZIM
// =======================================================================
static void PageSystem()
{
    UI::BeginCard(X("ASOSIY  ·  CORE"));
    {
        UI::Keybind(X("Menyu tugmasi"), CONFIG_GET(int, g_Variables.m_Gui.m_iMenuKey), "menukey");
        UI::Gap(4.f);
        UI::Keybind(X("Yopish tugmasi"), CONFIG_GET(int, g_Variables.m_Gui.m_iUnloadKey), "unloadkey");

        UI::Gap(6.f);
        UI::Toggle(X("Ekran yozishdan yashirish"), &CONFIG_GET(bool, g_Variables.m_Gui.m_bExcludeFromDesktopCapture));
        UI::Hint(X("Stream / screenshot dasturlariga menyu ko'rinmaydi."));
    }
    UI::EndCard();

    UI::BeginCard(X("QO'SHIMCHA  ·  UTILITY"));
    {
        UI::Toggle(X("Avtomatik qabul qilish (auto-accept)"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bAutoAccept));
        UI::Hint(X("Match topilganda orqa fonda markaziy tugmani bosadi."));

        UI::Gap(5.f);
        UI::Toggle(X("Flash himoya"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bAntiFlash));
        UI::Col2();
        UI::Toggle(X("Watermark"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bWatermark));

        UI::Toggle(X("C4 timer + damage"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bC4Timer));
        UI::Col2();
        UI::Toggle(X("Granata xavfi"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bGrenadeWarning));

        UI::Toggle(X("Sniper crosshair"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bSniperCrosshair));
        UI::Col2();
        UI::Toggle(X("Tomosha qiluvchilar"), &CONFIG_GET(bool, g_Variables.m_SpectatorList.m_bEnableSpectatorList));

        UI::Hint(X("Sniper crosshair: AWP / SSG08 scope ochilmagan holatda markazni chizadi."));
    }
    UI::EndCard();

    UI::BeginCard(X("OVOZ  ·  AUDIO"));
    {
        UI::Toggle(X("Hit sound (tegish ovozi)"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bHitSound));
        UI::Col2();
        UI::Toggle(X("Kill sound (o'ldirish ovozi)"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bKillSound));

        UI::Gap(4.f);
        UI::SliderF(X("Ovoz balandligi"), "##sndvol", &CONFIG_GET(float, g_Variables.m_Misc.m_flSoundVolume), 0.f, 100.f, "%.0f%%", 240.f);
        UI::Hint(X("hit_sound.wav va kill_sound.wav fayllarini exe yoniga qo'ying."));

        UI::Gap(4.f);
        if (UI::Button(X("TEST HIT"), ImVec2(120.f, 0.f), UI::BTN_SUCCESS))
            PlaySoundA("C:\\Windows\\Media\\Windows Default.wav", NULL, SND_ASYNC | SND_FILENAME);
        ImGui::SameLine(0.f, 8.f);
        if (UI::Button(X("TEST KILL"), ImVec2(120.f, 0.f), UI::BTN_SUCCESS))
            PlaySoundA("C:\\Windows\\Media\\Windows Default.wav", NULL, SND_ASYNC | SND_FILENAME);
    }
    UI::EndCard();

    UI::BeginCard(X("LITSENZIYA  ·  LICENSE"));
    {
        ImU32 colTier = U32(g_License.GetTierColor());

        ImVec2 p = ImGui::GetCursorScreenPos();
        UI::Chip(g_License.GetTierName(), colTier, p);
        ImGui::Dummy(ImVec2(UI::ChipWidth(g_License.GetTierName()) + 10.f, ImGui::GetTextLineHeight() + 6.f));
        ImGui::SameLine(0.f, 0.f);
        UI::TextC(UI::COL_TEXT, "%s", g_License.m_strUser.c_str());

        UI::TextC(UI::COL_TEXT_MUTE, X("Holat: %s"), g_License.m_strExpiry.c_str());

        UI::Gap(4.f);
        UI::Hint(X("Kalit formati:  SH-XXXXXXXX-M (MID)  yoki  SH-XXXXXXXX-P (PRO)"));
        UI::Hint(X("Kalit olish uchun admin: @bakoev_71"));
    }
    UI::EndCard();

    UI::BeginCard(X("YANGILANISH  ·  UPDATE"));
    {
        UI::TextC(UI::COL_TEXT_MUTE, X("Joriy versiya: v%s"), SHIFTHUB_VERSION);

        if (g_Updater.m_bUpdateAvailable)
        {
            UI::Gap(3.f);
            UI::Notice(UI::COL_GREEN, X("Yangi versiya mavjud: v%s"), g_Updater.m_strLatestVersion.c_str());

            if (!g_Updater.m_strChangelog.empty())
            {
                UI::Gap(2.f);
                UI::Hint(X("O'zgarishlar: %s"), g_Updater.m_strChangelog.c_str());
            }

            UI::Gap(5.f);

            if (g_Updater.m_bDownloading)
            {
                // custom neon progress bar
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetCursorScreenPos();
                float  w = ImMin(340.f, ImGui::GetContentRegionAvail().x);
                float  h = 18.f;
                dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(10, 14, 21, 255), 3.f);
                dl->AddRect(p, ImVec2(p.x + w, p.y + h), UI::Fade(UI::COL_CYAN, 0.5f), 3.f, 0, 1.f);
                float fill = ImClamp(g_Updater.m_flProgress, 0.f, 1.f) * (w - 4.f);
                if (fill > 0.f)
                {
                    dl->AddRectFilledMultiColor(ImVec2(p.x + 2.f, p.y + 2.f), ImVec2(p.x + 2.f + fill, p.y + h - 2.f),
                        UI::COL_TEAL, UI::COL_CYAN, UI::COL_CYAN, UI::COL_TEAL);
                    UI::GlowRect(dl, ImVec2(p.x + 2.f, p.y + 2.f), ImVec2(p.x + 2.f + fill, p.y + h - 2.f), UI::COL_CYAN, 2.f, 4, 1.f);
                }
                char szPct[16];
                snprintf(szPct, sizeof(szPct), "%d%%", (int)(ImClamp(g_Updater.m_flProgress, 0.f, 1.f) * 100.f));
                ImVec2 ts = ImGui::CalcTextSize(szPct);
                dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), UI::COL_TEXT, szPct);
                ImGui::Dummy(ImVec2(w, h + 4.f));

                UI::TextC(UI::COL_AMBER, "%s", g_Updater.m_strStatusText.c_str());
            }
            else if (g_Updater.m_bDownloadComplete)
            {
                if (UI::Button(X("O'RNATISH VA QAYTA ISHGA TUSHIRISH"), ImVec2(340.f, 36.f), UI::BTN_SUCCESS))
                    g_Updater.ApplyUpdate();
                UI::Gap(2.f);
                UI::TextC(UI::COL_GREEN, X("Tayyor! Bosing va dastur yangilanadi."));
            }
            else if (g_Updater.m_bDownloadFailed)
            {
                UI::Notice(UI::COL_RED, "%s", g_Updater.m_strStatusText.c_str());
                UI::Gap(3.f);
                if (UI::Button(X("QAYTA URINISH"), ImVec2(180.f, 0.f), UI::BTN_WARN))
                    g_Updater.StartDownload();
            }
            else
            {
                if (UI::Button(X("YANGILASH"), ImVec2(220.f, 36.f), UI::BTN_PRIMARY))
                    g_Updater.StartDownload();
            }
        }
        else
        {
            UI::Gap(2.f);
            UI::TextC(UI::COL_TEXT_FAINT, "%s", g_Updater.m_strStatusText.c_str());
        }

        UI::Gap(5.f);
        if (UI::Button(X("QAYTA TEKSHIRISH"), ImVec2(180.f, 0.f)))
            g_Updater.Recheck();
    }
    UI::EndCard();
}

// -----------------------------------------------------------------------
//  Fonts
// -----------------------------------------------------------------------
static ImFont* LoadFirstFont(ImGuiIO& io, const char* const* paths, int nPaths, float flSize, ImFontConfig* pCfg, const ImWchar* pRanges)
{
    for (int i = 0; i < nPaths; i++)
    {
        std::error_code ec;
        if (!std::filesystem::exists(paths[i], ec))
            continue;

        ImFont* pFont = io.Fonts->AddFontFromFileTTF(paths[i], flSize, pCfg, pRanges);
        if (pFont)
            return pFont;
    }
    return io.Fonts->AddFontDefault();
}

void Gui::Initialize(unsigned int uFontFlags)
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable imgui.ini generation

    UI::ApplyTheme();

    static const char* const kBody[]  = { "C:\\Windows\\Fonts\\segoeui.ttf", "C:\\Windows\\Fonts\\Verdana.ttf", "C:\\Windows\\Fonts\\tahoma.ttf" };
    static const char* const kBold[]  = { "C:\\Windows\\Fonts\\segoeuib.ttf", "C:\\Windows\\Fonts\\Verdanab.ttf", "C:\\Windows\\Fonts\\segoeui.ttf" };
    static const char* const kMono[]  = { "C:\\Windows\\Fonts\\consola.ttf", "C:\\Windows\\Fonts\\cour.ttf", "C:\\Windows\\Fonts\\Verdana.ttf" };
    static const char* const kESP[]   = { "C:\\Windows\\Fonts\\Verdana.ttf", "C:\\Windows\\Fonts\\segoeui.ttf" };

    const ImWchar* pRanges = UI::GlyphRanges();

    ImFontConfig cfgBody = {};
    cfgBody.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_LightHinting;

    ImFontConfig cfgBold = {};
    cfgBold.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_LightHinting
                             | ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bold;

    Fonts::Default = LoadFirstFont(io, kBody, IM_ARRAYSIZE(kBody), 17.f, &cfgBody, pRanges);
    Fonts::Small   = LoadFirstFont(io, kBody, IM_ARRAYSIZE(kBody), 13.f, &cfgBody, pRanges);
    Fonts::Title   = LoadFirstFont(io, kBold, IM_ARRAYSIZE(kBold), 25.f, &cfgBold, pRanges);
    Fonts::Mono    = LoadFirstFont(io, kMono, IM_ARRAYSIZE(kMono), 14.f, &cfgBold, pRanges);
    Fonts::ESP     = LoadFirstFont(io, kESP,  IM_ARRAYSIZE(kESP),  10.f, &cfgBold, pRanges);

    m_bInitialized = ImGuiFreeType::BuildFontAtlas(io.Fonts, uFontFlags);
}

// -----------------------------------------------------------------------
//  Update
// -----------------------------------------------------------------------
void Gui::Update(ImGuiIO& io)
{
    io.MouseDrawCursor = m_bOpen;
    if (m_bOpen)
    {
        POINT p;
        if (GetCursorPos(&p))
            io.MousePos = ImVec2(static_cast<float>(p.x), static_cast<float>(p.y));
    }
}

// =======================================================================
//  RENDER
// =======================================================================
struct NavEntry_t
{
    const char* m_szLabel;
    int         m_eIcon;
    void        (*m_pfnPage)();
    ETier       m_eTier;
};

void Gui::Render()
{
    if (!m_bInitialized)
        return;

    ImGuiIO& io = ImGui::GetIO();
    Gui::Update(io);

    // Auto-update: tekshirish (birinchi marta)
    static bool s_bUpdateChecked = false;
    if (!s_bUpdateChecked)
    {
        s_bUpdateChecked = true;
        g_Updater.CheckForUpdate();
    }

    static const NavEntry_t kNav[] =
    {
        { "VIZUAL",   UI::ICON_EYE,       &PageVisuals,   ETier::LITE },
        { "HARAKAT",  UI::ICON_MOVE,      &PageMovement,  ETier::MID  },
        { "JANG",     UI::ICON_CROSSHAIR, &PageCombat,    ETier::MID  },
        { "RADAR",    UI::ICON_RADAR,     &PageRadar,     ETier::LITE },
        { "INVENTAR", UI::ICON_CASE,      &PageInventory, ETier::LITE },
        { "KONFIG",   UI::ICON_DISK,      &PageConfigs,   ETier::LITE },
        { "TIZIM",    UI::ICON_GEAR,      &PageSystem,    ETier::LITE },
    };
    static const int kNavCount = IM_ARRAYSIZE(kNav);

    const float flMenuW   = 1180.f;
    const float flMenuH   = 760.f;
    const float flHeader  = 62.f;
    const float flFooter  = 30.f;
    const float flRailW   = 200.f;

    ImGui::SetNextWindowSize(ImVec2(flMenuW, flMenuH));
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::Begin(X("###kaelcheat_main"), nullptr,
        ImGuiWindowFlags_NoTitleBar      |
        ImGuiWindowFlags_NoResize        |
        ImGuiWindowFlags_NoScrollbar     |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse      |
        ImGuiWindowFlags_NoSavedSettings);

    const ImVec2 wp = ImGui::GetWindowPos();
    const ImVec2 ws = ImGui::GetWindowSize();
    ImDrawList*  dl = ImGui::GetWindowDrawList();

    // ==================== FRAME / BACKDROP ====================
    {
        ImVec2 mn = wp, mx = ImVec2(wp.x + ws.x, wp.y + ws.y);

        // body gradient
        dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y + flHeader), ImVec2(mx.x, mx.y),
            IM_COL32(8, 11, 17, 252), IM_COL32(8, 11, 17, 252),
            IM_COL32(5, 7, 12, 252),  IM_COL32(5, 7, 12, 252));

        // header band
        dl->AddRectFilled(mn, ImVec2(mx.x, mn.y + flHeader), UI::COL_BG_DEEP, 6.f, ImDrawFlags_RoundCornersTop);
        UI::Grid(dl, ImVec2(mn.x, mn.y), ImVec2(mx.x, mn.y + flHeader), IM_COL32(34, 226, 255, 9), 22.f);
        dl->AddRectFilledMultiColor(ImVec2(mn.x, mn.y + flHeader - 1.f), ImVec2(mx.x, mn.y + flHeader),
            UI::Fade(UI::COL_CYAN, 0.9f), UI::Fade(UI::COL_MAGENTA, 0.55f),
            UI::Fade(UI::COL_MAGENTA, 0.55f), UI::Fade(UI::COL_CYAN, 0.9f));

        // footer band
        dl->AddRectFilled(ImVec2(mn.x, mx.y - flFooter), mx, UI::COL_BG_DEEP, 6.f, ImDrawFlags_RoundCornersBottom);
        dl->AddLine(ImVec2(mn.x, mx.y - flFooter), ImVec2(mx.x, mx.y - flFooter), UI::Fade(UI::COL_CYAN, 0.25f), 1.f);

        // outer border + glow + brackets
        dl->AddRect(mn, mx, UI::Fade(UI::COL_CYAN, 0.45f), 6.f, 0, 1.2f);
        UI::GlowRect(dl, mn, mx, UI::COL_CYAN, 6.f, 6, 0.9f);
        UI::Brackets(dl, ImVec2(mn.x + 2.f, mn.y + 2.f), ImVec2(mx.x - 2.f, mx.y - 2.f), UI::COL_CYAN, 18.f, 1.8f);
    }

    // ==================== DRAG HANDLE ====================
    ImGui::SetCursorPos(ImVec2(0.f, 0.f));
    ImGui::InvisibleButton(X("##drag"), ImVec2(ws.x - 120.f, flHeader));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        ImGui::SetWindowPos(ImVec2(wp.x + io.MouseDelta.x, wp.y + io.MouseDelta.y));

    // ==================== HEADER CONTENT ====================
    {
        // logo mark
        ImVec2 c(wp.x + 32.f, wp.y + flHeader * 0.5f);
        float  pulse = 0.65f + 0.35f * UI::Pulse(2.2f);
        dl->AddNgon(c, 15.f, UI::Fade(UI::COL_CYAN, pulse), 6, 1.6f);
        dl->AddNgon(c, 10.f, UI::Fade(UI::COL_MAGENTA, 0.55f), 6, 1.f);
        UI::Icon(dl, c, 12.f, UI::ICON_BOLT, UI::Fade(UI::COL_CYAN, pulse));

        // brand:  KAEL_CHEAT  (ikki rangli wordmark)
        if (Fonts::Title) ImGui::PushFont(Fonts::Title);
        ImVec2 bs = ImGui::CalcTextSize("KAEL");
        dl->AddText(ImVec2(wp.x + 58.f, wp.y + 10.f), UI::COL_TEXT, "KAEL");
        dl->AddText(ImVec2(wp.x + 58.f + bs.x, wp.y + 10.f), UI::COL_MAGENTA, "_CHEAT");
        if (Fonts::Title) ImGui::PopFont();

        if (Fonts::Mono) ImGui::PushFont(Fonts::Mono);
        char szSub[96];
        snprintf(szSub, sizeof(szSub), "CS2 EXTERNAL   v%s", SHIFTHUB_VERSION);
        dl->AddText(ImVec2(wp.x + 60.f, wp.y + 37.f), UI::COL_TEXT_FAINT, szSub);
        if (Fonts::Mono) ImGui::PopFont();

        // right side: tier + user chips
        ImU32 colTier = U32(g_License.GetTierColor());
        const char* szTier = g_License.GetTierName();
        float wTier = UI::ChipWidth(szTier);
        float wUser = UI::ChipWidth(g_License.m_strUser.c_str());
        float xRight = wp.x + ws.x - 54.f;

        UI::Chip(g_License.m_strUser.c_str(), UI::COL_TEXT_MUTE, ImVec2(xRight - wUser, wp.y + 21.f));
        UI::Chip(szTier, colTier, ImVec2(xRight - wUser - wTier - 8.f, wp.y + 21.f));

        // close button
        ImGui::SetCursorPos(ImVec2(ws.x - 40.f, 18.f));
        ImGui::InvisibleButton(X("##close"), ImVec2(26.f, 26.f));
        bool bCloseHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked())
            Gui::m_bOpen = false;

        ImVec2 cmn = ImGui::GetItemRectMin(), cmx = ImGui::GetItemRectMax();
        ImU32 colX = bCloseHover ? UI::COL_RED : UI::COL_TEXT_MUTE;
        if (bCloseHover)
        {
            dl->AddRectFilled(cmn, cmx, UI::Fade(UI::COL_RED, 0.18f), 3.f);
            dl->AddRect(cmn, cmx, UI::Fade(UI::COL_RED, 0.7f), 3.f, 0, 1.f);
        }
        dl->AddLine(ImVec2(cmn.x + 8.f, cmn.y + 8.f), ImVec2(cmx.x - 8.f, cmx.y - 8.f), colX, 1.6f);
        dl->AddLine(ImVec2(cmx.x - 8.f, cmn.y + 8.f), ImVec2(cmn.x + 8.f, cmx.y - 8.f), colX, 1.6f);
    }

    // ==================== NAV RAIL ====================
    const float flBodyH = ws.y - flHeader - flFooter;

    ImGui::SetCursorPos(ImVec2(0.f, flHeader));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 12.f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, UI::V4(IM_COL32(7, 10, 16, 255)));
    if (ImGui::BeginChild(X("##nav"), ImVec2(flRailW, flBodyH), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysUseWindowPadding))
    {
        ImGui::Dummy(ImVec2(0.f, 2.f));
        for (int i = 0; i < kNavCount; i++)
        {
            bool bLocked = !g_License.HasFeature(kNav[i].m_eTier);
            if (UI::NavItem(kNav[i].m_szLabel, kNav[i].m_eIcon, Tabs::m_iCurrentTab == i, bLocked))
                Tabs::m_iCurrentTab = i;
        }

        // rail footer: live status
        float flStatusY = ImGui::GetWindowHeight() - 74.f;
        if (flStatusY > ImGui::GetCursorPosY())
            ImGui::SetCursorPosY(flStatusY);

        ImDrawList* rdl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float  w = ImGui::GetContentRegionAvail().x;

        rdl->AddLine(ImVec2(p.x, p.y), ImVec2(p.x + w, p.y), UI::Fade(UI::COL_CYAN, 0.20f), 1.f);

        float flGameFrameTime = g_Interfaces.m_GlobalVars.m_flFrameTime;
        float flFps = (flGameFrameTime > 0.0001f && flGameFrameTime < 1.f) ? (1.f / flGameFrameTime) : io.Framerate;
        ImU32 colFps = (flFps >= 120.f) ? UI::COL_GREEN : (flFps >= 60.f ? UI::COL_AMBER : UI::COL_RED);
        rdl->AddCircleFilled(ImVec2(p.x + 6.f, p.y + 18.f), 3.5f, colFps, 10);
        rdl->AddCircle(ImVec2(p.x + 6.f, p.y + 18.f), 6.f * (0.7f + 0.3f * UI::Pulse(3.f)), UI::Fade(colFps, 0.35f), 12, 1.f);

        char szFps[48];
        snprintf(szFps, sizeof(szFps), "%.0f FPS", flFps);
        rdl->AddText(ImVec2(p.x + 18.f, p.y + 11.f), UI::COL_TEXT_MUTE, szFps);

        char szKeys[64];
        snprintf(szKeys, sizeof(szKeys), "MENU  %s", UI::KeyName(CONFIG_GET(int, g_Variables.m_Gui.m_iMenuKey)));
        rdl->AddText(ImVec2(p.x + 2.f, p.y + 34.f), UI::COL_TEXT_FAINT, szKeys);

        char szKeys2[64];
        snprintf(szKeys2, sizeof(szKeys2), "EXIT  %s", UI::KeyName(CONFIG_GET(int, g_Variables.m_Gui.m_iUnloadKey)));
        rdl->AddText(ImVec2(p.x + 2.f, p.y + 50.f), UI::COL_TEXT_FAINT, szKeys2);
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    // vertical divider
    dl->AddLine(ImVec2(wp.x + flRailW, wp.y + flHeader), ImVec2(wp.x + flRailW, wp.y + flHeader + flBodyH),
        UI::Fade(UI::COL_CYAN, 0.22f), 1.f);

    // ==================== PAGE ====================
    ImGui::SetCursorPos(ImVec2(flRailW + 1.f, flHeader));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.f, 14.f));
    if (ImGui::BeginChild(X("##page"), ImVec2(ws.x - flRailW - 1.f, flBodyH), false,
        ImGuiWindowFlags_AlwaysUseWindowPadding))
    {
        // page-change fade
        static int   s_iLastTab = -1;
        static float s_flFade   = 1.f;
        if (s_iLastTab != Tabs::m_iCurrentTab)
        {
            s_iLastTab = Tabs::m_iCurrentTab;
            s_flFade   = 0.f;
        }
        s_flFade = ImMin(1.f, s_flFade + io.DeltaTime * 6.f);

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.25f + 0.75f * s_flFade);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (1.f - s_flFade) * 8.f);

        // page title strip
        {
            const NavEntry_t& nav = kNav[ImClamp(Tabs::m_iCurrentTab, 0, kNavCount - 1)];
            ImDrawList* pdl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();

            UI::Icon(pdl, ImVec2(p.x + 8.f, p.y + 9.f), 17.f, nav.m_eIcon, UI::COL_CYAN);
            if (Fonts::Mono) ImGui::PushFont(Fonts::Mono);
            pdl->AddText(ImVec2(p.x + 24.f, p.y + 2.f), UI::COL_TEXT, nav.m_szLabel);
            if (Fonts::Mono) ImGui::PopFont();

            char szPath[64];
            snprintf(szPath, sizeof(szPath), "// %02d / %02d", Tabs::m_iCurrentTab + 1, kNavCount);
            ImVec2 ts = ImGui::CalcTextSize(szPath);
            pdl->AddText(ImVec2(p.x + ImGui::GetContentRegionAvail().x - ts.x, p.y + 2.f), UI::COL_TEXT_FAINT, szPath);

            UI::NeonLine(pdl, ImVec2(p.x, p.y + 21.f), ImGui::GetContentRegionAvail().x, UI::Fade(UI::COL_CYAN, 0.45f), 1.f);
            ImGui::Dummy(ImVec2(0.f, 26.f));
        }

        if (Tabs::m_iCurrentTab >= 0 && Tabs::m_iCurrentTab < kNavCount && kNav[Tabs::m_iCurrentTab].m_pfnPage)
            kNav[Tabs::m_iCurrentTab].m_pfnPage();

        ImGui::PopStyleVar();
        ImGui::Dummy(ImVec2(0.f, 6.f));
    }
    ImGui::EndChild();
    ImGui::PopStyleVar();

    // ==================== STATUS BAR ====================
    {
        float y = wp.y + ws.y - flFooter;
        float ty = y + (flFooter - ImGui::GetTextLineHeight()) * 0.5f;

        dl->AddText(ImVec2(wp.x + 16.f, ty), UI::Fade(UI::COL_CYAN, 0.75f), "KAEL_CHEAT");

        const char* szMid = X("DELETE = panic  ·  END = chiqish");
        ImVec2 ms = ImGui::CalcTextSize(szMid);
        dl->AddText(ImVec2(wp.x + (ws.x - ms.x) * 0.5f, ty), UI::COL_TEXT_FAINT, szMid);

        char szClock[32] = "";
        {
            time_t t = time(nullptr);
            tm tmNow{};
            if (localtime_s(&tmNow, &t) == 0)
                snprintf(szClock, sizeof(szClock), "%02d:%02d:%02d", tmNow.tm_hour, tmNow.tm_min, tmNow.tm_sec);
        }
        ImVec2 cs = ImGui::CalcTextSize(szClock);
        dl->AddText(ImVec2(wp.x + ws.x - cs.x - 16.f, ty), UI::COL_TEXT_MUTE, szClock);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
