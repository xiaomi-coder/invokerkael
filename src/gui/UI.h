#pragma once
// =====================================================================
//  KAEL_CHEAT  ::  CYBERPUNK UI KIT
//  Minimal, professional neon design system built on top of Dear ImGui.
//  Header-only — include after Gui.h.
// =====================================================================

namespace UI
{
    // -----------------------------------------------------------------
    //  PALETTE
    // -----------------------------------------------------------------
    inline constexpr ImU32 COL_BG        = IM_COL32(  6,   9,  14, 252);  // window base
    inline constexpr ImU32 COL_BG_DEEP   = IM_COL32(  3,   5,   9, 255);  // header / footer
    inline constexpr ImU32 COL_PANEL     = IM_COL32( 11,  15,  23, 255);  // card body
    inline constexpr ImU32 COL_PANEL_HI  = IM_COL32( 17,  23,  34, 255);  // frame / hovered
    inline constexpr ImU32 COL_PANEL_ACT = IM_COL32( 22,  30,  44, 255);
    inline constexpr ImU32 COL_LINE      = IM_COL32( 30,  42,  60, 255);  // hairlines
    inline constexpr ImU32 COL_CYAN      = IM_COL32( 34, 226, 255, 255);  // primary neon
    inline constexpr ImU32 COL_CYAN_SOFT = IM_COL32( 34, 226, 255,  90);
    inline constexpr ImU32 COL_CYAN_FAINT= IM_COL32( 34, 226, 255,  32);
    inline constexpr ImU32 COL_TEAL      = IM_COL32( 12, 140, 168, 255);
    inline constexpr ImU32 COL_MAGENTA   = IM_COL32(255,  46, 136, 255);  // secondary neon
    inline constexpr ImU32 COL_MAG_SOFT  = IM_COL32(255,  46, 136,  70);
    inline constexpr ImU32 COL_AMBER     = IM_COL32(255, 190,  60, 255);  // warning
    inline constexpr ImU32 COL_RED       = IM_COL32(255,  71,  87, 255);  // danger / locked
    inline constexpr ImU32 COL_GREEN     = IM_COL32( 58, 232, 168, 255);  // ok
    inline constexpr ImU32 COL_TEXT      = IM_COL32(208, 224, 240, 255);
    inline constexpr ImU32 COL_TEXT_MUTE = IM_COL32(112, 133, 158, 255);
    inline constexpr ImU32 COL_TEXT_FAINT= IM_COL32( 74,  90, 110, 255);

    // Latin + Latin-1 + general punctuation (dashes, quotes, bullets) + Cyrillic.
    // The stock Cyrillic range lacks U+2014 etc., which then render as '?'.
    inline const ImWchar* GlyphRanges()
    {
        static const ImWchar ranges[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x0100, 0x017F, // Latin Extended-A (o'zbek harflari uchun)
            0x2000, 0x206F, // General punctuation: – — ‘ ’ “ ” • …
            0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
            0x2DE0, 0x2DFF, // Cyrillic Extended-A
            0xA640, 0xA69F, // Cyrillic Extended-B
            0,
        };
        return &ranges[0];
    }

    inline ImVec4 V4(ImU32 c) { return ImGui::ColorConvertU32ToFloat4(c); }
    inline ImU32  Fade(ImU32 c, float a)
    {
        ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
        v.w *= a;
        return ImGui::ColorConvertFloat4ToU32(v);
    }
    inline ImU32  Mix(ImU32 a, ImU32 b, float t)
    {
        ImVec4 x = ImGui::ColorConvertU32ToFloat4(a), y = ImGui::ColorConvertU32ToFloat4(b);
        return ImGui::ColorConvertFloat4ToU32(ImVec4(
            x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t,
            x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t));
    }

    // -----------------------------------------------------------------
    //  ANIMATION  (per-widget eased state, single UI thread)
    // -----------------------------------------------------------------
    inline std::map<ImGuiID, float> g_mapAnim;

    inline float Anim(ImGuiID id, float flTarget, float flSpeed = 12.f)
    {
        float& flCur = g_mapAnim[id];
        float flStep = ImClamp(ImGui::GetIO().DeltaTime * flSpeed, 0.f, 1.f);
        flCur += (flTarget - flCur) * flStep;
        if (fabsf(flTarget - flCur) < 0.001f) flCur = flTarget;
        return flCur;
    }

    inline float Pulse(float flSpeed = 2.f)
    {
        return (sinf((float)ImGui::GetTime() * flSpeed) + 1.f) * 0.5f;
    }

    // -----------------------------------------------------------------
    //  DECORATION PRIMITIVES
    // -----------------------------------------------------------------

