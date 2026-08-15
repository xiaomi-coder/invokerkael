#include "Skins.h"

// =====================================================================
//  Skin bazasi: skins.json ni yuklab olish, tahlil qilish, suratlarni
//  keshlash va DX11 teksturalariga aylantirish.
// =====================================================================

using json = nlohmann::json;

namespace Skins
{
    static const char* const kSkinsUrl = "https://ob.tonyha7.com/skins.json";

    // ---------------- ichki holat ----------------
    struct PendingImage_t
    {
        SkinEntry_t*             m_pSkin = nullptr;
        std::vector<std::uint8_t> m_vecBytes;
    };

    static std::mutex                 s_mtxQueue;
    static std::deque<SkinEntry_t*>   s_dqRequests;    // yuklab olish navbati
    static std::mutex                 s_mtxPending;
    static std::deque<PendingImage_t> s_dqPending;     // tekstura kutayotganlar
    static std::atomic<bool>          s_bStop{ false };
    static std::vector<std::thread>   s_vecWorkers;
    static std::thread                s_threadLoader;
    static std::atomic<bool>          s_bStarted{ false };

    // ---------------------------------------------------------------
    //  Kesh papkasi:  %TEMP%\kael_skins\
    // ---------------------------------------------------------------
    static std::filesystem::path CacheDir()
    {
        std::error_code ec;
        std::filesystem::path dir = std::filesystem::temp_directory_path(ec) / "kael_skins";
        std::filesystem::create_directories(dir, ec);
        return dir;
    }

