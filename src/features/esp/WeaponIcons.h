#pragma once

namespace WeaponIcons
{
    // Initialize: load all weapon PNGs from "weapons/" folder into DX11 textures.
    // Must be called after Window::Create() (needs Window::m_pDevice)
    void Initialize();

    // Release all DX11 textures. Call before Window::Destroy()
    void Shutdown();

    // Get DX11 texture for a weapon name (e.g. "ak47", "usp_silencer").
    // Returns nullptr if icon not found.
    ImTextureID GetIcon(const std::string& szWeaponName);

    // Check if an icon exists for the given weapon name
    bool HasIcon(const std::string& szWeaponName);

    // Get the original image dimensions for proper aspect ratio
    bool GetIconSize(const std::string& szWeaponName, int& iWidth, int& iHeight);
}
