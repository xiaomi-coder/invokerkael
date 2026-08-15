#pragma once
#include "HttpClient.h"
#include <json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>

// =====================================================================
// SHIFTHUB Auto-Updater
// VPS da 2 ta fayl kerak:
//   1. version.json  — {"version":"2.1","download_url":"http://IP/shifthub/shifthub.exe","changelog":"..."}
//   2. shifthub.exe  — yangi exe fayl
//
// Yangi versiya chiqarganda faqat shu 2 ta faylni almashtiring.
// =====================================================================

class CUpdater
{
public:
    // Update holati
    bool        m_bChecked          = false;    // tekshirilganmi
    bool        m_bUpdateAvailable  = false;    // yangi versiya bormi
    bool        m_bDownloading      = false;    // yuklab olish jarayoni
    bool        m_bDownloadComplete = false;    // yuklab olish tugadimi
    bool        m_bDownloadFailed   = false;    // xatolik bo'ldimi
    float       m_flProgress        = 0.f;      // 0.0 - 1.0 progress
    std::string m_strLatestVersion;             // yangi versiya raqami
    std::string m_strChangelog;                 // o'zgarishlar
    std::string m_strDownloadUrl;               // exe yuklab olish URL
    std::string m_strStatusText;                // holat matni

    // ---------------------------------------------------------------
    // VPS dan version.json ni tekshirish
    // ---------------------------------------------------------------
    void CheckForUpdate()
    {
        if (m_bChecked) return;
        m_bChecked = true;
        m_strStatusText = "Tekshirilmoqda...";

        try
        {
            Http::Response resp = Http::Get(SHIFTHUB_UPDATE_URL);
            if (!resp.success || resp.body.empty())
            {
                m_strStatusText = "Server bilan bog'lanib bo'lmadi";
                return;
            }

            nlohmann::json jVer = nlohmann::json::parse(resp.body);
            m_strLatestVersion = jVer.value("version", "");
            m_strDownloadUrl   = jVer.value("download_url", "");
            m_strChangelog     = jVer.value("changelog", "");

            if (m_strLatestVersion.empty())
            {
                m_strStatusText = "Server javobi noto'g'ri";
                return;
            }

            // Versiya solishtiruvi — oddiy string compare
            // "2.0" < "2.1" bo'lsa update bor
            if (CompareVersions(SHIFTHUB_VERSION, m_strLatestVersion) < 0)
            {
                m_bUpdateAvailable = true;
                m_strStatusText = "Yangi versiya mavjud!";
            }
            else
            {
                m_strStatusText = "Oxirgi versiya o'rnatilgan";
            }
        }
        catch (const std::exception& ex)
        {
            m_strStatusText = "Xatolik: ";
            m_strStatusText += ex.what();
        }
        catch (...)
        {
            m_strStatusText = "Noma'lum xatolik";
        }
    }

    // ---------------------------------------------------------------
    // Qayta tekshirish (tugma bosilganda)
    // ---------------------------------------------------------------
    void Recheck()
    {
        m_bChecked = false;
        m_bUpdateAvailable = false;
        m_bDownloading = false;
        m_bDownloadComplete = false;
        m_bDownloadFailed = false;
        m_flProgress = 0.f;
        m_strStatusText = "";
        CheckForUpdate();
    }

    // ---------------------------------------------------------------
    // Yangi exe ni yuklab olish (alohida thread da)
    // ---------------------------------------------------------------
    void StartDownload()
    {
        if (m_bDownloading || m_strDownloadUrl.empty()) return;
        m_bDownloading = true;
        m_bDownloadFailed = false;
        m_bDownloadComplete = false;
        m_flProgress = 0.f;
        m_strStatusText = "Yuklab olinmoqda...";

        std::thread([this]()
        {
            try
            {
                // Exe ni yuklab olish
                m_flProgress = 0.1f;
                Http::Response resp = Http::Get(m_strDownloadUrl);
                m_flProgress = 0.8f;

                if (!resp.success || resp.body.empty())
                {
                    m_bDownloadFailed = true;
                    m_bDownloading = false;
                    m_strStatusText = "Yuklab olish xato!";
                    return;
                }

                // %TEMP% ga saqlash
                std::filesystem::path tempDir = std::filesystem::temp_directory_path();
                std::filesystem::path tempExe = tempDir / "shifthub_update.exe";

                std::ofstream ofs(tempExe, std::ios::binary);
                if (!ofs.is_open())
                {
                    m_bDownloadFailed = true;
                    m_bDownloading = false;
                    m_strStatusText = "Faylni saqlash xato!";
                    return;
                }

                ofs.write(resp.body.data(), resp.body.size());
                ofs.close();
                m_flProgress = 1.0f;

                m_bDownloadComplete = true;
                m_bDownloading = false;
                m_strStatusText = "Tayyor! Yangilash uchun bosing.";
            }
            catch (...)
            {
                m_bDownloadFailed = true;
                m_bDownloading = false;
                m_strStatusText = "Yuklab olishda xatolik!";
            }
        }).detach();
    }