    static std::string SafeFileName(const std::string& strId)
    {
        std::string strOut;
        strOut.reserve(strId.size());
        for (char c : strId)
            strOut += (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') ? c : '_';
        if (strOut.size() > 64) strOut.resize(64);
        return strOut;
    }

    static bool ReadFileBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& vecOut)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        file.seekg(0, std::ios::end);
        const std::streamoff nSize = file.tellg();
        if (nSize <= 0 || nSize > 8 * 1024 * 1024) return false;
        file.seekg(0, std::ios::beg);
        vecOut.resize(static_cast<size_t>(nSize));
        file.read(reinterpret_cast<char*>(vecOut.data()), nSize);
        return file.good() || file.eof();
    }

    static void WriteFileBytes(const std::filesystem::path& path, const std::string& strData)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) return;
        file.write(strData.data(), static_cast<std::streamsize>(strData.size()));
    }

    // ---------------------------------------------------------------
    //  Rarity rangini "#eb4b4b" ko'rinishidan o'qish
    // ---------------------------------------------------------------
    static ImU32 ParseHexColor(const std::string& strHex)
    {
        if (strHex.size() < 7 || strHex[0] != '#')
            return IM_COL32(138, 138, 154, 255);

        auto Hex = [](char c) -> int
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };

        const int r = Hex(strHex[1]) * 16 + Hex(strHex[2]);
        const int g = Hex(strHex[3]) * 16 + Hex(strHex[4]);
        const int b = Hex(strHex[5]) * 16 + Hex(strHex[6]);
        return IM_COL32(r, g, b, 255);
    }

    static int KindFromCategory(const std::string& strCategory)
    {
        if (strCategory == "sfui_invpanel_filter_gloves") return KIND_GLOVE;
        if (strCategory == "sfui_invpanel_filter_melee")  return KIND_KNIFE;
        return KIND_WEAPON;
    }

    // JSON qiymati matn yoki son bo'lishi mumkin
    static int JsonToInt(const json& j, int nDefault = 0)
    {
        try
        {
            if (j.is_number_integer())  return j.get<int>();
            if (j.is_number_float())    return static_cast<int>(j.get<float>());
            if (j.is_string())          return std::stoi(j.get<std::string>());
        }
        catch (...) { }
        return nDefault;
    }

    static float JsonToFloat(const json& j, float flDefault = 0.f)
    {
        try
        {
            if (j.is_number())  return j.get<float>();
            if (j.is_string())  return std::stof(j.get<std::string>());
        }
        catch (...) { }
        return flDefault;
    }

    // ---------------------------------------------------------------
    //  Bazani tahlil qilish
    // ---------------------------------------------------------------
    static bool ParseSkins(const std::string& strJson)
    {
        json arr;
        try { arr = json::parse(strJson); }
        catch (...) { return false; }

        if (!arr.is_array() || arr.empty())
            return false;

        std::deque<SkinEntry_t>   dqSkins;
        std::map<int, WeaponGroup_t> mapWeapons;

        for (const auto& item : arr)
        {
            try
            {
                if (!item.contains("paint_index") || item["paint_index"].is_null())
                    continue;

                const int nPaint = JsonToInt(item["paint_index"], -1);
                if (nPaint < 0) continue;

                int         nWeaponId = 0;
                std::string strWeapon;
                if (item.contains("weapon") && item["weapon"].is_object())
                {
                    nWeaponId = JsonToInt(item["weapon"].value("weapon_id", json(0)), 0);
                    strWeapon = item["weapon"].value("name", std::string());
                }
                if (nWeaponId == 0) continue;

                dqSkins.emplace_back();
                SkinEntry_t& skin = dqSkins.back();

                skin.m_strId      = item.value("id", std::string());
                skin.m_strName    = item.value("name", std::string());
                skin.m_strWeapon  = strWeapon;
                skin.m_strImageUrl= item.value("image", std::string());
                skin.m_nWeaponId  = nWeaponId;
                skin.m_nPaintKit  = nPaint;
                skin.m_bLegacy    = item.value("legacy_model", false);

                if (item.contains("category") && item["category"].is_object())
                    skin.m_eKind = KindFromCategory(item["category"].value("id", std::string()));

                if (item.contains("rarity") && item["rarity"].is_object())
                {
                    skin.m_strRarity = item["rarity"].value("name", std::string());
                    skin.m_colRarity = ParseHexColor(item["rarity"].value("color", std::string("#8A8A9A")));
                }

                if (item.contains("min_float")) skin.m_flMinFloat = JsonToFloat(item["min_float"], 0.f);
                if (item.contains("max_float")) skin.m_flMaxFloat = JsonToFloat(item["max_float"], 1.f);
                if (skin.m_flMaxFloat <= skin.m_flMinFloat) skin.m_flMaxFloat = 1.f;

                // "AK-47 | Redline"  ->  "Redline"
                const size_t uBar = skin.m_strName.find('|');
                skin.m_strShort = (uBar != std::string::npos && uBar + 2 < skin.m_strName.size())
                    ? skin.m_strName.substr(uBar + 2)
                    : skin.m_strName;

                WeaponGroup_t& group = mapWeapons[nWeaponId];
                group.m_nDefIndex = nWeaponId;
                group.m_eKind     = skin.m_eKind;
                if (group.m_strName.empty()) group.m_strName = strWeapon;
                group.m_vecSkins.push_back(&skin);
            }
            catch (...) { /* bitta skin xato bo'lsa — o'tkazib yuboramiz */ }
        }

        if (dqSkins.empty())
            return false;

        std::vector<WeaponGroup_t> vecWeapons;
        vecWeapons.reserve(mapWeapons.size());
        for (auto& entry : mapWeapons)
            vecWeapons.push_back(std::move(entry.second));

        // pichoq va qo'lqoplar oxirida emas — alifbo bo'yicha, lekin turi bo'yicha ajratilgan
        std::sort(vecWeapons.begin(), vecWeapons.end(), [](const WeaponGroup_t& a, const WeaponGroup_t& b)
        {
            if (a.m_eKind != b.m_eKind) return a.m_eKind < b.m_eKind;
            return a.m_strName < b.m_strName;
        });

        for (WeaponGroup_t& group : vecWeapons)
        {
            std::sort(group.m_vecSkins.begin(), group.m_vecSkins.end(),
                [](const SkinEntry_t* a, const SkinEntry_t* b) { return a->m_strShort < b->m_strShort; });
        }

        g_dqSkins    = std::move(dqSkins);
        g_vecWeapons = std::move(vecWeapons);
        return true;
    }

    // ---------------------------------------------------------------
    //  Yuklovchi oqim
    // ---------------------------------------------------------------
    static void LoaderThread()
    {
        const std::filesystem::path cachePath = CacheDir() / "skins.json";

        // 1) diskdagi kesh (tez ishga tushish uchun)
        std::string strJson;
        {
            std::vector<std::uint8_t> vecBytes;
            if (ReadFileBytes(cachePath, vecBytes) && vecBytes.size() > 1024)
            {
                strJson.assign(reinterpret_cast<const char*>(vecBytes.data()), vecBytes.size());
                g_strDbStatus = "keshdan o'qilmoqda...";
                if (ParseSkins(strJson))
                {
                    g_bReady = true;
                    g_strDbStatus = "kesh yuklandi";
                }
            }
        }

        // 2) internetdan yangilash
        if (!s_bStop)
        {
            if (!g_bReady) g_strDbStatus = "skinlar yuklanmoqda...";

            Http::Response resp = Http::Get(kSkinsUrl);
            if (resp.success && resp.body.size() > 1024)
            {
                if (g_bReady)
                {
                    // Ro'yxat allaqachon keshdan qurilgan va UI uni o'qiyapti —
                    // uni almashtirsak pointerlar buziladi. Faqat diskka saqlaymiz,
                    // yangi ma'lumot keyingi ishga tushirishda ishlatiladi.
                    WriteFileBytes(cachePath, resp.body);
                }
                else if (ParseSkins(resp.body))
                {
                    WriteFileBytes(cachePath, resp.body);
                    g_bReady = true;
                    g_strDbStatus = "yangilandi";
                }
                else
                {
                    g_strDbStatus = "javobni o'qib bo'lmadi";
                }
            }
            else if (!g_bReady)
            {
                g_strDbStatus = "internetga ulanib bo'lmadi";
            }
        }

        if (g_bReady)
        {
            size_t nSkins = g_dqSkins.size();
            char szBuf[128];
            snprintf(szBuf, sizeof(szBuf), "%zu ta skin  ·  %zu ta qurol", nSkins, g_vecWeapons.size());
            g_strDbStatus = szBuf;
        }

        g_bLoading = false;
    }

    // ---------------------------------------------------------------
    //  Surat yuklovchi oqimlar
    // ---------------------------------------------------------------
    static void ImageThread()
    {
        const std::filesystem::path dir = CacheDir();

        while (!s_bStop)
        {
            SkinEntry_t* pSkin = nullptr;
            {
                std::lock_guard<std::mutex> lock(s_mtxQueue);
                if (!s_dqRequests.empty())
                {
                    pSkin = s_dqRequests.front();
                    s_dqRequests.pop_front();
                }
            }

            if (!pSkin)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                continue;
            }

            if (pSkin->m_strImageUrl.empty())
            {
                pSkin->m_nImgState = IMG_FAILED;
                continue;
            }

            std::vector<std::uint8_t> vecBytes;
            const std::filesystem::path file = dir / (SafeFileName(pSkin->m_strId) + ".png");

            // 1) diskdagi kesh
            if (!ReadFileBytes(file, vecBytes) || vecBytes.size() < 64)
            {
                // 2) internet
                Http::Response resp = Http::Get(pSkin->m_strImageUrl);
                if (!resp.success || resp.body.size() < 64)
                {
                    pSkin->m_nImgState = IMG_FAILED;
                    continue;
                }

                WriteFileBytes(file, resp.body);
                vecBytes.assign(resp.body.begin(), resp.body.end());
            }

            {
                std::lock_guard<std::mutex> lock(s_mtxPending);
                s_dqPending.push_back(PendingImage_t{ pSkin, std::move(vecBytes) });
            }
            pSkin->m_nImgState = IMG_DECODE;
        }
    }

    // ---------------------------------------------------------------
    //  API
    // ---------------------------------------------------------------
    void Initialize()
    {
        if (s_bStarted.exchange(true))
            return;

        s_bStop   = false;
        g_bLoading = true;

        s_threadLoader = std::thread(&LoaderThread);
        s_threadLoader.detach();

        for (int i = 0; i < 3; i++)
        {
            std::thread t(&ImageThread);
            t.detach();
        }
    }

    void Shutdown()
    {
        s_bStop = true;
    }

    void RequestImage(SkinEntry_t* pSkin)
    {
        if (!pSkin || pSkin->m_nImgState != IMG_NONE)
            return;

        pSkin->m_nImgState = IMG_QUEUED;

        std::lock_guard<std::mutex> lock(s_mtxQueue);
        if (s_dqRequests.size() > 512)      // navbat cheksiz o'smasin
        {
            SkinEntry_t* pDropped = s_dqRequests.front();
            s_dqRequests.pop_front();
            if (pDropped) pDropped->m_nImgState = IMG_NONE;   // keyinroq qayta so'raladi
        }
        s_dqRequests.push_back(pSkin);
    }

    static ImTextureID CreateTextureFromMemory(const std::uint8_t* pBytes, int nSize, int& iW, int& iH)
    {
        if (!Window::m_pDevice) return nullptr;

        int iChannels = 0;
        unsigned char* pData = stbi_load_from_memory(pBytes, nSize, &iW, &iH, &iChannels, 4);
        if (!pData) return nullptr;

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
        sub.pSysMem     = pData;
        sub.SysMemPitch = iW * 4;

        ID3D11Texture2D* pTex = nullptr;
        HRESULT hr = Window::m_pDevice->CreateTexture2D(&desc, &sub, &pTex);
        stbi_image_free(pData);

        if (FAILED(hr) || !pTex) return nullptr;

        D3D11_SHADER_RESOURCE_VIEW_DESC srv = {};
        srv.Format              = desc.Format;
        srv.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        srv.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* pSRV = nullptr;
        hr = Window::m_pDevice->CreateShaderResourceView(pTex, &srv, &pSRV);
        pTex->Release();

        if (FAILED(hr) || !pSRV) return nullptr;
        return reinterpret_cast<ImTextureID>(pSRV);
    }

    void PumpTextures()
    {
        // kadr uchun bir nechta — freeze bo'lmasligi uchun
        for (int i = 0; i < 4; i++)
        {
            PendingImage_t pending;
            {
                std::lock_guard<std::mutex> lock(s_mtxPending);
                if (s_dqPending.empty()) return;
                pending = std::move(s_dqPending.front());
                s_dqPending.pop_front();
            }

            if (!pending.m_pSkin) continue;

            int iW = 0, iH = 0;
            ImTextureID pTexture = CreateTextureFromMemory(pending.m_vecBytes.data(),
                static_cast<int>(pending.m_vecBytes.size()), iW, iH);

            if (pTexture)
            {
                pending.m_pSkin->m_pTexture = pTexture;
                pending.m_pSkin->m_nTexW    = iW;
                pending.m_pSkin->m_nTexH    = iH;
                pending.m_pSkin->m_nImgState = IMG_READY;
            }
            else
            {
                pending.m_pSkin->m_nImgState = IMG_FAILED;
            }
        }
    }
}
