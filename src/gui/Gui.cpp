#include "../Includes.h"

// -----------------------------------------------------------------------
// Modern Dark / Blue palette
// -----------------------------------------------------------------------
static inline ImVec4 C(int r, int g, int b, int a = 255)
{
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

static void ApplyModernTheme()
{
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 8.f;
    s.ChildRounding     = 6.f;
    s.FrameRounding     = 4.f;
    s.PopupRounding     = 6.f;
    s.ScrollbarRounding = 4.f;
    s.GrabRounding      = 4.f;
    s.TabRounding       = 4.f;

    s.WindowBorderSize  = 1.f;
    s.FrameBorderSize   = 1.f;
    s.PopupBorderSize   = 1.f;
    s.ChildBorderSize   = 1.f;

    s.ItemSpacing       = ImVec2(8.f,  6.f);
    s.FramePadding      = ImVec2(6.f,  4.f);
    s.WindowPadding     = ImVec2(12.f, 10.f);

    ImVec4* c = s.Colors;

    c[ImGuiCol_WindowBg]            = C(15,  15,  22);
    c[ImGuiCol_ChildBg]             = C(20,  20,  28);
    c[ImGuiCol_PopupBg]             = C(20,  20,  28);

    c[ImGuiCol_Border]              = C(40,  40,  55, 120);
    c[ImGuiCol_BorderShadow]        = C(0,   0,    0,  0);

    c[ImGuiCol_FrameBg]             = C(25,  25,  35);
    c[ImGuiCol_FrameBgHovered]      = C(35,  35,  48);
    c[ImGuiCol_FrameBgActive]       = C(60,  110, 220);

    c[ImGuiCol_TitleBg]             = C(12,  12,  18);
    c[ImGuiCol_TitleBgActive]       = C(20,  20,  28);
    c[ImGuiCol_TitleBgCollapsed]    = C(12,  12,  18);

    c[ImGuiCol_MenuBarBg]           = C(20,  20,  28);

    c[ImGuiCol_ScrollbarBg]         = C(15,  15,  22);
    c[ImGuiCol_ScrollbarGrab]       = C(50,  100, 200);
    c[ImGuiCol_ScrollbarGrabHovered]= C(70,  130, 240);
    c[ImGuiCol_ScrollbarGrabActive] = C(90,  150, 255);

    c[ImGuiCol_CheckMark]           = C(70,  140, 240);
    c[ImGuiCol_SliderGrab]          = C(50,  100, 200);
    c[ImGuiCol_SliderGrabActive]    = C(70,  140, 240);

    c[ImGuiCol_Button]              = C(35,  35,  48);
    c[ImGuiCol_ButtonHovered]       = C(50,  50,  65);
    c[ImGuiCol_ButtonActive]        = C(60,  110, 220);

    c[ImGuiCol_Header]              = C(40,  40,  55);
    c[ImGuiCol_HeaderHovered]       = C(60,  110, 220, 180);
    c[ImGuiCol_HeaderActive]        = C(80,  140, 240);

    c[ImGuiCol_Separator]           = C(40,  40,  55);
    c[ImGuiCol_SeparatorHovered]    = C(60,  110, 220);
    c[ImGuiCol_SeparatorActive]     = C(80,  140, 240);

    c[ImGuiCol_ResizeGrip]          = C(40,  40,  55, 80);
    c[ImGuiCol_ResizeGripHovered]   = C(60,  110, 220);
    c[ImGuiCol_ResizeGripActive]    = C(80,  140, 240);

    c[ImGuiCol_Tab]                 = C(20,  20,  28);
    c[ImGuiCol_TabHovered]          = C(60,  110, 220, 180);
    c[ImGuiCol_TabActive]           = C(60,  110, 220);
    c[ImGuiCol_TabUnfocused]        = C(20,  20,  28);
    c[ImGuiCol_TabUnfocusedActive]  = C(40,  80,  160);

    c[ImGuiCol_PlotLines]           = C(70,  140, 240);
    c[ImGuiCol_PlotLinesHovered]    = C(90,  160, 255);
    c[ImGuiCol_PlotHistogram]       = C(70,  140, 240);
    c[ImGuiCol_PlotHistogramHovered]= C(90,  160, 255);

    c[ImGuiCol_TableBorderLight]    = C(40,  40,  55);
    c[ImGuiCol_TableBorderStrong]   = C(50,  50,  65);

    c[ImGuiCol_TextSelectedBg]      = C(60,  110, 220, 80);

    c[ImGuiCol_Text]                = C(230, 230, 240);
    c[ImGuiCol_TextDisabled]        = C(120, 120, 130);

    c[ImGuiCol_NavHighlight]        = C(70,  140, 240);
    c[ImGuiCol_DragDropTarget]      = C(90,  160, 255);
    c[ImGuiCol_ModalWindowDimBg]    = C(0,   0,    0, 140);
}

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------
static void SectionTitle(const char* label)
{
    ImGui::PushStyleColor(ImGuiCol_Text, C(100, 150, 255));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
    ImGui::Separator();
    ImGui::Spacing();
}

// Draw a red padlock + "MID+ only" message inline
static void LockedFeature(const char* szFeatureName, ETier eRequired)
{
    ImGui::PushStyleColor(ImGuiCol_Text, C(255, 60, 60));
    ImGui::Text("  [QULFLANGAN]  %s  -  %s daraja kerak",
        szFeatureName,
        eRequired == ETier::MID ? "MID" : "PRO");
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, C(120, 120, 140));
    ImGui::Text("  Yangilash shifthub.uz orqali");
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

static void ColorEdit4Shim(const char* label, Color& col)
{
    float arr[4] = { col.rBase(), col.gBase(), col.bBase(), col.aBase() };
    if (ImGui::ColorEdit4(label, arr, ImGuiColorEditFlags_AlphaBar))
        col.Set(arr[0], arr[1], arr[2], arr[3]);
}

// Returns a human-readable key name for common VK codes
static const char* GetKeyName(int vk)
{
    switch (vk)
    {
    case 0:              return "Auto";
    case VK_LBUTTON:     return "LMB";
    case VK_RBUTTON:     return "RMB";
    case VK_MBUTTON:     return "MMB";
    case VK_XBUTTON1:    return "Mouse4";
    case VK_XBUTTON2:    return "Mouse5";
    case VK_SPACE:       return "Space";
    case VK_LSHIFT:      return "L.Shift";
    case VK_RSHIFT:      return "R.Shift";
    case VK_LCONTROL:    return "L.Ctrl";
    case VK_RCONTROL:    return "R.Ctrl";
    case VK_LMENU:       return "L.Alt";
    case VK_RMENU:       return "R.Alt";
    case VK_CAPITAL:     return "CapsLock";
    case VK_TAB:         return "Tab";
    case VK_F1:          return "F1";
    case VK_F2:          return "F2";
    case VK_F3:          return "F3";
    case VK_F4:          return "F4";
    case VK_F5:          return "F5";
    case VK_F6:          return "F6";
    case VK_INSERT:      return "Insert";
    case VK_DELETE:      return "Delete";
    default:
        // Single printable character (A-Z, 0-9)
        if (vk >= 'A' && vk <= 'Z') { static char buf[2]; buf[0] = (char)vk; buf[1] = 0; return buf; }
        if (vk >= '0' && vk <= '9') { static char buf[2]; buf[0] = (char)vk; buf[1] = 0; return buf; }
        return "?";
    }
}

// Shows current key + a "[ Bind ]" button. Click Bind, then press any key to assign.
// id must be unique (e.g. "##bind_aimkey")
static void KeyBind(const char* label, int& key, const char* id)
{
    static int*  s_pListening  = nullptr; // which key slot is waiting for input
    static float s_flListenTimer = 0.f;   // cancel after 5s

    bool bThisListening = (s_pListening == &key);

    ImGui::Text("%s", label);
    ImGui::SameLine();

    if (bThisListening)
    {
        s_flListenTimer -= ImGui::GetIO().DeltaTime;
        if (s_flListenTimer <= 0.f)
            s_pListening = nullptr;

        ImGui::PushStyleColor(ImGuiCol_Button,        C(180, 60,  60));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(220, 80,  80));
        ImGui::PushStyleColor(ImGuiCol_Text,          C(255, 255, 100));

        char buf[32];
        snprintf(buf, sizeof(buf), "[ Tugmani bosing... ]##%s", id);
        ImGui::Button(buf, ImVec2(140.f, 0.f));
        ImGui::PopStyleColor(3);

        // Scan all keys (BUT wait 0.2s before accepting, so we don't catch the left-click used to press this button)
        if (s_flListenTimer < 4.8f) // 5.0f - 0.2f
        {
            for (int i = 1; i < 256; i++)
            {
                if (GetAsyncKeyState(i) & 0x8000)
                {
                    if (i == VK_ESCAPE)
                    {
                        // ESC always cancels
                        s_pListening = nullptr;
                    }
                    else
                    {
                        key = i;
                        s_pListening = nullptr;
                    }
                    break;
                }
            }
        }
    }
    else
    {
        // Show current key name + Bind button
        char btnLabel[64];
        snprintf(btnLabel, sizeof(btnLabel), "[ %-8s ] O'rnatish##%s", GetKeyName(key), id);

        ImGui::PushStyleColor(ImGuiCol_Button,        C(40, 60, 100));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(50, 90, 140));
        if (ImGui::Button(btnLabel, ImVec2(160.f, 0.f)))
        {
            s_pListening   = &key;
            s_flListenTimer = 5.f;
        }
        ImGui::PopStyleColor(2);
    }

    // "0 = Auto" hint for trigger key
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, C(80, 80, 100));
    ImGui::Text("(ESC=bekor qilish)");
    ImGui::PopStyleColor();
}

