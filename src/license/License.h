#pragma once
#include "HttpClient.h"

enum class ETier : std::uint8_t
{
    LITE = 0,   // Free  - ESP + Radar + C4 Timer + AntiFlash
    MID  = 1,   // Paid  - + Bhop + Triggerbot
    PRO  = 2,   // Paid  - + Aimbot + all features
};

class CLicense
{
public:
    ETier       m_eTier      = ETier::LITE;
    std::string m_strUser    = "Guest";
    std::string m_strExpiry  = "N/A";
    std::string m_strToken;
    std::string m_strApiUrl  = "https://kaelcoder-production.up.railway.app";

    // Login via console UI and authenticate with server
    void Load();

    // Check license status from server
    bool CheckLicense();

    // Send heartbeat to server (updates last_online, total_minutes)
    void SendHeartbeat();

    // Download weapon/dependency files from server
    void DownloadDependencies();

    // Check if current tier allows a feature
    bool HasFeature(ETier eRequired) const
    {
        return static_cast<std::uint8_t>(m_eTier) >= static_cast<std::uint8_t>(eRequired);
    }

    const char* GetTierName() const
    {
        switch (m_eTier)
        {
        case ETier::PRO:  return "PRO";
        case ETier::MID:  return "MID";
        default:          return "FREE";
        }
    }

    // Tier badge color for UI
    ImVec4 GetTierColor() const
    {
        switch (m_eTier)
        {
        case ETier::PRO:  return ImVec4(1.0f, 0.85f, 0.0f, 1.0f);   // gold
        case ETier::MID:  return ImVec4(0.4f, 0.8f,  1.0f, 1.0f);   // blue
        default:          return ImVec4(0.6f, 0.6f,  0.6f, 1.0f);   // grey
        }
    }
};

inline CLicense g_License;