    // Soft outer glow around a rect (cheap, layered strokes)
    inline void GlowRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImU32 col, float rounding = 3.f, int nLayers = 5, float flIntensity = 1.f)
    {
        for (int i = nLayers; i >= 1; i--)
        {
            float f = (float)i;
            float a = (0.10f * flIntensity) * (1.f - (f - 1.f) / (float)nLayers);
            dl->AddRect(ImVec2(mn.x - f, mn.y - f), ImVec2(mx.x + f, mx.y + f),
                Fade(col, a), rounding + f, 0, 1.f);
        }
    }

    // Technical corner brackets — the signature cyberpunk frame
    inline void Brackets(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImU32 col, float flLen = 14.f, float flThick = 1.6f)
    {
        dl->AddLine(ImVec2(mn.x, mn.y), ImVec2(mn.x + flLen, mn.y), col, flThick);
        dl->AddLine(ImVec2(mn.x, mn.y), ImVec2(mn.x, mn.y + flLen), col, flThick);
        dl->AddLine(ImVec2(mx.x - flLen, mn.y), ImVec2(mx.x, mn.y), col, flThick);
        dl->AddLine(ImVec2(mx.x, mn.y), ImVec2(mx.x, mn.y + flLen), col, flThick);
        dl->AddLine(ImVec2(mn.x, mx.y - flLen), ImVec2(mn.x, mx.y), col, flThick);
        dl->AddLine(ImVec2(mn.x, mx.y), ImVec2(mn.x + flLen, mx.y), col, flThick);
        dl->AddLine(ImVec2(mx.x - flLen, mx.y), ImVec2(mx.x, mx.y), col, flThick);
        dl->AddLine(ImVec2(mx.x, mx.y - flLen), ImVec2(mx.x, mx.y), col, flThick);
    }

    // Faint technical grid (blueprint feel)
    inline void Grid(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImU32 col, float flStep = 26.f)
    {
        for (float x = mn.x; x < mx.x; x += flStep)
            dl->AddLine(ImVec2(x, mn.y), ImVec2(x, mx.y), col, 1.f);
        for (float y = mn.y; y < mx.y; y += flStep)
            dl->AddLine(ImVec2(mn.x, y), ImVec2(mx.x, y), col, 1.f);
    }

    // Horizontal scanlines
    inline void Scanlines(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImU32 col, float flStep = 3.f)
    {
        for (float y = mn.y; y < mx.y; y += flStep)
            dl->AddLine(ImVec2(mn.x, y), ImVec2(mx.x, y), col, 1.f);
    }

    // Neon divider that fades out to the right
    inline void NeonLine(ImDrawList* dl, ImVec2 p, float flWidth, ImU32 col, float flThick = 1.f)
    {
        dl->AddRectFilledMultiColor(p, ImVec2(p.x + flWidth, p.y + flThick),
            col, Fade(col, 0.f), Fade(col, 0.f), col);
    }

    // -----------------------------------------------------------------
    //  VECTOR ICONS  (no icon font needed)
    // -----------------------------------------------------------------
    enum EIcon
    {
        ICON_EYE = 0, ICON_MOVE, ICON_CROSSHAIR, ICON_RADAR,
        ICON_GRENADE, ICON_CASE, ICON_DISK, ICON_GEAR, ICON_LOCK, ICON_BOLT
    };

    inline void Icon(ImDrawList* dl, ImVec2 c, float s, int eIcon, ImU32 col)
    {
        const float r = s * 0.5f;
        switch (eIcon)
        {
        case ICON_EYE:
        {
            dl->PathClear();
            dl->PathLineTo(ImVec2(c.x - r, c.y));
            dl->PathBezierQuadraticCurveTo(ImVec2(c.x, c.y - r * 1.15f), ImVec2(c.x + r, c.y), 12);
            dl->PathBezierQuadraticCurveTo(ImVec2(c.x, c.y + r * 1.15f), ImVec2(c.x - r, c.y), 12);
            dl->PathStroke(col, 0, 1.5f);
            dl->AddCircleFilled(c, r * 0.34f, col, 10);
            break;
        }
        case ICON_MOVE:
        {
            for (int i = 0; i < 2; i++)
            {
                float x = c.x - r * 0.75f + i * r * 0.8f;
                dl->PathClear();
                dl->PathLineTo(ImVec2(x, c.y - r * 0.7f));
                dl->PathLineTo(ImVec2(x + r * 0.6f, c.y));
                dl->PathLineTo(ImVec2(x, c.y + r * 0.7f));
                dl->PathStroke(Fade(col, i == 0 ? 0.55f : 1.f), 0, 1.6f);
            }
            break;
        }
        case ICON_CROSSHAIR:
        {
            dl->AddCircle(c, r * 0.72f, col, 20, 1.4f);
            dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.45f, c.y), col, 1.4f);
            dl->AddLine(ImVec2(c.x + r * 0.45f, c.y), ImVec2(c.x + r, c.y), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y - r * 0.45f), col, 1.4f);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.45f), ImVec2(c.x, c.y + r), col, 1.4f);
            dl->AddCircleFilled(c, 1.3f, col, 6);
            break;
        }
        case ICON_RADAR:
        {
            dl->AddCircle(c, r, Fade(col, 0.55f), 24, 1.2f);
            dl->AddCircle(c, r * 0.55f, Fade(col, 0.35f), 18, 1.f);
            float a = (float)ImGui::GetTime() * 2.2f;
            dl->AddLine(c, ImVec2(c.x + cosf(a) * r, c.y + sinf(a) * r), col, 1.4f);
            dl->AddCircleFilled(ImVec2(c.x + cosf(a - 0.7f) * r * 0.6f, c.y + sinf(a - 0.7f) * r * 0.6f), 1.6f, col, 6);
            break;
        }
        case ICON_GRENADE:
        {
            dl->AddCircle(ImVec2(c.x, c.y + r * 0.18f), r * 0.68f, col, 18, 1.5f);
            dl->AddLine(ImVec2(c.x - r * 0.22f, c.y - r * 0.55f), ImVec2(c.x + r * 0.22f, c.y - r * 0.55f), col, 1.5f);
            dl->AddLine(ImVec2(c.x + r * 0.22f, c.y - r * 0.55f), ImVec2(c.x + r * 0.62f, c.y - r * 0.9f), col, 1.3f);
            break;
        }
        case ICON_CASE:
        {
            dl->AddRect(ImVec2(c.x - r, c.y - r * 0.6f), ImVec2(c.x + r, c.y + r * 0.8f), col, 2.f, 0, 1.4f);
            dl->AddLine(ImVec2(c.x - r, c.y + r * 0.05f), ImVec2(c.x + r, c.y + r * 0.05f), Fade(col, 0.6f), 1.2f);
            dl->AddLine(ImVec2(c.x - r * 0.35f, c.y - r * 0.6f), ImVec2(c.x - r * 0.35f, c.y - r), col, 1.2f);
            dl->AddLine(ImVec2(c.x + r * 0.35f, c.y - r * 0.6f), ImVec2(c.x + r * 0.35f, c.y - r), col, 1.2f);
            dl->AddLine(ImVec2(c.x - r * 0.35f, c.y - r), ImVec2(c.x + r * 0.35f, c.y - r), col, 1.2f);
            break;
        }
        case ICON_DISK:
        {
            dl->AddRect(ImVec2(c.x - r, c.y - r), ImVec2(c.x + r, c.y + r), col, 1.5f, 0, 1.4f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.45f, c.y - r), ImVec2(c.x + r * 0.45f, c.y - r * 0.25f), Fade(col, 0.7f), 1.f);
            dl->AddRect(ImVec2(c.x - r * 0.6f, c.y + r * 0.15f), ImVec2(c.x + r * 0.6f, c.y + r), Fade(col, 0.7f), 1.f, 0, 1.2f);
            break;
        }
        case ICON_GEAR:
        {
            dl->AddCircle(c, r * 0.70f, col, 20, 1.4f);
            dl->AddCircle(c, r * 0.30f, Fade(col, 0.75f), 10, 1.2f);
            for (int i = 0; i < 6; i++)
            {
                float a = (float)i * (IM_PI / 3.f) + 0.26f;
                dl->AddLine(ImVec2(c.x + cosf(a) * r * 0.66f, c.y + sinf(a) * r * 0.66f),
                            ImVec2(c.x + cosf(a) * r, c.y + sinf(a) * r), col, 1.8f);
            }
            break;
        }
        case ICON_LOCK:
        {
            dl->AddRectFilled(ImVec2(c.x - r * 0.7f, c.y - r * 0.1f), ImVec2(c.x + r * 0.7f, c.y + r * 0.8f), col, 1.5f);
            dl->PathClear();
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.1f), r * 0.42f, IM_PI, IM_PI * 2.f, 12);
            dl->PathStroke(col, 0, 1.8f);
            break;
        }
        case ICON_BOLT:
        {
            dl->PathClear();
            dl->PathLineTo(ImVec2(c.x + r * 0.35f, c.y - r));
            dl->PathLineTo(ImVec2(c.x - r * 0.45f, c.y + r * 0.1f));
            dl->PathLineTo(ImVec2(c.x + r * 0.05f, c.y + r * 0.1f));
            dl->PathLineTo(ImVec2(c.x - r * 0.3f, c.y + r));
            dl->PathLineTo(ImVec2(c.x + r * 0.5f, c.y - r * 0.15f));
            dl->PathLineTo(ImVec2(c.x, c.y - r * 0.15f));
            dl->PathFillConvex(col);
            break;
        }
        default: break;
        }
    }

    // -----------------------------------------------------------------
    //  THEME
    // -----------------------------------------------------------------
    inline void ApplyTheme()
    {
        ImGuiStyle& s = ImGui::GetStyle();

        s.WindowRounding    = 6.f;
        s.ChildRounding     = 3.f;
        s.FrameRounding     = 2.f;
        s.PopupRounding     = 3.f;
        s.ScrollbarRounding = 2.f;
        s.GrabRounding      = 2.f;
        s.TabRounding       = 2.f;

        s.WindowBorderSize  = 0.f;
        s.ChildBorderSize   = 1.f;
        s.FrameBorderSize   = 1.f;
        s.PopupBorderSize   = 1.f;

        s.WindowPadding     = ImVec2(0.f, 0.f);
        s.FramePadding      = ImVec2(9.f, 5.f);
        s.ItemSpacing       = ImVec2(9.f, 7.f);
        s.ItemInnerSpacing  = ImVec2(7.f, 5.f);
        s.ScrollbarSize     = 9.f;
        s.GrabMinSize       = 9.f;
        s.WindowTitleAlign  = ImVec2(0.f, 0.5f);

        ImVec4* c = s.Colors;
        c[ImGuiCol_WindowBg]             = V4(COL_BG);
        c[ImGuiCol_ChildBg]              = V4(IM_COL32(0, 0, 0, 0));
        c[ImGuiCol_PopupBg]              = V4(IM_COL32(9, 13, 20, 250));

        c[ImGuiCol_Border]               = V4(COL_LINE);
        c[ImGuiCol_BorderShadow]         = V4(IM_COL32(0, 0, 0, 0));

        c[ImGuiCol_FrameBg]              = V4(COL_PANEL_HI);
        c[ImGuiCol_FrameBgHovered]       = V4(COL_PANEL_ACT);
        c[ImGuiCol_FrameBgActive]        = V4(IM_COL32(28, 40, 58, 255));

        c[ImGuiCol_TitleBg]              = V4(COL_BG_DEEP);
        c[ImGuiCol_TitleBgActive]        = V4(COL_BG_DEEP);
        c[ImGuiCol_TitleBgCollapsed]     = V4(COL_BG_DEEP);
        c[ImGuiCol_MenuBarBg]            = V4(COL_BG_DEEP);

        c[ImGuiCol_ScrollbarBg]          = V4(IM_COL32(10, 14, 21, 160));
        c[ImGuiCol_ScrollbarGrab]        = V4(IM_COL32(46, 88, 112, 255));
        c[ImGuiCol_ScrollbarGrabHovered] = V4(COL_TEAL);
        c[ImGuiCol_ScrollbarGrabActive]  = V4(COL_CYAN);

        c[ImGuiCol_CheckMark]            = V4(COL_CYAN);
        c[ImGuiCol_SliderGrab]           = V4(COL_CYAN);
        c[ImGuiCol_SliderGrabActive]     = V4(IM_COL32(160, 244, 255, 255));

        c[ImGuiCol_Button]               = V4(IM_COL32(18, 26, 38, 255));
        c[ImGuiCol_ButtonHovered]        = V4(IM_COL32(26, 40, 58, 255));
        c[ImGuiCol_ButtonActive]         = V4(IM_COL32(34, 54, 76, 255));

        c[ImGuiCol_Header]               = V4(IM_COL32(20, 30, 44, 255));
        c[ImGuiCol_HeaderHovered]        = V4(IM_COL32(28, 44, 64, 255));
        c[ImGuiCol_HeaderActive]         = V4(IM_COL32(16, 62, 82, 255));

        c[ImGuiCol_Separator]            = V4(COL_LINE);
        c[ImGuiCol_SeparatorHovered]     = V4(COL_TEAL);
        c[ImGuiCol_SeparatorActive]      = V4(COL_CYAN);

        c[ImGuiCol_ResizeGrip]           = V4(IM_COL32(0, 0, 0, 0));
        c[ImGuiCol_ResizeGripHovered]    = V4(COL_CYAN_SOFT);
        c[ImGuiCol_ResizeGripActive]     = V4(COL_CYAN);

        c[ImGuiCol_Tab]                  = V4(IM_COL32(12, 17, 26, 255));
        c[ImGuiCol_TabHovered]           = V4(COL_TEAL);
        c[ImGuiCol_TabActive]            = V4(IM_COL32(16, 62, 82, 255));
        c[ImGuiCol_TabUnfocused]         = V4(IM_COL32(12, 17, 26, 255));
        c[ImGuiCol_TabUnfocusedActive]   = V4(IM_COL32(16, 44, 60, 255));

        c[ImGuiCol_PlotLines]            = V4(COL_CYAN);
        c[ImGuiCol_PlotLinesHovered]     = V4(COL_MAGENTA);
        c[ImGuiCol_PlotHistogram]        = V4(COL_CYAN);
        c[ImGuiCol_PlotHistogramHovered] = V4(COL_MAGENTA);

        c[ImGuiCol_TableHeaderBg]        = V4(COL_PANEL_HI);
        c[ImGuiCol_TableBorderLight]     = V4(IM_COL32(26, 36, 52, 255));
        c[ImGuiCol_TableBorderStrong]    = V4(COL_LINE);

        c[ImGuiCol_TextSelectedBg]       = V4(Fade(COL_CYAN, 0.25f));
        c[ImGuiCol_Text]                 = V4(COL_TEXT);
        c[ImGuiCol_TextDisabled]         = V4(COL_TEXT_FAINT);
        c[ImGuiCol_NavHighlight]         = V4(COL_CYAN_SOFT);
        c[ImGuiCol_DragDropTarget]       = V4(COL_MAGENTA);
        c[ImGuiCol_ModalWindowDimBg]     = V4(IM_COL32(2, 4, 8, 190));
    }

    // -----------------------------------------------------------------
    //  TEXT HELPERS
    // -----------------------------------------------------------------
    inline void TextC(ImU32 col, const char* fmt, ...)
    {
        va_list args; va_start(args, fmt);
        ImGui::PushStyleColor(ImGuiCol_Text, V4(col));
        ImGui::TextV(fmt, args);
        ImGui::PopStyleColor();
        va_end(args);
    }

    // Small dimmed explanation line under a control
    inline void Hint(const char* fmt, ...)
    {
        va_list args; va_start(args, fmt);
        char buf[512];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        if (Fonts::Small) ImGui::PushFont(Fonts::Small);
        ImGui::PushStyleColor(ImGuiCol_Text, V4(COL_TEXT_MUTE));
        ImGui::TextWrapped("%s", buf);
        ImGui::PopStyleColor();
        if (Fonts::Small) ImGui::PopFont();
    }

    // Colored status/notice line with a leading marker
    inline void Notice(ImU32 col, const char* fmt, ...)
    {
        va_list args; va_start(args, fmt);
        char buf[512];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = ImGui::GetTextLineHeight();
        dl->AddRectFilled(ImVec2(p.x, p.y + 1.f), ImVec2(p.x + 2.f, p.y + h - 1.f), col, 1.f);
        ImGui::Indent(9.f);
        ImGui::PushStyleColor(ImGuiCol_Text, V4(col));
        ImGui::TextWrapped("%s", buf);
        ImGui::PopStyleColor();
        ImGui::Unindent(9.f);
    }

    // Uppercase section label with neon rule
    inline void Section(const char* szTitle)
    {
        ImGui::Dummy(ImVec2(0.f, 2.f));
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = ImGui::GetTextLineHeight();

        dl->AddRectFilled(ImVec2(p.x, p.y + 2.f), ImVec2(p.x + 3.f, p.y + h - 1.f), COL_CYAN, 1.f);
        GlowRect(dl, ImVec2(p.x, p.y + 2.f), ImVec2(p.x + 3.f, p.y + h - 1.f), COL_CYAN, 1.f, 3, 0.9f);

        ImGui::Indent(11.f);
        ImGui::PushStyleColor(ImGuiCol_Text, V4(COL_TEXT));
        ImGui::TextUnformatted(szTitle);
        ImGui::PopStyleColor();
        ImGui::Unindent(11.f);

        ImVec2 pl = ImGui::GetCursorScreenPos();
        NeonLine(dl, ImVec2(pl.x, pl.y + 1.f), ImGui::GetContentRegionAvail().x, Fade(COL_CYAN, 0.35f), 1.f);
        ImGui::Dummy(ImVec2(0.f, 4.f));
    }

    // Pill badge
    inline void Chip(const char* szText, ImU32 col, ImVec2 posScreen)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 ts = ImGui::CalcTextSize(szText);
        ImVec2 mn = posScreen;
        ImVec2 mx = ImVec2(mn.x + ts.x + 14.f, mn.y + ts.y + 5.f);
        dl->AddRectFilled(mn, mx, Fade(col, 0.14f), 2.f);
        dl->AddRect(mn, mx, Fade(col, 0.75f), 2.f, 0, 1.f);
        dl->AddText(ImVec2(mn.x + 7.f, mn.y + 2.f), col, szText);
    }

    inline float ChipWidth(const char* szText) { return ImGui::CalcTextSize(szText).x + 14.f; }

    // -----------------------------------------------------------------
    //  CARD  (variable height, drawn behind content via draw-list split)
    // -----------------------------------------------------------------
    struct CardCtx
    {
        ImDrawListSplitter splitter;
        ImVec2             mn;
        float              width;
        float              col2X;   // window-local x of the second column
        bool               active;
    };
    inline CardCtx g_Card;

    inline void BeginCard(const char* szTitle = nullptr)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        g_Card.active = true;
        g_Card.width  = ImGui::GetContentRegionAvail().x;
        g_Card.mn     = ImGui::GetCursorScreenPos();
        g_Card.col2X  = (g_Card.mn.x - ImGui::GetWindowPos().x) + g_Card.width * 0.5f;

        g_Card.splitter.Split(dl, 2);
        g_Card.splitter.SetCurrentChannel(dl, 1);

        ImGui::Dummy(ImVec2(0.f, 9.f));
        ImGui::Indent(14.f);
        ImGui::PushItemWidth(g_Card.width - 28.f);

        if (szTitle)
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            float h = ImGui::GetTextLineHeight();
            dl->AddRectFilled(ImVec2(p.x - 6.f, p.y + 2.f), ImVec2(p.x - 3.f, p.y + h - 1.f), COL_CYAN, 1.f);
            ImGui::PushStyleColor(ImGuiCol_Text, V4(COL_TEXT));
            ImGui::TextUnformatted(szTitle);
            ImGui::PopStyleColor();

            ImVec2 pl = ImGui::GetCursorScreenPos();
            NeonLine(dl, ImVec2(pl.x - 6.f, pl.y + 1.f), g_Card.width - 22.f, Fade(COL_CYAN, 0.30f), 1.f);
            ImGui::Dummy(ImVec2(0.f, 3.f));
        }
    }

    inline void EndCard()
    {
        if (!g_Card.active) return;
        ImGui::PopItemWidth();
        ImGui::Unindent(14.f);
        ImGui::Dummy(ImVec2(0.f, 8.f));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 mn = g_Card.mn;
        ImVec2 mx = ImVec2(mn.x + g_Card.width, ImGui::GetCursorScreenPos().y);

        g_Card.splitter.SetCurrentChannel(dl, 0);
        dl->AddRectFilled(mn, mx, COL_PANEL, 4.f);
        dl->AddRect(mn, mx, IM_COL32(26, 36, 52, 255), 4.f, 0, 1.f);
        // top neon edge
        dl->AddRectFilledMultiColor(mn, ImVec2(mn.x + g_Card.width * 0.6f, mn.y + 1.f),
            Fade(COL_CYAN, 0.85f), Fade(COL_CYAN, 0.f), Fade(COL_CYAN, 0.f), Fade(COL_CYAN, 0.85f));
        Brackets(dl, ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), Fade(COL_CYAN, 0.30f), 10.f, 1.2f);
        g_Card.splitter.Merge(dl);

        g_Card.active = false;
        ImGui::Dummy(ImVec2(0.f, 3.f));
    }

    // -----------------------------------------------------------------
    //  NAV ITEM  (sidebar)
    // -----------------------------------------------------------------
    inline bool NavItem(const char* szLabel, int eIcon, bool bActive, bool bLocked = false)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        const ImGuiID id  = win->GetID(szLabel);
        const float   w   = ImGui::GetContentRegionAvail().x;
        const float   h   = 36.f;
        ImVec2        pos = win->DC.CursorPos;
        ImRect        bb(pos, ImVec2(pos.x + w, pos.y + h));

        ImGui::ItemSize(bb, 0.f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

        float t = Anim(id, bActive ? 1.f : (hovered ? 0.45f : 0.f), 14.f);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (t > 0.01f)
        {
            dl->AddRectFilledMultiColor(bb.Min, bb.Max,
                Fade(COL_CYAN, 0.16f * t), Fade(COL_CYAN, 0.f),
                Fade(COL_CYAN, 0.f), Fade(COL_CYAN, 0.16f * t));
            dl->AddRectFilled(bb.Min, ImVec2(bb.Min.x + 2.5f, bb.Max.y), Fade(COL_CYAN, t), 1.f);
            if (bActive)
                GlowRect(dl, bb.Min, ImVec2(bb.Min.x + 2.5f, bb.Max.y), COL_CYAN, 1.f, 4, 1.2f);
        }

        ImU32 colFg = bLocked ? Mix(COL_TEXT_FAINT, COL_RED, 0.35f)
                              : Mix(COL_TEXT_MUTE, COL_CYAN, t);

        Icon(dl, ImVec2(bb.Min.x + 26.f, bb.Min.y + h * 0.5f), 15.f, eIcon, colFg);

        ImVec2 ts = ImGui::CalcTextSize(szLabel);
        dl->AddText(ImVec2(bb.Min.x + 46.f, bb.Min.y + (h - ts.y) * 0.5f),
            bActive ? COL_TEXT : colFg, szLabel);

        if (bLocked)
            Icon(dl, ImVec2(bb.Max.x - 18.f, bb.Min.y + h * 0.5f), 10.f, ICON_LOCK, Fade(COL_RED, 0.75f));

        return pressed;
    }

    // -----------------------------------------------------------------
    //  TOGGLE SWITCH
    // -----------------------------------------------------------------
    inline bool Toggle(const char* szLabel, bool* pValue)
    {
        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        ImGuiContext& g   = *GImGui;
        const ImGuiID id  = win->GetID(szLabel);
        const char*   end = ImGui::FindRenderedTextEnd(szLabel);
        const ImVec2  ls  = ImGui::CalcTextSize(szLabel, end);

        const float   hSw = ImGui::GetFrameHeight() * 0.82f;
        const float   wSw = hSw * 1.95f;
        ImVec2        pos = win->DC.CursorPos;
        const float   hRow = ImMax(hSw, ls.y);

        ImRect bb(pos, ImVec2(pos.x + wSw + (ls.x > 0.f ? g.Style.ItemInnerSpacing.x + ls.x : 0.f), pos.y + hRow));
        ImGui::ItemSize(bb, 0.f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        if (pressed)
        {
            *pValue = !*pValue;
            ImGui::MarkItemEdited(id);
        }

        const float t = Anim(id, *pValue ? 1.f : 0.f, 16.f);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImVec2 swMin(bb.Min.x, bb.Min.y + (hRow - hSw) * 0.5f);
        ImVec2 swMax(swMin.x + wSw, swMin.y + hSw);
        float  rad = hSw * 0.5f;

        ImU32 colTrack = Mix(IM_COL32(20, 27, 39, 255), Fade(COL_CYAN, 0.22f), t);
        ImU32 colEdge  = Mix(IM_COL32(38, 50, 70, 255), COL_CYAN, t);

        dl->AddRectFilled(swMin, swMax, colTrack, rad);
        dl->AddRect(swMin, swMax, hovered ? Mix(colEdge, COL_CYAN, 0.4f) : colEdge, rad, 0, 1.f);
        if (t > 0.02f)
            GlowRect(dl, swMin, swMax, COL_CYAN, rad, 4, t);

        ImVec2 knob(swMin.x + rad + (wSw - hSw) * t, swMin.y + rad);
        dl->AddCircleFilled(knob, rad - 2.5f, Mix(IM_COL32(78, 96, 118, 255), COL_CYAN, t), 18);
        if (t > 0.5f)
            dl->AddCircleFilled(knob, rad - 1.f, Fade(COL_CYAN, 0.25f * t), 18);

        if (ls.x > 0.f)
            dl->AddText(ImVec2(swMax.x + g.Style.ItemInnerSpacing.x, bb.Min.y + (hRow - ls.y) * 0.5f),
                *pValue ? COL_TEXT : (hovered ? COL_TEXT : COL_TEXT_MUTE), szLabel, end);

        return pressed;
    }

    // -----------------------------------------------------------------
    //  LABELLED CONTROLS
    // -----------------------------------------------------------------
    inline void FieldLabel(const char* szLabel)
    {
        if (!szLabel || !*szLabel) return;
        ImGui::PushStyleColor(ImGuiCol_Text, V4(COL_TEXT_MUTE));
        ImGui::TextUnformatted(szLabel);
        ImGui::PopStyleColor();
    }

    // Label + control are grouped into one item so 2-column rows line up.
    inline bool SliderF(const char* szLabel, const char* szId, float* v, float mn, float mx, const char* fmt, float width = 0.f)
    {
        ImGui::BeginGroup();
        FieldLabel(szLabel);
        if (width > 0.f) ImGui::SetNextItemWidth(width);
        else             ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
        bool r = ImGui::SliderFloat(szId, v, mn, mx, fmt);
        ImGui::EndGroup();
        return r;
    }

    inline bool SliderI(const char* szLabel, const char* szId, int* v, int mn, int mx, const char* fmt, float width = 0.f)
    {
        ImGui::BeginGroup();
        FieldLabel(szLabel);
        if (width > 0.f) ImGui::SetNextItemWidth(width);
        else             ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.72f);
        bool r = ImGui::SliderInt(szId, v, mn, mx, fmt);
        ImGui::EndGroup();
        return r;
    }

    inline bool Combo(const char* szLabel, const char* szId, int* v, const char* const items[], int count, float width = 0.f)
    {
        ImGui::BeginGroup();
        FieldLabel(szLabel);
        if (width > 0.f) ImGui::SetNextItemWidth(width);
        else             ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.6f);
        bool r = ImGui::Combo(szId, v, items, count);
        ImGui::EndGroup();
        return r;
    }

    inline void ColorPick(const char* szLabel, Color& col)
    {
        ImGui::BeginGroup();
        float arr[4] = { col.rBase(), col.gBase(), col.bBase(), col.aBase() };
        char szId[128];
        snprintf(szId, sizeof(szId), "##col_%s", szLabel);
        if (ImGui::ColorEdit4(szId, arr, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf))
            col.Set(arr[0], arr[1], arr[2], arr[3]);
        ImGui::SameLine(0.f, 9.f);
        ImGui::PushStyleColor(ImGuiCol_Text, V4(COL_TEXT_MUTE));
        ImGui::TextUnformatted(szLabel);
        ImGui::PopStyleColor();
        ImGui::EndGroup();
    }

    // -----------------------------------------------------------------
    //  BUTTONS
    // -----------------------------------------------------------------
    enum EButton { BTN_GHOST = 0, BTN_PRIMARY, BTN_DANGER, BTN_SUCCESS, BTN_WARN };

    inline bool Button(const char* szLabel, ImVec2 size, int eStyle = BTN_GHOST)
    {
        ImU32 accent = COL_CYAN;
        switch (eStyle)
        {
        case BTN_DANGER:  accent = COL_RED;     break;
        case BTN_SUCCESS: accent = COL_GREEN;   break;
        case BTN_WARN:    accent = COL_AMBER;   break;
        case BTN_PRIMARY: accent = COL_CYAN;    break;
        default:          accent = COL_TEAL;    break;
        }

        ImGuiWindow* win = ImGui::GetCurrentWindow();
        if (win->SkipItems) return false;

        const ImGuiID id  = win->GetID(szLabel);
        const char*   end = ImGui::FindRenderedTextEnd(szLabel);
        ImVec2        ts  = ImGui::CalcTextSize(szLabel, end);
        if (size.x <= 0.f) size.x = ts.x + 26.f;
        if (size.y <= 0.f) size.y = ImGui::GetFrameHeight() + 3.f;

        ImVec2 pos = win->DC.CursorPos;
        ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, 0.f);
        if (!ImGui::ItemAdd(bb, id)) return false;

        bool hovered, held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
        float t = Anim(id, held ? 1.f : (hovered ? 0.6f : 0.f), 18.f);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 colBg = (eStyle == BTN_PRIMARY)
            ? Mix(Fade(accent, 0.28f), Fade(accent, 0.55f), t)
            : Mix(IM_COL32(15, 21, 31, 255), Fade(accent, 0.22f), t);

        dl->AddRectFilled(bb.Min, bb.Max, colBg, 3.f);
        dl->AddRect(bb.Min, bb.Max, Mix(Fade(accent, 0.45f), accent, t), 3.f, 0, 1.f);
        if (t > 0.02f) GlowRect(dl, bb.Min, bb.Max, accent, 3.f, 4, t);

        // corner ticks
        dl->AddLine(ImVec2(bb.Min.x + 1.f, bb.Min.y + 1.f), ImVec2(bb.Min.x + 6.f, bb.Min.y + 1.f), Fade(accent, 0.8f), 1.f);
        dl->AddLine(ImVec2(bb.Max.x - 6.f, bb.Max.y - 1.f), ImVec2(bb.Max.x - 1.f, bb.Max.y - 1.f), Fade(accent, 0.8f), 1.f);

        dl->AddText(ImVec2(bb.Min.x + (size.x - ts.x) * 0.5f, bb.Min.y + (size.y - ts.y) * 0.5f),
            Mix(Mix(COL_TEXT, accent, 0.35f), COL_TEXT, t), szLabel, end);

        return pressed;
    }

    // -----------------------------------------------------------------
    //  KEY BIND
    // -----------------------------------------------------------------
    inline const char* KeyName(int vk)
    {
        switch (vk)
        {
        case 0:           return "Yo'q";
        case VK_LBUTTON:  return "LMB";
        case VK_RBUTTON:  return "RMB";
        case VK_MBUTTON:  return "MMB";
        case VK_XBUTTON1: return "Mouse4";
        case VK_XBUTTON2: return "Mouse5";
        case VK_SPACE:    return "Space";
        case VK_SHIFT: case VK_LSHIFT:   return "L.Shift";
        case VK_RSHIFT:   return "R.Shift";
        case VK_CONTROL: case VK_LCONTROL: return "L.Ctrl";
        case VK_RCONTROL: return "R.Ctrl";
        case VK_MENU: case VK_LMENU:     return "L.Alt";
        case VK_RMENU:    return "R.Alt";
        case VK_CAPITAL:  return "CapsLock";
        case VK_TAB:      return "Tab";
        case VK_INSERT:   return "Insert";
        case VK_DELETE:   return "Delete";
        case VK_HOME:     return "Home";
        case VK_END:      return "End";
        case VK_PRIOR:    return "PgUp";
        case VK_NEXT:     return "PgDn";
        case VK_RETURN:   return "Enter";
        case VK_BACK:     return "Backspace";
        default:
        {
            static char buf[16];
            if (vk >= VK_F1 && vk <= VK_F12) { snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1); return buf; }
            if (vk >= 'A' && vk <= 'Z')      { buf[0] = (char)vk; buf[1] = 0; return buf; }
            if (vk >= '0' && vk <= '9')      { buf[0] = (char)vk; buf[1] = 0; return buf; }
            snprintf(buf, sizeof(buf), "#%d", vk);
            return buf;
        }
        }
    }

    // Compact keybind capture widget:  LABEL  [ KEY ]
    inline bool Keybind(const char* szLabel, int& key, const char* szId)
    {
        static int*  s_pListening = nullptr;
        static float s_flTimer    = 0.f;

        bool bChanged   = false;
        bool bListening = (s_pListening == &key);

        ImGui::BeginGroup();
        if (szLabel && *szLabel)
        {
            FieldLabel(szLabel);
            ImGui::SameLine(0.f, 10.f);
        }

        char szBtn[96];
        if (bListening) snprintf(szBtn, sizeof(szBtn), "[ tugmani bosing ]##%s", szId);
        else            snprintf(szBtn, sizeof(szBtn), "[ %s ]##%s", KeyName(key), szId);

        if (Button(szBtn, ImVec2(150.f, 0.f), bListening ? BTN_WARN : BTN_GHOST))
        {
            if (bListening) { s_pListening = nullptr; }
            else            { s_pListening = &key; s_flTimer = 5.f; }
        }

        if (bListening)
        {
            s_flTimer -= ImGui::GetIO().DeltaTime;
            if (s_flTimer <= 0.f) s_pListening = nullptr;

            // 0.2s grace so the click that armed the widget is not captured
            if (s_flTimer < 4.8f)
            {
                for (int i = 1; i < 256; i++)
                {
                    if (!(GetAsyncKeyState(i) & 0x8000)) continue;
                    if (i != VK_ESCAPE) { key = i; bChanged = true; }
                    s_pListening = nullptr;
                    break;
                }
            }
            ImGui::SameLine(0.f, 8.f);
            ImGui::PushStyleColor(ImGuiCol_Text, V4(COL_TEXT_FAINT));
            ImGui::TextUnformatted("ESC = bekor");
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();

        return bChanged;
    }

    // -----------------------------------------------------------------
    //  LOCKED FEATURE CARD
    // -----------------------------------------------------------------
    inline void Locked(const char* szFeature, const char* szTier)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 mn = ImGui::GetCursorScreenPos();
        float  w  = ImGui::GetContentRegionAvail().x;
        float  h  = 62.f;
        ImVec2 mx(mn.x + w, mn.y + h);

        dl->AddRectFilled(mn, mx, IM_COL32(24, 10, 16, 235), 4.f);
        dl->AddRect(mn, mx, Fade(COL_RED, 0.45f), 4.f, 0, 1.f);
        Scanlines(dl, ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), IM_COL32(255, 71, 87, 10), 4.f);
        Brackets(dl, ImVec2(mn.x + 1.f, mn.y + 1.f), ImVec2(mx.x - 1.f, mx.y - 1.f), Fade(COL_RED, 0.7f), 10.f, 1.4f);

        Icon(dl, ImVec2(mn.x + 30.f, mn.y + h * 0.5f), 22.f, ICON_LOCK, Fade(COL_RED, 0.85f + 0.15f * Pulse(3.f)));

        char szTop[128];
        snprintf(szTop, sizeof(szTop), "%s  —  QULFLANGAN", szFeature);
        dl->AddText(ImVec2(mn.x + 56.f, mn.y + 14.f), COL_RED, szTop);

        char szBot[160];
        snprintf(szBot, sizeof(szBot), "%s darajasi kerak  ·  yangilash uchun: @bakoev_71", szTier);
        dl->AddText(ImVec2(mn.x + 56.f, mn.y + 34.f), COL_TEXT_MUTE, szBot);

        ImGui::Dummy(ImVec2(w, h + 4.f));
    }

    // -----------------------------------------------------------------
    //  LAYOUT HELPERS
    // -----------------------------------------------------------------
    // Place the next widget in the second column of a 2-up row.
    // Call directly after the first widget of the row.
    inline void Col2(float flOffset = 0.f)
    {
        if (g_Card.active) ImGui::SameLine(g_Card.col2X + flOffset, 0.f);
        else               ImGui::SameLine(ImGui::GetWindowContentRegionMax().x * 0.5f + flOffset, 0.f);
    }

    inline void Gap(float y = 6.f) { ImGui::Dummy(ImVec2(0.f, y)); }
}