    // ---------------------------------------------------------------
    // Yangi exe ni o'rnatish — batch script orqali
    // Eski exe o'chiriladi, yangi exe ko'chiriladi, qayta ishga tushadi
    // ---------------------------------------------------------------
    void ApplyUpdate()
    {
        if (!m_bDownloadComplete) return;

        try
        {
            // Joriy exe yo'li
            char szCurrentPath[MAX_PATH] = {};
            GetModuleFileNameA(NULL, szCurrentPath, MAX_PATH);
            std::string strCurrentExe = szCurrentPath;

            // Temp dagi yangi exe
            std::filesystem::path tempExe = std::filesystem::temp_directory_path() / "shifthub_update.exe";

            // Batch script yaratish
            std::filesystem::path batPath = std::filesystem::temp_directory_path() / "shifthub_update.bat";

            std::ofstream bat(batPath);
            bat << "@echo off\r\n";
            bat << "echo KAEL_CHEAT yangilanmoqda...\r\n";
            bat << "timeout /t 2 /nobreak >nul\r\n";                          // 2 soniya kutish (exe yopilishi uchun)
            bat << "del /f /q \"" << strCurrentExe << "\"\r\n";               // eski exe o'chirish
            bat << "copy /y \"" << tempExe.string() << "\" \"" << strCurrentExe << "\"\r\n"; // yangi exe ko'chirish
            bat << "del /f /q \"" << tempExe.string() << "\"\r\n";            // temp faylni tozalash
            bat << "start \"\" \"" << strCurrentExe << "\"\r\n";              // yangi exe ishga tushirish
            bat << "del /f /q \"" << batPath.string() << "\"\r\n";            // batch script o'zini o'chirish
            bat.close();

            // Batch scriptni ishga tushirish (yashirin oyna)
            STARTUPINFOA si = {};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;

            PROCESS_INFORMATION pi = {};
            std::string strCmd = "cmd.exe /c \"" + batPath.string() + "\"";

            if (CreateProcessA(NULL, strCmd.data(), NULL, NULL, FALSE,
                CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }

            // Joriy dasturni yopish
            ExitProcess(0);
        }
        catch (...)
        {
            m_strStatusText = "Yangilash xato! Qo'lda yangilang.";
        }
    }

private:
    // ---------------------------------------------------------------
    // Versiyalarni solishtirish: "2.0" vs "2.1"
    // Qaytaradi: -1 (a < b), 0 (a == b), 1 (a > b)
    // ---------------------------------------------------------------
    static int CompareVersions(const std::string& a, const std::string& b)
    {
        auto Split = [](const std::string& s) -> std::vector<int>
        {
            std::vector<int> parts;
            std::string token;
            for (char c : s)
            {
                if (c == '.')
                {
                    parts.push_back(token.empty() ? 0 : std::stoi(token));
                    token.clear();
                }
                else if (c >= '0' && c <= '9')
                    token += c;
            }
            parts.push_back(token.empty() ? 0 : std::stoi(token));
            return parts;
        };

        auto va = Split(a);
        auto vb = Split(b);
        size_t len = (std::max)(va.size(), vb.size());

        for (size_t i = 0; i < len; i++)
        {
            int na = (i < va.size()) ? va[i] : 0;
            int nb = (i < vb.size()) ? vb[i] : 0;
            if (na < nb) return -1;
            if (na > nb) return  1;
        }
        return 0;
    }
};

inline CUpdater g_Updater;
