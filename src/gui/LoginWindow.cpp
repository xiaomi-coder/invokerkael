#include "../Includes.h"
#include <json.hpp>

// stb_image for background loading
#include "../../ext/stb/stb_image.h"

using json = nlohmann::json;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static bool s_bDrag = false;
static POINT s_ptDrag = {};

static LRESULT CALLBACK LoginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return 0;
    switch (msg)
    {
    case WM_LBUTTONDOWN: s_bDrag = true; GetCursorPos(&s_ptDrag); SetCapture(hWnd); return 0;
    case WM_MOUSEMOVE:
        if (s_bDrag) {
            POINT pt; GetCursorPos(&pt); RECT rc; GetWindowRect(hWnd, &rc);
            MoveWindow(hWnd, rc.left + pt.x - s_ptDrag.x, rc.top + pt.y - s_ptDrag.y,
                rc.right - rc.left, rc.bottom - rc.top, TRUE);
            s_ptDrag = pt;
        } return 0;
    case WM_LBUTTONUP: s_bDrag = false; ReleaseCapture(); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// Helpers
static bool IsCS2Running()
{
    PROCESSENTRY32 pe = {}; pe.dwSize = sizeof(pe);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    if (Process32First(snap, &pe)) {
        do { if (!strcmp(pe.szExeFile, "cs2.exe")) { found = true; break; } }
        while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

static bool LoadTextureFromFile(const char* filename, ID3D11Device* d3dDevice, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    d3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

// ===================================================================
bool LoginWindow::Create()
{
    if (m_bInitialized) return true;
    
    // YANADA KENGROQ OYNA (Kael/Invoker foni uchun maxsus 800x500 o'lcham)
    int wndW = 860, wndH = 500;
    int scrW = GetSystemMetrics(SM_CXSCREEN), scrH = GetSystemMetrics(SM_CYSCREEN);

    m_wc = {};
    m_wc.cbSize = sizeof(WNDCLASSEXW);
    m_wc.style = CS_CLASSDC;
    m_wc.lpfnWndProc = LoginWndProc;
    m_wc.hInstance = GetModuleHandleW(NULL);
    m_wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    m_wc.lpszClassName = L"KaeL_Cheat_Login";
    RegisterClassExW(&m_wc);

    m_hWnd = CreateWindowExW(0, m_wc.lpszClassName, L"KaeL Cheat",
        WS_POPUP | WS_VISIBLE, (scrW - wndW) / 2, (scrH - wndH) / 2, wndW, wndH,
        NULL, NULL, m_wc.hInstance, NULL);
    if (!m_hWnd) return false;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1; sd.BufferDesc.Width = wndW; sd.BufferDesc.Height = wndH;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sd.BufferDesc.RefreshRate = { 60, 1 };
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = m_hWnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL lvl[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL fl;
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        lvl, 2, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pDevice, &fl, &m_pContext) != S_OK)
        return false;

    ID3D11Texture2D* bb = nullptr;
    m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    if (bb) { m_pDevice->CreateRenderTargetView(bb, NULL, &m_pRTV); bb->Release(); }

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(m_hWnd);
    ImGui_ImplDX11_Init(m_pDevice, m_pContext);

    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig cfg = {};
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_Bold;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", 18, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGuiFreeType::BuildFontAtlas(io.Fonts, 0);

    m_bInitialized = true;
    return true;
}

static void ApplyKaelTheme()
{
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0; s.FrameRounding = 6; s.GrabRounding = 6;
    s.WindowBorderSize = 0; s.FrameBorderSize = 0;
    s.ItemSpacing = { 8, 12 }; s.FramePadding = { 12, 10 };
    auto* c = s.Colors;
    
    // Kael theme (Dark red/orange accents)
    c[ImGuiCol_WindowBg]       = { 0.05f, 0.05f, 0.05f, 0.0f }; // Asos oynani shaffof qildik, orqadan rasm ko'rinishi uchun
    c[ImGuiCol_FrameBg]        = { 0.15f, 0.12f, 0.12f, 0.8f };
    c[ImGuiCol_FrameBgHovered] = { 0.20f, 0.15f, 0.15f, 0.9f };
    c[ImGuiCol_FrameBgActive]  = { 0.8f,  0.4f,  0.1f,  0.6f };
    c[ImGuiCol_Text]           = { 0.95f, 0.90f, 0.85f, 1.0f };
    c[ImGuiCol_TextDisabled]   = { 0.50f, 0.45f, 0.40f, 1.0f };
    c[ImGuiCol_Button]         = { 0.70f, 0.35f, 0.10f, 1.0f };
    c[ImGuiCol_ButtonHovered]  = { 0.85f, 0.45f, 0.15f, 1.0f };
    c[ImGuiCol_ButtonActive]   = { 0.95f, 0.55f, 0.20f, 1.0f };
    c[ImGuiCol_CheckMark]      = { 1.00f, 0.60f, 0.20f, 1.0f };
}

enum class EPhase { LOGIN, CONNECTING, LOADING, READY, DONE };

bool LoginWindow::Run()
{
    if (!m_bInitialized) return false;
    ApplyKaelTheme();

    // Rasm uchun o'zgaruvchilar
    ID3D11ShaderResourceView* bgTexture = nullptr;
    int bgW = 0, bgH = 0;
    bool bHasBg = LoadTextureFromFile("kael_bg.jpg", m_pDevice, &bgTexture, &bgW, &bgH);

    EPhase ePhase = EPhase::LOGIN; // Dual Mode support
    g_License.m_strUser = "";
    g_License.m_eTier = ETier::LITE;
    char szUser[64] = "", szPass[64] = "";
    std::string strError;
    float flTimer = 0.f, flPulse = 0.f;

    struct Step {
        const char* name;
        const char* icon;
        float prog; bool done;
    };
    Step steps[] = {
        { "Litsenziyani tekshirish",  "[ AUTH ]", 0, false },
        { "Fayllarni yuklash",        "[ INIT ]", 0, false },
        { "Counter-Strike 2",         "[ GAME ]", 0, false },
        { "Dasturni sozlash",         "[ CORE ]", 0, false },
    };
    int nSteps = 4, nCur = 0;
    bool bCS2Found = false;
    float flCS2CheckTimer = 0.f;

    bool bRunning = true;
    while (bRunning)
    {
        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg); DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) { bRunning = false; exit(0); }
        }
        if (!bRunning) break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGuiIO& io = ImGui::GetIO();
        float dt = io.DeltaTime;
        float W = io.DisplaySize.x, H = io.DisplaySize.y;
        flPulse += dt;

        ImGui::SetNextWindowPos({ 0, 0 });
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##Main", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse);

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // 1. Fon Rasmini Chizish
        if (bHasBg)
        {
            dl->AddImage((ImTextureID)bgTexture, ImVec2(0, 0), ImVec2(W, H));
        }
        else
        {
            // Agar rasm topilmasa, oddiy qora/qizg'ish gradient qilinadi
            dl->AddRectFilledMultiColor(ImVec2(0, 0), ImVec2(W, H),
                IM_COL32(15, 10, 10, 255), IM_COL32(40, 15, 10, 255),
                IM_COL32(20, 10, 10, 255), IM_COL32(10, 5, 5, 255));
        }

        // X Button (Top Right)
        {
            ImGui::SetCursorPos({ W - 35, 8 });
            ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 0 });
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.2f, 0.2f, 0.8f });
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.9f, 0.2f, 0.2f, 1.0f });
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.8f, 0.8f, 1.0f });
            if (ImGui::Button("X", { 25, 25 })) exit(0);
            ImGui::PopStyleColor(4);
        }

        // 2. Login Oyna Dizayni (O'ng tomonda quyuq panel)
        float panelW = 380;
        float panelX = W - panelW - 30; // O'ng chekkadan sal ichkarida
        float panelY = 40;
        float panelH = H - 80;

        // Panel Orqa Foni (Shaffof qora/to'q jigar rang)
        dl->AddRectFilled(ImVec2(panelX, panelY), ImVec2(panelX + panelW, panelY + panelH), IM_COL32(12, 10, 10, 220), 12.f);
        dl->AddRect(ImVec2(panelX, panelY), ImVec2(panelX + panelW, panelY + panelH), IM_COL32(150, 80, 20, 150), 12.f, 0, 2.f);

        // ===============================================================
        //  LOGIN SCREEN
        // ===============================================================
        if (ePhase == EPhase::LOGIN || ePhase == EPhase::CONNECTING)
        {
            float fW = 300.f, fX = panelX + (panelW - fW) * 0.5f;

            // Sarlavha
            ImGui::SetCursorPosY(panelY + 40);
            {
                const char* t = "P R O J E C T   K A E L";
                ImGui::SetCursorPosX(panelX + (panelW - ImGui::CalcTextSize(t).x) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.65f, 0.20f, 1.0f });
                ImGui::Text("%s", t);
                ImGui::PopStyleColor();
            }
            {
                const char* st = "INVOKER X CS2 OPERATIVE";
                ImGui::SetCursorPosY(panelY + 65);
                ImGui::SetCursorPosX(panelX + (panelW - ImGui::CalcTextSize(st).x) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, { 0.6f, 0.5f, 0.4f, 1.0f });
                ImGui::Text("%s", st);
                ImGui::PopStyleColor();
            }

            dl->AddLine({ panelX + 30, panelY + 95 }, { panelX + panelW - 30, panelY + 95 }, IM_COL32(180, 80, 20, 100));

            ImGui::SetCursorPos({ fX, panelY + 130 });

            // USERNAME
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.7f, 0.6f, 1 }); ImGui::Text("Foydalanuvchi nomi"); ImGui::PopStyleColor();
            ImGui::SetCursorPosX(fX); ImGui::PushItemWidth(fW);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 14, 12 });
            bool e1 = ImGui::InputText("##user", szUser, sizeof(szUser), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleVar(); ImGui::PopItemWidth();
            
            // PASSWORD
            ImGui::SetCursorPosX(fX);
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.7f, 0.6f, 1 }); ImGui::Text("Parol"); ImGui::PopStyleColor();
            ImGui::SetCursorPosX(fX); ImGui::PushItemWidth(fW);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 14, 12 });
            bool e2 = ImGui::InputText("##pass", szPass, sizeof(szPass), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleVar(); ImGui::PopItemWidth();
            
            ImGui::Spacing(); ImGui::Spacing();

            if (ePhase == EPhase::LOGIN)
            {
                // KIRISH BUTTON
                ImGui::SetCursorPosX(fX);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 16 });
                bool bClickPro = ImGui::Button("P R O  L O G I N", { fW, 0 });
                
                ImGui::SetCursorPosX(fX);
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.2f, 1.0f });
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.3f, 0.3f, 1.0f });
                bool bClickFree = ImGui::Button("B E P U L  K I R I S H   (FREE)", { fW, 0 });
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar();

                if (bClickFree)
                {
                    g_License.m_strToken = "free_token";
                    g_License.m_strUser = "BEPUL (Free)";
                    g_License.m_eTier = ETier::LITE;
                    g_License.m_strExpiry = "N/A";
                    ePhase = EPhase::LOADING; nCur = 0;
                    for (int i = 0; i < nSteps; i++) { steps[i].prog = 0; steps[i].done = false; }
                    bCS2Found = false;
                }
                else if ((bClickPro || e1 || e2) && strlen(szUser) > 0 && strlen(szPass) > 0)
                { strError.clear(); ePhase = EPhase::CONNECTING; flTimer = 0; }
                else if (bClickPro || e1 || e2)
                    strError = "Ma'lumotlar to'liq kiritilmadi!";
            }
            else // CONNECTING
            {
                ImGui::SetCursorPosX(fX);
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.65f, 0.20f, 1 });
                ImGui::Text("Tizimga ulanmoqda..."); ImGui::PopStyleColor();

                flTimer += dt; float prog = fminf(flTimer / 1.5f, 1.f);
                ImGui::SetCursorPosX(fX); ImVec2 bp = ImGui::GetCursorScreenPos();
                dl->AddRectFilled(bp, { bp.x + fW, bp.y + 5 }, IM_COL32(35, 20, 20, 255), 2);
                dl->AddRectFilled(bp, { bp.x + fW * prog, bp.y + 5 }, IM_COL32(200, 100, 30, 255), 2);

                if (flTimer >= 1.5f)
                {
                    json jBody; jBody["username"] = std::string(szUser); jBody["password"] = std::string(szPass);
                    Http::Response resp = Http::Post(g_License.m_strApiUrl + "/api/auth/login", jBody.dump());

                    if (!resp.success || resp.body.empty())
                    {
                        strError = (resp.statusCode == 0) ? "Server g'oyib bo'ldi!" : "Xato (kod: " + std::to_string(resp.statusCode) + ")";
                        try { json j = json::parse(resp.body); strError = j.value("error", strError); } catch (...) {}
                        ePhase = EPhase::LOGIN;
                    }
                    else
                    {
                        try {
                            json jr = json::parse(resp.body);
                            g_License.m_strToken = jr.value("token", "");
                            g_License.m_strUser = jr["user"].value("username", std::string(szUser));
                            std::string t = jr["user"].value("tier", "free");
                            g_License.m_eTier = (t == "pro") ? ETier::PRO : (t == "mid") ? ETier::MID : ETier::LITE;
                            g_License.m_strExpiry = jr["user"].value("expires_at", "N/A");
                            ePhase = EPhase::LOADING; nCur = 0;
                            for (int i = 0; i < nSteps; i++) { steps[i].prog = 0; steps[i].done = false; }
                            bCS2Found = false;
                        } catch (...) { strError = "Server ma'lumoti nostandart!"; ePhase = EPhase::LOGIN; }
                    }
                }
            }

            if (!strError.empty())
            {
                ImGui::Spacing(); ImGui::Spacing();
                ImGui::SetCursorPosX(panelX + (panelW - ImGui::CalcTextSize(strError.c_str()).x) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, { 1, 0.3f, 0.2f, 1 });
                ImGui::TextWrapped("%s", strError.c_str()); ImGui::PopStyleColor();
            }
        }
        else if (ePhase == EPhase::LOADING || ePhase == EPhase::READY)
        {
            float fW = 320.f, fX = panelX + (panelW - fW) * 0.5f;

            // H HEADER
            ImGui::SetCursorPosY(panelY + 30);
            {
                const char* t = "K A E L  C H E A T";
                ImGui::SetCursorPosX(panelX + (panelW - ImGui::CalcTextSize(t).x) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 0.65f, 0.2f, 1 });
                ImGui::Text("%s", t); ImGui::PopStyleColor();
            }
            dl->AddLine({ panelX + 30, panelY + 60 }, { panelX + panelW - 30, panelY + 60 }, IM_COL32(180, 80, 20, 100));

            // USER INFO
            ImGui::SetCursorPosY(panelY + 80);
            {
                std::string nameUp = g_License.m_strUser;
                for (auto& ch : nameUp) ch = (char)toupper((unsigned char)ch);
                char info[128];
                snprintf(info, sizeof(info), "VIP: %s  |  %s", nameUp.c_str(), g_License.GetTierName());
                ImGui::SetCursorPosX(panelX + (panelW - ImGui::CalcTextSize(info).x) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.8f, 0.7f, 1 });
                ImGui::Text("%s", info); ImGui::PopStyleColor();
            }

            // LOADING STEPS
            float sY = panelY + 120;
            for (int i = 0; i < nSteps; i++)
            {
                float rowY = sY + i * 55;
                ImGui::SetCursorPos({ fX, rowY });
                ImVec4 nameCol = steps[i].done ? ImVec4(1.0f, 0.65f, 0.2f, 1) :
                    (i == nCur ? ImVec4(0.9f, 0.8f, 0.7f, 1) : ImVec4(0.5f, 0.45f, 0.4f, 1));
                
                ImGui::PushStyleColor(ImGuiCol_Text, nameCol);
                ImGui::Text("%s  %s", steps[i].icon, steps[i].name);
                ImGui::PopStyleColor();

                float barY = rowY + 28;
                dl->AddRectFilled({ fX, barY }, { fX + fW, barY + 4 }, IM_COL32(30, 20, 20, 200), 2);
                float fillW = fW * steps[i].prog;
                if (fillW > 0)
                    dl->AddRectFilled({ fX, barY }, { fX + fillW, barY + 4 }, IM_COL32(200, 120, 30, 255), 2);
            }

            // ANIMATE
            if (nCur < nSteps && !steps[nCur].done)
            {
                if (nCur == 2)
                {
                    flCS2CheckTimer += dt;
                    if (flCS2CheckTimer >= 0.5f) { flCS2CheckTimer = 0; bCS2Found = IsCS2Running(); }
                    if (bCS2Found)
                    {
                        steps[nCur].prog += dt / 0.5f;
                        if (steps[nCur].prog >= 1.f) { steps[nCur].prog = 1; steps[nCur].done = true; nCur++; }
                    }
                    else
                    {
                        steps[nCur].prog = (sinf(flPulse * 3.f) + 1.f) * 0.15f + 0.05f;
                        ImGui::SetCursorPos({ fX, sY + 2 * 55 + 38 });
                        ImGui::PushStyleColor(ImGuiCol_Text, { 0.7f, 0.3f, 0.3f, 1 });
                        ImGui::Text("CS2 O'yinini ishga tushiring...");
                        ImGui::PopStyleColor();
                    }
                }
                else
                {
                    steps[nCur].prog += dt / 0.8f;
                    if (steps[nCur].prog >= 1.f)
                    {
                        steps[nCur].prog = 1; steps[nCur].done = true;
                        if (nCur == 0) g_License.CheckLicense();
                        if (nCur == 1) g_License.DownloadDependencies();
                        nCur++;
                    }
                }
            }

            if (nCur >= nSteps && ePhase == EPhase::LOADING) ePhase = EPhase::READY;

            // BOSHLASH
            if (ePhase == EPhase::READY)
            {
                float btnY = panelY + panelH - 80;
                ImGui::SetCursorPos({ fX, btnY });
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, 14 });
                if (ImGui::Button("B O S H L A S H", { fW, 0 })) ePhase = EPhase::DONE;
                ImGui::PopStyleVar();

                // VK Key
                int& menuKey = CONFIG_GET(int, g_Variables.m_Gui.m_iMenuKey);
                static bool bListeningMenuKey = false;
                static float fWaitTimer = 0.f;

                ImGui::SetCursorPos({ fX, btnY + 45 });
                if (bListeningMenuKey)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.4f, 0.2f, 1 });
                    ImGui::Text("Tugmani bosing... (Esc bekor)");
                    ImGui::PopStyleColor();

                    fWaitTimer -= dt;
                    if (fWaitTimer < 4.8f)
                    {
                        for (int i = 1; i < 256; i++) {
                            if (GetAsyncKeyState(i) & 0x8000) {
                                if (i != VK_ESCAPE) menuKey = i;
                                bListeningMenuKey = false;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    char buf[64]; snprintf(buf, sizeof(buf), "Menyu tugmasi: KEY_%d (O'ZGARTIRISH)", menuKey);
                    
                    ImGui::PushStyleColor(ImGuiCol_Button, {0,0,0,0});
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.4f, 0.2f, 0.3f });
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.9f, 0.5f, 0.2f, 0.5f });
                    ImGui::PushStyleColor(ImGuiCol_Text, { 0.7f, 0.6f, 0.5f, 1 });
                    if (ImGui::Button(buf, ImVec2(fW, 0)))
                    {
                        bListeningMenuKey = true;
                        fWaitTimer = 5.0f;
                    }
                    ImGui::PopStyleColor(4);
                }
            }
        }
        else if (ePhase == EPhase::DONE)
        {
            ImGui::End(); ImGui::Render();
            float cc[4] = { 0, 0, 0, 1 };
            m_pContext->OMSetRenderTargets(1, &m_pRTV, NULL);
            m_pContext->ClearRenderTargetView(m_pRTV, cc);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            m_pSwapChain->Present(1, 0);
            break;
        }

        ImGui::End();
        ImGui::Render();
        float cc[4] = { 0, 0, 0, 1 };
        m_pContext->OMSetRenderTargets(1, &m_pRTV, NULL);
        m_pContext->ClearRenderTargetView(m_pRTV, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        m_pSwapChain->Present(1, 0);
    }
    
    if (bgTexture) bgTexture->Release();
    return true;
}

void LoginWindow::Destroy()
{
    if (!m_bInitialized) return;
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (m_pRTV) { m_pRTV->Release(); m_pRTV = nullptr; }
    if (m_pSwapChain) { m_pSwapChain->Release(); m_pSwapChain = nullptr; }
    if (m_pContext) { m_pContext->Release(); m_pContext = nullptr; }
    if (m_pDevice) { m_pDevice->Release(); m_pDevice = nullptr; }
    if (m_hWnd) { DestroyWindow(m_hWnd); m_hWnd = nullptr; }
    UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
    m_bInitialized = false;
}
