#include "../Includes.h"
#include <json.hpp>
using json = nlohmann::json;

// -----------------------------------------------------------------------
// Console-based login (fallback if GUI login fails)
// -----------------------------------------------------------------------
void CLicense::Load()
{
    m_eTier   = ETier::LITE;
    m_strUser = "Guest";

    std::cout << std::endl;
    std::cout << "  ==============================" << std::endl;
    std::cout << "  SHIFTHUB.UZ - Login" << std::endl;
    std::cout << "  ==============================" << std::endl;

    for (int attempt = 0; attempt < 5; attempt++)
    {
        std::string username, password;

        std::cout << std::endl;
        std::cout << "  Username: ";
        std::getline(std::cin, username);
        std::cout << "  Password: ";

        char ch;
        while ((ch = _getch()) != '\r')
        {
            if (ch == '\b') { if (!password.empty()) { password.pop_back(); std::cout << "\b \b"; } }
            else { password += ch; std::cout << '*'; }
        }
        std::cout << std::endl;

        while (!username.empty() && (username.back() == ' ' || username.back() == '\n' || username.back() == '\r'))
            username.pop_back();

        if (username.empty() || password.empty())
        {
            std::cout << "  [!] Username va password kiriting!" << std::endl;
            continue;
        }

        json jBody;
        jBody["username"] = username;
        jBody["password"] = password;

        std::cout << "  Ulanmoqda..." << std::endl;
        Http::Response resp = Http::Post(m_strApiUrl + "/api/auth/login", jBody.dump());

        if (!resp.success || resp.body.empty())
        {
            std::cout << "  [X] Login xato!" << std::endl;
            continue;
        }

        try
        {
            json jResp = json::parse(resp.body);
            m_strToken = jResp.value("token", "");
            m_strUser  = jResp["user"].value("username", username);

            std::string strTier = jResp["user"].value("tier", "free");
            if (strTier == "pro")       m_eTier = ETier::PRO;
            else if (strTier == "mid")  m_eTier = ETier::MID;
            else                        m_eTier = ETier::LITE;

            m_strExpiry = jResp["user"].value("expires_at", "N/A");

            std::cout << "  [+] Login muvaffaqiyatli! (" << GetTierName() << ")" << std::endl;
            DownloadDependencies();
            CheckLicense();
            return;
        }
        catch (...) { std::cout << "  [X] Server javobi xato!" << std::endl; }
    }

    std::cout << "  [X] 5 ta urinish tugadi!" << std::endl;
    Sleep(3000);
    exit(1);
}

// -----------------------------------------------------------------------
bool CLicense::CheckLicense()
{
    if (m_strToken.empty()) return false;

    Http::Response resp = Http::Get(m_strApiUrl + "/api/license/check", m_strToken);
    if (!resp.success) return false;

    try
    {
        json jResp = json::parse(resp.body);
        if (!jResp.value("valid", false)) { m_eTier = ETier::LITE; return false; }

        std::string strTier = jResp.value("tier", "free");
        if (strTier == "pro")       m_eTier = ETier::PRO;
        else if (strTier == "mid")  m_eTier = ETier::MID;
        else                        m_eTier = ETier::LITE;

        return true;
    }
    catch (...) { return false; }
}

// -----------------------------------------------------------------------
void CLicense::SendHeartbeat()
{
    if (m_strToken.empty()) return;
    Http::Post(m_strApiUrl + "/api/license/heartbeat", "{}", m_strToken);
}

// -----------------------------------------------------------------------
void CLicense::DownloadDependencies()
{
    if (m_strToken.empty()) return;

    Http::Response resp = Http::Get(m_strApiUrl + "/api/files/list", m_strToken);
    if (!resp.success || resp.body.empty()) return;

    try
    {
        json jResp = json::parse(resp.body);
        if (!jResp.contains("files")) return;

        char szExePath[MAX_PATH] = {};
        GetModuleFileNameA(nullptr, szExePath, MAX_PATH);
        std::filesystem::path weaponsDir = std::filesystem::path(szExePath).parent_path() / "weapons";
        std::filesystem::create_directories(weaponsDir);

        for (auto& jFile : jResp["files"])
        {
            std::string filename = jFile.value("name", "");
            if (filename.empty()) continue;

            std::filesystem::path filePath = weaponsDir / filename;
            if (std::filesystem::exists(filePath)) continue;

            Http::Response fileResp = Http::Get(
                m_strApiUrl + "/api/files/weapons/" + filename, m_strToken);

            if (fileResp.success && !fileResp.body.empty())
            {
                std::ofstream ofs(filePath, std::ios::binary);
                ofs.write(fileResp.body.data(), fileResp.body.size());
                ofs.close();
            }
        }
    }
    catch (...) {}
}