// -----------------------------------------------------------------------
// Initialize fonts
// -----------------------------------------------------------------------
void Gui::Initialize(unsigned int uFontFlags)
{
    ImGuiIO& io = ImGui::GetIO();

    ApplyModernTheme();

    ImFontConfig cfg = {};
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_LightHinting
                         | ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bold;

    Fonts::Default = io.Fonts->AddFontFromFileTTF(X("C:\\Windows\\Fonts\\Verdana.ttf"), 18, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    Fonts::ESP      = io.Fonts->AddFontFromFileTTF(X("C:\\Windows\\Fonts\\Verdana.ttf"), 10, &cfg, io.Fonts->GetGlyphRangesCyrillic());

    m_bInitialized = ImGuiFreeType::BuildFontAtlas(io.Fonts, uFontFlags);
}

// -----------------------------------------------------------------------
// Update
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

// -----------------------------------------------------------------------
// Render
// -----------------------------------------------------------------------
void Gui::Render()
{
    if (!m_bInitialized)
        return;

    ImGuiIO& io = ImGui::GetIO();
    Gui::Update(io);

    // ================================================================
    // Title bar text:  SHIFTHUB  |  v1.0  |  [TIER]  |  User
    // ================================================================
    char szTitle[128];
    snprintf(szTitle, sizeof(szTitle),
        " KaeL cheat  |  v1.0  |  [ %s ]  |  %s",
        g_License.GetTierName(),
        g_License.m_strUser.c_str());

    ImGui::SetNextWindowSize(ImVec2(880, 560));
    ImGui::Begin(szTitle, nullptr,
        ImGuiWindowFlags_NoResize       |
        ImGuiWindowFlags_NoSavedSettings|
        ImGuiWindowFlags_NoCollapse);
    {
        // Tier badge strip
        ImGui::PushStyleColor(ImGuiCol_Text, g_License.GetTierColor());
        ImGui::Text("  Daraja: %s   |   %s   |   kaelcheat",
            g_License.GetTierName(),
            g_License.m_strExpiry.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Spacing();

        static int current_tab = 0;
        const char* tabs[] = { "Vizual", "UZBEK", "Jang", "Harakat", "Radar", "Inventar", "Sozlamalar" };

        ImGui::BeginChild(X("LeftPanel"), ImVec2(160, 0), true);
        ImGui::Spacing();
        for (int i = 0; i < 7; ++i)
        {
            bool bSelected = (current_tab == i);
            if (bSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, C(50, 100, 200));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(60, 110, 210));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, C(70, 130, 230));
                ImGui::PushStyleColor(ImGuiCol_Text, C(255, 255, 255));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, C(20, 20, 28));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(30, 30, 40));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, C(40, 40, 50));
                ImGui::PushStyleColor(ImGuiCol_Text, C(140, 140, 150));
            }

            bool bLocked = (g_License.m_eTier == ETier::LITE && (i == 2 || i == 3 || i == 5));
            char szTabName[64];
            if (bLocked) snprintf(szTabName, sizeof(szTabName), "%s  [PRO]", tabs[i]);
            else snprintf(szTabName, sizeof(szTabName), "%s", tabs[i]);

            if (bLocked && !bSelected) 
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 100)); // Qulf rangi
            else if (bLocked && bSelected)
                ImGui::PushStyleColor(ImGuiCol_Text, C(200, 180, 180));

            if (ImGui::Button(szTabName, ImVec2(-1, 40)))
                current_tab = i;

            if (bLocked) ImGui::PopStyleColor(); // Qo'shimcha qulf rangi uchun
            ImGui::PopStyleColor(4); // Asosiy 4 ta rang
            ImGui::Spacing();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        ImGui::BeginChild(X("RightPanel"), ImVec2(0, 0), true);
        {
            if (current_tab == 0) // Vizual
            {
                ImGui::BeginChild(X("##LeftVis"), ImVec2(440, 0), false);


                ImGui::Spacing();
                SectionTitle("Devor orti ko'rish (WH)");

                ImGui::Checkbox(X("Devordan ko'rishni yoqish"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals));

                if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals))
                {
                    ImGui::Spacing();
                    {
                        auto& vecMod = Config::Get<std::vector<bool>>(g_Variables.m_PlayerVisuals.m_vecVisualsModifiers);
                        bool bIgnoreTeam = vecMod[VISUALS_IGNORE_TEAMMATES];
                        bool bOnlyVis    = vecMod[VISUALS_ONLY_WHEN_VISIBLE];
                        if (ImGui::Checkbox(X("Jamoani e'tiborsiz"), &bIgnoreTeam)) vecMod[VISUALS_IGNORE_TEAMMATES] = bIgnoreTeam;
                        ImGui::SameLine(220.f);
                        if (ImGui::Checkbox(X("Faqat ko'ringanlar"), &bOnlyVis))        vecMod[VISUALS_ONLY_WHEN_VISIBLE] = bOnlyVis;
                    }

                    ImGui::Spacing();
                    SectionTitle("Quti");

                    ImGui::Checkbox(X("Quti chizish"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBox));
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBox))
                    {
                        static const char* boxTypes[] = { "2D Quti", "Burchak", "Ikkisi" };
                        ImGui::SetNextItemWidth(160.f);
                        ImGui::Combo(X("Quti turi"), &CONFIG_GET(int, g_Variables.m_PlayerVisuals.m_iBoxType), boxTypes, 3);
                        ImGui::Checkbox(X("Quti chegarasi"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline));
                    }

                    ImGui::Spacing();
                    SectionTitle("Qo'shimcha ma'lumotlar");

                    ImGui::Checkbox(X("Jon paneli"),  &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHealthBar));
                    ImGui::SameLine(220.f);
                    ImGui::Checkbox(X("Ism"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawName));

                    ImGui::Checkbox(X("Qurol nomi"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawWeapon));
                    ImGui::SameLine(220.f);
                    ImGui::Checkbox(X("Masofa"),    &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawDistance));

                    ImGui::Checkbox(X("Skelet"),    &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSkeleton));
                    ImGui::SameLine(220.f);
                    ImGui::Checkbox(X("Bosh nuqta"),    &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHeadDot));

                    ImGui::Checkbox(X("Tana bo'yash"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawFilledBody));
                    ImGui::SameLine(220.f);
                    ImGui::Checkbox(X("Chiziqlar"),  &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawSnaplines));


                    SectionTitle("Tezkor tugma");

                    KeyBind("ESP yoqish/o'chirish:", CONFIG_GET(int, g_Variables.m_Hotkeys.m_iESPToggleKey), "espkey");

                    ImGui::Spacing();
                    SectionTitle("Ranglar");

                    ColorEdit4Shim(X("Dushman (ko'ringan)"),  CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyVisible));
                    ColorEdit4Shim(X("Dushman (yashirin)"), CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colEnemyOccluded));
                    ColorEdit4Shim(X("Jamoadosh"),       CONFIG_GET(Color, g_Variables.m_PlayerVisuals.m_colTeammate));
                }



                ImGui::Spacing();
                ImGui::Spacing();
                SectionTitle("Odamni yoritish (Glow)");
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                ImGui::Text("  Dushman yoki sizni fosfordek yoritib turadi");
                ImGui::PopStyleColor();

                ImGui::Checkbox(X("Yoritishni yoqish"), &CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow));

                if (CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow))
                {
                    ImGui::Checkbox(X("Glow bilan Jon va Qurolni ko'rsatish"), &CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowInfo));
                    ImGui::Checkbox(X("Faqat dushmanlarni yoritish##glow"), &CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowEnemyOnly));

                    static const char* glowTypes[] = { "O'chirilgan", "Oddiy", "Pulsating", "Tashqi Kontur" };
                    ImGui::SetNextItemWidth(160.f);
                    ImGui::Combo(X("Yoritish turi"), &CONFIG_GET(int, g_Variables.m_PlayerGlow.m_iGlowType), glowTypes, 4);

                    ColorEdit4Shim(X("Dushman rangi##gcol"), CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowEnemy));
                    if (!CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bGlowEnemyOnly))
                        ColorEdit4Shim(X("Jamoadosh rangi##gcol2"), CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowTeam));
                }

                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild(X("##RightVis"), ImVec2(0, 0), false);


                // --- O'NG TOMON: ESP PREVIEW ---
                ImGui::Spacing();
                SectionTitle("ESP Preview");

                ImVec2 pxy = ImGui::GetCursorScreenPos();
                ImDrawList* dl = ImGui::GetWindowDrawList();

                // Fon qutichasi (qorong'ulashtirilgan)
                dl->AddRectFilled(pxy, ImVec2(pxy.x + 220.f, pxy.y + 300.f), IM_COL32(10, 15, 20, 255), 6.f);
                dl->AddRect(pxy, ImVec2(pxy.x + 220.f, pxy.y + 300.f), IM_COL32(40, 60, 90, 255), 6.f);

                float cx = pxy.x + 110.f;  // Markaz X
                float cy = pxy.y + 150.f;  // Markaz Y

                // Dummy model (Stickman / Maneken) chizish
                ImU32 colStick = IM_COL32(180, 180, 180, 255);

                // Glow (Yoritish) effekti
                if (CONFIG_GET(bool, g_Variables.m_PlayerGlow.m_bEnableGlow))
                {
                    Color gCol = CONFIG_GET(Color, g_Variables.m_PlayerGlow.m_colGlowEnemy);
                    ImU32 colGlow = gCol.GetU32(0.6f); // Orqa fona biroz xiraroq yorqinlik
                    
                    // Tashqi qalin porlovchi qatlam (Glow outline)
                    dl->AddCircle(ImVec2(cx, cy - 60), 16, colGlow, 0, 6.0f); 
                    dl->AddLine(ImVec2(cx, cy - 46), ImVec2(cx, cy + 20), colGlow, 6.0f); 
                    dl->AddLine(ImVec2(cx, cy - 30), ImVec2(cx - 25, cy + 5), colGlow, 6.0f); 
                    dl->AddLine(ImVec2(cx, cy - 30), ImVec2(cx + 25, cy + 5), colGlow, 6.0f); 
                    dl->AddLine(ImVec2(cx, cy + 20), ImVec2(cx - 20, cy + 80), colGlow, 6.0f); 
                    dl->AddLine(ImVec2(cx, cy + 20), ImVec2(cx + 20, cy + 80), colGlow, 6.0f); 
                    
                    int gType = CONFIG_GET(int, g_Variables.m_PlayerGlow.m_iGlowType);
                    if (gType == 1 || gType == 2) { // Oddiy va Pulsating turlarida modelning o'zi xam rangga kiradi
                        colStick = gCol.GetU32(1.0f);
                    }
                }

                // Asosiy tana chiziqlari
                dl->AddCircle(ImVec2(cx, cy - 60), 14, colStick, 0, 2.0f); // Bosh
                dl->AddLine(ImVec2(cx, cy - 46), ImVec2(cx, cy + 20), colStick, 2.0f); // Tana
                dl->AddLine(ImVec2(cx, cy - 30), ImVec2(cx - 25, cy + 5), colStick, 2.0f); // Chap qo'l
                dl->AddLine(ImVec2(cx, cy - 30), ImVec2(cx + 25, cy + 5), colStick, 2.0f); // O'ng qo'l
                dl->AddLine(ImVec2(cx, cy + 20), ImVec2(cx - 20, cy + 80), colStick, 2.0f); // Chap oyoq
                dl->AddLine(ImVec2(cx, cy + 20), ImVec2(cx + 20, cy + 80), colStick, 2.0f); // O'ng oyoq

                // --- ESP XUSUSIYATLARI KO'RINISHI ---
                if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bEnableVisuals))
                {
                    float minX = cx - 50.f, minY = cy - 85.f;
                    float maxX = cx + 50.f, maxY = cy + 90.f;

                    // Quti chizish
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBox))
                    {
                        int boxType = CONFIG_GET(int, g_Variables.m_PlayerVisuals.m_iBoxType);
                        bool bOutline = CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline);
                        
                        ImU32 colBox = IM_COL32(0, 150, 255, 255);
                        ImU32 colOut = IM_COL32(0, 0, 0, 255);
                        float th = 2.0f; float outTh = 4.0f;

                        if (boxType == 0 || boxType == 2) { // 2D yoki Ikkisi
                            if (bOutline) dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), colOut, 0.f, 0, outTh);
                            dl->AddRect(ImVec2(minX, minY), ImVec2(maxX, maxY), colBox, 0.f, 0, th);
                        }
                        
                        if (boxType == 1 || boxType == 2) { // Burchak yoki Ikkisi
                            float l = 20.f;
                            ImVec2 p[8][2] = {
                                { ImVec2(minX, minY), ImVec2(minX + l, minY) }, { ImVec2(minX, minY), ImVec2(minX, minY + l) },
                                { ImVec2(maxX, minY), ImVec2(maxX - l, minY) }, { ImVec2(maxX, minY), ImVec2(maxX, minY + l) },
                                { ImVec2(minX, maxY), ImVec2(minX + l, maxY) }, { ImVec2(minX, maxY), ImVec2(minX, maxY - l) },
                                { ImVec2(maxX, maxY), ImVec2(maxX - l, maxY) }, { ImVec2(maxX, maxY), ImVec2(maxX, maxY - l) }
                            };
                            for (int i=0; i<8; i++) {
                                if (bOutline) dl->AddLine(p[i][0], p[i][1], colOut, outTh);
                                dl->AddLine(p[i][0], p[i][1], colBox, th);
                            }
                        }
                    }

                    // Jon paneli (Yashil chiziq)
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHealthBar))
                    {
                        if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline))
                            dl->AddRectFilled(ImVec2(minX - 11, minY - 1), ImVec2(minX - 5, maxY + 1), IM_COL32(0, 0, 0, 255));
                        dl->AddRectFilled(ImVec2(minX - 10, minY), ImVec2(minX - 6, maxY), IM_COL32(40, 40, 40, 255));
                        dl->AddRectFilled(ImVec2(minX - 10, minY + 30), ImVec2(minX - 6, maxY), IM_COL32(0, 255, 40, 255));
                    }

                    // Ism (Tepa qismda)
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawName))
                    {
                        const char* nameStr = "Dushman";
                        ImVec2 tSize = ImGui::CalcTextSize(nameStr);
                        dl->AddText(ImVec2(cx - tSize.x * 0.5f, minY - 18), IM_COL32(255, 255, 255, 255), nameStr);
                    }

                    // Qurol (Past qismda - Endi pistolet surati chiziladi)
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawWeapon))
                    {
                        float wY = maxY + 8;
                        if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawBoxOutline)) {
                            dl->AddRectFilled(ImVec2(cx - 10-1, wY-1), ImVec2(cx + 10+1, wY + 4+1), IM_COL32(0,0,0,255));
                            dl->AddRectFilled(ImVec2(cx - 10-1, wY-1), ImVec2(cx - 4+1, wY + 12+1), IM_COL32(0,0,0,255));
                        }
                        dl->AddRectFilled(ImVec2(cx - 10, wY), ImVec2(cx + 10, wY + 4), IM_COL32(200, 200, 200, 255)); // Stvol (Barrel)
                        dl->AddRectFilled(ImVec2(cx - 10, wY), ImVec2(cx - 4, wY + 12), IM_COL32(150, 150, 150, 255)); // Dastasi (Handle)
                    }

                    // Bosh nuqta (Qizil)
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHeadDot))
                    {
                        dl->AddCircleFilled(ImVec2(cx, cy - 60), 4, IM_COL32(255, 0, 0, 255));
                    }
                    
                    // Masofa ko'rsatkichi
                    if (CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawDistance))
                    {
                        const char* distStr = "[12m]";
                        ImVec2 tSize = ImGui::CalcTextSize(distStr);
                        dl->AddText(ImVec2(cx - tSize.x * 0.5f, maxY + 20), IM_COL32(180, 180, 180, 255), distStr);
                    }
                }

                ImGui::EndChild();
            }
            else if (current_tab == 1) // UZBEK
            {
                ImGui::Spacing();
                SectionTitle("UZBEK xususiyatlari");

                ImGui::Checkbox(X("Bomba ogohlantirish"), &CONFIG_GET(bool, g_Variables.m_PlayerVisuals.m_bDrawHasC4));

                ImGui::Checkbox(X("Flash himoya"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bAntiFlash));
                ImGui::SameLine(220.f);
                ImGui::Checkbox(X("C4 Timer"),     &CONFIG_GET(bool, g_Variables.m_Misc.m_bC4Timer));

                ImGui::Checkbox(X("Granata xavfi"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bGrenadeWarning));
                ImGui::SameLine(220.f);
                ImGui::Checkbox(X("Sniper Crosshair"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bSniperCrosshair));

                ImGui::Checkbox(X("Avtoqabul (Match)"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bAutoAccept));
                ImGui::SameLine(220.f);
                ImGui::Checkbox(X("Tomoshabinlar"), &CONFIG_GET(bool, g_Variables.m_SpectatorList.m_bEnableSpectatorList));

                ImGui::Spacing();
                SectionTitle("Yerdagi Qurollar (Loot)");
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                ImGui::Text("  Yerda yotgan qurollarni masofasi va nomi bilan ko'rsatib turadi");
                ImGui::PopStyleColor();

                ImGui::Checkbox(X("Loot ESP yoqish"), &CONFIG_GET(bool, g_Variables.m_ESP.m_bDroppedWeapons));
                if (CONFIG_GET(bool, g_Variables.m_ESP.m_bDroppedWeapons))
                {
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat(X("Loot Masofasi"), &CONFIG_GET(float, g_Variables.m_ESP.m_flWeaponDistance), 5.f, 200.f, "%.0f metragacha");
                }
            }
            else if (current_tab == 2) // Jang
            {
                ImGui::Spacing();

                // --- Triggerbot (MID+) ---
                SectionTitle("Triggerbot");

                if (g_License.HasFeature(ETier::MID))
                {
                    ImGui::Checkbox(X("Triggerbot yoqish"), &CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bEnableTriggerbot));
                    ImGui::Spacing();

                    bool& bAutoShoot = CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bAutoShoot);
                    ImGui::Checkbox(X("Avto otish (tugmasiz)"), &bAutoShoot);

                    if (!bAutoShoot)
                    {
                        KeyBind("Ushlab turish:", CONFIG_GET(int, g_Variables.m_TriggerBot.m_iTriggerKey), "trigkey");
                        ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                        ImGui::Text("  OTISH kerak bo'lganda shu tugmani bosing");
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, C(100, 150, 255));
                        ImGui::Text("  [+] Avtomatik otadi (dushman nishonga tushganda)");
                        ImGui::PopStyleColor();
                    }

                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat(X("Otish kechikishi"), &CONFIG_GET(float, g_Variables.m_TriggerBot.m_flShotDelay), 0.f, 300.f, "%.0f ms");

                    ImGui::Checkbox(X("Jamoani e'tiborsiz##trig"), &CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bIgnoreTeammates));
                    ImGui::Checkbox(X("Faqat ko'ringanlar (devordan emas)"), &CONFIG_GET(bool, g_Variables.m_TriggerBot.m_bOnlyVisible));
                }
                else
                {
                    LockedFeature("Triggerbot", ETier::MID);
                }

                ImGui::Spacing();
                ImGui::Spacing();

                // --- Aimbot (PRO) ---
                SectionTitle("Aimbot");

                if (g_License.HasFeature(ETier::PRO))
                {
                    ImGui::Checkbox(X("Aimbot yoqish"), &CONFIG_GET(bool, g_Variables.m_AimBot.m_bEnableAimbot));
                    ImGui::Spacing();

                    KeyBind("Nishon tugmasi:", CONFIG_GET(int, g_Variables.m_AimBot.m_iAimKey), "aimkey");

                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat(X("FOV"), &CONFIG_GET(float, g_Variables.m_AimBot.m_flFOV), 0.5f, 30.f, "%.1f deg");

                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat(X("Silliqlik"), &CONFIG_GET(float, g_Variables.m_AimBot.m_flSmooth), 1.f, 20.f, "%.1f");
                    ImGui::PushStyleColor(ImGuiCol_Text, C(120, 120, 140));
                    ImGui::SameLine(); ImGui::Text("(1=tez, 10=silliq)");
                    ImGui::PopStyleColor();

                    static const char* hitboxNames[] = { "Bosh", "Bo'yin", "Ko'krak" };
                    ImGui::SetNextItemWidth(140.f);
                    ImGui::Combo(X("Nishon joyi"), &CONFIG_GET(int, g_Variables.m_AimBot.m_iHitbox), hitboxNames, 3);

                    ImGui::Checkbox(X("Jamoani e'tiborsiz##aim"), &CONFIG_GET(bool, g_Variables.m_AimBot.m_bIgnoreTeammates));
                    ImGui::Checkbox(X("FOV doirasini chizish"),       &CONFIG_GET(bool, g_Variables.m_AimBot.m_bDrawFOV));

                    ImGui::Spacing();
                    SectionTitle("Recoil Control (RCS) - Otkacha");
                    ImGui::Checkbox(X("RCS yoqish"), &CONFIG_GET(bool, g_Variables.m_RCS.m_bEnable));
                    if (CONFIG_GET(bool, g_Variables.m_RCS.m_bEnable))
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                        ImGui::Text("Aimbot yoqilgan payti ishlamaydi, mustaqil tortadi.");
                        ImGui::PopStyleColor();
                        ImGui::SetNextItemWidth(200.f);
                        ImGui::SliderFloat(X("RCS X (Yonga) kuch"), &CONFIG_GET(float, g_Variables.m_RCS.m_flScaleX), 0.f, 2.0f, "%.2f");
                        ImGui::SetNextItemWidth(200.f);
                        ImGui::SliderFloat(X("RCS Y (Pastga) kuch"), &CONFIG_GET(float, g_Variables.m_RCS.m_flScaleY), 0.f, 2.0f, "%.2f");
                    }
                }
                else
                {
                    LockedFeature("Aimbot", ETier::PRO);
                }
            }
            else if (current_tab == 3) // Harakat
            {
                ImGui::Spacing();

                if (g_License.HasFeature(ETier::MID))
                {
                    SectionTitle("Bunny Hop");

                    ImGui::Checkbox(X("Bhop yoqish"), &CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableBhop));

                    if (CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableBhop))
                    {
                        ImGui::Checkbox(X("Auto-Strafer (Avtomatik sakrash)"), &CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableAutoStrafe));
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Text, C(255, 200, 50));
                        ImGui::Text("  Space (32) ishlatmang!");
                        ImGui::PopStyleColor();
                        KeyBind("Ushlab turish:", CONFIG_GET(int, g_Variables.m_Bhop.m_iBhopKey), "bhopkey");
                        ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                        ImGui::Text("  Bu tugmani ushlab turing — cheat Space bosadi");
                        ImGui::PopStyleColor();
                    }
                }
                else
                {
                    LockedFeature("Bunny Hop", ETier::MID);
                }
            }
            else if (current_tab == 4) // Radar
            {
                ImGui::Spacing();
                SectionTitle("2D Radar");

                ImGui::Checkbox(X("Radar yoqish"), &CONFIG_GET(bool, g_Variables.m_Radar.m_bEnableRadar));

                if (CONFIG_GET(bool, g_Variables.m_Radar.m_bEnableRadar))
                {
                    ImGui::Spacing();
                    ImGui::SetNextItemWidth(180.f);
                    ImGui::SliderFloat(X("Radar kattaligi"),   &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarSize),  100.f, 400.f, "%.0f px");
                    ImGui::SetNextItemWidth(180.f);
                    ImGui::SliderFloat(X("Radar masofasi"),  &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarRange), 500.f, 5000.f, "%.0f u");
                    ImGui::SetNextItemWidth(180.f);
                    ImGui::SliderFloat(X("X pozitsiya"),   &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarX),    0.f, 1920.f, "%.0f");
                    ImGui::SetNextItemWidth(180.f);
                    ImGui::SliderFloat(X("Y pozitsiya"),   &CONFIG_GET(float, g_Variables.m_Radar.m_flRadarY),    0.f, 1080.f, "%.0f");
                    ImGui::Checkbox(X("Qarash bilan aylantirish"), &CONFIG_GET(bool, g_Variables.m_Radar.m_bRadarRotate));
                }

                ImGui::Spacing();
                ImGui::Spacing();
                SectionTitle("O'yin radari (In-Game Hack)");
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                ImGui::Text("  Dushmanlarni o'yinning chap tepasidagi asil radariga ochib beradi.");
                ImGui::PopStyleColor();

                ImGui::Checkbox(X("O'yin radarida ko'rsatish"), &CONFIG_GET(bool, g_Variables.m_Radar.m_bInGameRadar));

                ImGui::Spacing();
                ImGui::Spacing();
                SectionTitle("Ovozli Radar (Sonar ESP)");
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                ImGui::Text("  Dushmanga qaraganingizda devor ortidan ovoz (Piip) chiqaradi.");
                ImGui::PopStyleColor();

                ImGui::Checkbox(X("Ovozli radarni yoqish"), &CONFIG_GET(bool, g_Variables.m_Misc.m_bEnableSonar));
                if (CONFIG_GET(bool, g_Variables.m_Misc.m_bEnableSonar))
                {
                    ImGui::SetNextItemWidth(180.f);
                    ImGui::SliderFloat(X("Sezuvchanlik radiusi (FOV)"), &CONFIG_GET(float, g_Variables.m_Misc.m_flSonarFOV), 1.f, 20.f, "%.1f deg");
                }
            }
            else if (current_tab == 5) // Inventar
            {
                ImGui::Spacing();
                SectionTitle("Skin o'zgartirish");

                ImGui::Checkbox(X("Skin o'zgartirish yoqish"), &SkinChanger::m_bEnabled);
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                ImGui::Text("  Qo'ldagi qurollar skinini o'zgartiradi");
                ImGui::PopStyleColor();

                if (SkinChanger::m_bEnabled)
                {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    static int nSelectedWeapon = 0;

                    ImGui::BeginColumns(X("##SkinCols"), 2, ImGuiColumnsFlags_NoResize);
                    ImGui::SetColumnWidth(0, 180.f);
                    {
                        // === LEFT: Weapon list ===
                        ImGui::PushStyleColor(ImGuiCol_Text, C(100, 150, 255));
                        ImGui::TextUnformatted("Qurollar");
                        ImGui::PopStyleColor();
                        ImGui::Spacing();

                        if (ImGui::BeginListBox(X("##WeaponList"), ImVec2(-1, 340.f)))
                        {
                            for (int i = 0; i < SkinChanger::g_nWeaponCategoryCount; i++)
                            {
                                WeaponCategory_t& cat = SkinChanger::g_vecWeaponCategories[i];
                                WeaponSkinConfig_t& cfg = SkinChanger::GetWeaponConfig(cat.m_nDefIndex);

                                // Mark weapons that have a skin selected
                                char label[64];
                                if (cfg.m_nPaintKit != 0)
                                    snprintf(label, sizeof(label), "[*] %s", cat.m_szName);
                                else
                                    snprintf(label, sizeof(label), "    %s", cat.m_szName);

                                if (ImGui::Selectable(label, i == nSelectedWeapon))
                                    nSelectedWeapon = i;
                            }
                            ImGui::EndListBox();
                        }
                    }
                    ImGui::NextColumn();
                    {
                        // === RIGHT: Skin selector ===
                        if (nSelectedWeapon >= 0 && nSelectedWeapon < SkinChanger::g_nWeaponCategoryCount)
                        {
                            WeaponCategory_t& cat = SkinChanger::g_vecWeaponCategories[nSelectedWeapon];
                            WeaponSkinConfig_t& cfg = SkinChanger::GetWeaponConfig(cat.m_nDefIndex);

                            ImGui::PushStyleColor(ImGuiCol_Text, C(100, 150, 255));
                            ImGui::Text("%s - Skinlar", cat.m_szName);
                            ImGui::PopStyleColor();
                            ImGui::Spacing();

                            // Skin dropdown
                            const char* szCurrentSkin = "Default";
                            for (int j = 0; j < cat.m_nSkinCount; j++)
                            {
                                if (cat.m_pSkins[j].m_nPaintKit == cfg.m_nPaintKit)
                                {
                                    szCurrentSkin = cat.m_pSkins[j].m_szName;
                                    break;
                                }
                            }

                            ImGui::SetNextItemWidth(-1);
                            if (ImGui::BeginCombo(X("##SkinCombo"), szCurrentSkin))
                            {
                                for (int j = 0; j < cat.m_nSkinCount; j++)
                                {
                                    bool bSelected = (cat.m_pSkins[j].m_nPaintKit == cfg.m_nPaintKit);
                                    char skinLabel[96];
                                    if (cat.m_pSkins[j].m_nPaintKit == 0)
                                        snprintf(skinLabel, sizeof(skinLabel), "Default (off)");
                                    else
                                        snprintf(skinLabel, sizeof(skinLabel), "[%d] %s", cat.m_pSkins[j].m_nPaintKit, cat.m_pSkins[j].m_szName);

                                    if (ImGui::Selectable(skinLabel, bSelected))
                                        cfg.m_nPaintKit = cat.m_pSkins[j].m_nPaintKit;

                                    if (bSelected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }

                            ImGui::Spacing();
                            ImGui::Spacing();

                            // Wear slider
                            SectionTitle("Sifat (Wear)");
                            ImGui::SetNextItemWidth(-1);
                            ImGui::SliderFloat(X("##Wear"), &cfg.m_flWear, 0.0001f, 1.0f, "%.4f");

                            ImGui::PushStyleColor(ImGuiCol_Text, C(100, 100, 120));
                            if (cfg.m_flWear < 0.07f)       ImGui::Text("  Factory New");
                            else if (cfg.m_flWear < 0.15f)  ImGui::Text("  Minimal Wear");
                            else if (cfg.m_flWear < 0.38f)  ImGui::Text("  Field-Tested");
                            else if (cfg.m_flWear < 0.45f)  ImGui::Text("  Well-Worn");
                            else                             ImGui::Text("  Battle-Scarred");
                            ImGui::PopStyleColor();

                            ImGui::Spacing();

                            // Seed
                            SectionTitle("Pattern Seed");
                            ImGui::SetNextItemWidth(120.f);
                            ImGui::InputInt(X("##Seed"), &cfg.m_nSeed);
                            if (cfg.m_nSeed < 0) cfg.m_nSeed = 0;
                            if (cfg.m_nSeed > 1000) cfg.m_nSeed = 1000;

                            ImGui::Spacing();

                            // StatTrak
                            SectionTitle("StatTrak");
                            bool bStatTrak = (cfg.m_nStatTrak >= 0);
                            if (ImGui::Checkbox(X("Enable StatTrak##st"), &bStatTrak))
                                cfg.m_nStatTrak = bStatTrak ? 0 : -1;

                            if (cfg.m_nStatTrak >= 0)
                            {
                                ImGui::SetNextItemWidth(120.f);
                                ImGui::InputInt(X("##StatTrakVal"), &cfg.m_nStatTrak);
                                if (cfg.m_nStatTrak < 0) cfg.m_nStatTrak = 0;
                            }

                            ImGui::Spacing();
                            ImGui::Spacing();

                            // Reset button
                            ImGui::PushStyleColor(ImGuiCol_Button,        C(120, 30, 30));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, C(180, 50, 50));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  C(220, 60, 60));
                            if (ImGui::Button(X("Reset##skinreset"), ImVec2(120.f, 0.f)))
                            {
                                cfg.m_nPaintKit = 0;
                                cfg.m_flWear    = 0.0001f;
                                cfg.m_nSeed     = 0;
                                cfg.m_nStatTrak = -1;
                            }
                            ImGui::PopStyleColor(3);

                            ImGui::SameLine();

                            // Reset ALL
                            if (ImGui::Button(X("Reset All##skinresetall"), ImVec2(120.f, 0.f)))
                                SkinChanger::m_mapWeaponConfigs.clear();
                        }
                    }
                    ImGui::EndColumns();
                }
            }
            else if (current_tab == 6) // Sozlamalar
            {
                ImGui::Spacing();
                SectionTitle("Sozlamalar boshqaruvi");

                ImGui::BeginColumns(X("##CfgCols"), 2, ImGuiColumnsFlags_NoResize);
                {
                    static int nSelected = -1;
                    if (ImGui::BeginListBox(X("##CfgList"), ImVec2(-1, -1)))
                    {
                        for (size_t i = 0; i < Config::m_vecFileNames.size(); i++)
                        {
                            if (ImGui::Selectable(Config::m_vecFileNames.at(i).c_str(),
                                i == (size_t)nSelected,
                                ImGuiSelectableFlags_DontClosePopups))
                                nSelected = (int)i;
                        }
                        ImGui::EndListBox();
                    }
                    ImGui::NextColumn();

                    static std::string strCfgName;
                    ImGui::InputTextWithHint(X("##cfgfile"), X("config nomi..."), &strCfgName);

                    if (ImGui::Button(X("Yaratish"), ImVec2(120.f, 0.f)))
                    {
                        Config::Save(strCfgName);
                        strCfgName.clear();
                        Config::Refresh();
                    }
                    if (ImGui::Button(X("Yangilash"), ImVec2(120.f, 0.f))) Config::Refresh();

                    if (nSelected != -1)
                    {
                        ImGui::Spacing();
                        if (ImGui::Button(X("Saqlash"),   ImVec2(120.f, 0.f))) Config::Save(Config::m_vecFileNames.at(nSelected));
                        if (ImGui::Button(X("Yuklash"),   ImVec2(120.f, 0.f))) Config::Load(Config::m_vecFileNames.at(nSelected));
                        if (ImGui::Button(X("O'chirish"), ImVec2(120.f, 0.f)))
                        {
                            Config::Remove(Config::m_vecFileNames.at(nSelected));
                            Config::Refresh();
                            nSelected = -1;
                        }
                    }
                }
                ImGui::EndColumns();

                ImGui::Spacing();
                ImGui::Spacing();

                SectionTitle("Asosiy");

                ImGui::Text("Menyu tugmasi:");
                ImGui::SetNextItemWidth(120.f);
                ImGui::InputInt(X("##menukey"), &CONFIG_GET(int, g_Variables.m_Gui.m_iMenuKey));

                ImGui::Text("Yopish tugmasi:");
                ImGui::SetNextItemWidth(120.f);
                ImGui::InputInt(X("##unloadkey"), &CONFIG_GET(int, g_Variables.m_Gui.m_iUnloadKey));

                ImGui::Checkbox(X("Ekran yozishdan yashirish"),
                    &CONFIG_GET(bool, g_Variables.m_Gui.m_bExcludeFromDesktopCapture));


                ImGui::Spacing();
                ImGui::Spacing();
                SectionTitle("Litsenziya ma'lumoti");

                ImGui::PushStyleColor(ImGuiCol_Text, g_License.GetTierColor());
                ImGui::Text("  Daraja:    %s", g_License.GetTierName());
                ImGui::Text("  Foydalanuvchi: %s", g_License.m_strUser.c_str());
                ImGui::Text("  Holat:     %s", g_License.m_strExpiry.c_str());
                ImGui::PopStyleColor();

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, C(100, 120, 110));
                ImGui::Text("  Yangilash uchun key.txt faylini exe yoniga qo'ying.");
                ImGui::Text("  Kalit formati:  SH-XXXXXXXX-M  (MID)  yoki  SH-XXXXXXXX-P  (PRO)");
                ImGui::Text("  Kalit olish: shifthub.uz");
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChild(); // end RightPanel
    }
    ImGui::End(); // end Window
}
