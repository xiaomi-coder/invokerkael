#define STB_IMAGE_IMPLEMENTATION
#include "../../Includes.h"

// STB image is already included via Includes.h (stb_image.h)

struct WeaponTexture_t
{
    ImTextureID pTexture = nullptr;
    int iWidth = 0;
    int iHeight = 0;
};

static std::unordered_map<std::string, WeaponTexture_t> s_mapIcons;

// -----------------------------------------------------------------------
// Helper: load a PNG file into a DX11 ShaderResourceView
// -----------------------------------------------------------------------
static bool LoadTextureFromFile(const std::string& szPath, WeaponTexture_t& out)
{
    // Load image using stb_image
    int iWidth = 0, iHeight = 0, iChannels = 0;
    unsigned char* pData = stbi_load(szPath.c_str(), &iWidth, &iHeight, &iChannels, 4); // force RGBA
    if (!pData)
        return false;

    // Create DX11 texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = iWidth;
    desc.Height = iHeight;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = pData;
    subResource.SysMemPitch = iWidth * 4;

    ID3D11Texture2D* pTexture2D = nullptr;
    HRESULT hr = Window::m_pDevice->CreateTexture2D(&desc, &subResource, &pTexture2D);
    stbi_image_free(pData);

    if (FAILED(hr) || !pTexture2D)
        return false;

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView* pSRV = nullptr;
    hr = Window::m_pDevice->CreateShaderResourceView(pTexture2D, &srvDesc, &pSRV);
    pTexture2D->Release();

    if (FAILED(hr) || !pSRV)
        return false;

    out.pTexture = reinterpret_cast<ImTextureID>(pSRV);
    out.iWidth = iWidth;
    out.iHeight = iHeight;
    return true;
}

// -----------------------------------------------------------------------
// Get the path to the weapons/ folder next to the executable
// -----------------------------------------------------------------------
static std::string GetWeaponsFolder()
{
    char szPath[MAX_PATH] = {};
    GetModuleFileNameA(NULL, szPath, MAX_PATH);

    // Remove the executable name to get the directory
    std::string strPath(szPath);
    size_t uLastSlash = strPath.find_last_of("\\/");
    if (uLastSlash != std::string::npos)
        strPath = strPath.substr(0, uLastSlash + 1);

    return strPath + "weapons\\";
}

// -----------------------------------------------------------------------
// Initialize: load all weapon PNGs from the weapons/ folder
// -----------------------------------------------------------------------
void WeaponIcons::Initialize()
{
    if (!Window::m_pDevice)
        return;

    std::string strFolder = GetWeaponsFolder();

    // Check if weapons folder exists
    if (!std::filesystem::exists(strFolder))
    {
#ifdef _DEBUG
        std::cout << X("[WeaponIcons] weapons/ folder not found: ") << strFolder << std::endl;
#endif
        return;
    }

    int iLoaded = 0;
    for (const auto& entry : std::filesystem::directory_iterator(strFolder))
    {
        if (!entry.is_regular_file())
            continue;

        std::string strExt = entry.path().extension().string();
        std::transform(strExt.begin(), strExt.end(), strExt.begin(), ::tolower);

        if (strExt != ".png")
            continue;

        std::string strName = entry.path().stem().string();
        std::string strFullPath = entry.path().string();

        WeaponTexture_t tex;
        if (LoadTextureFromFile(strFullPath, tex))
        {
            s_mapIcons[strName] = tex;
            iLoaded++;
#ifdef _DEBUG
            std::cout << X("[WeaponIcons] Loaded: ") << strName << X(" (") << tex.iWidth << X("x") << tex.iHeight << X(")") << std::endl;
#endif
        }
    }

#ifdef _DEBUG
    std::cout << X("[WeaponIcons] Total loaded: ") << iLoaded << X(" icons") << std::endl;
#endif
}

// -----------------------------------------------------------------------
// Shutdown: release all DX11 textures
// -----------------------------------------------------------------------
void WeaponIcons::Shutdown()
{
    for (auto& [name, tex] : s_mapIcons)
    {
        if (tex.pTexture)
        {
            reinterpret_cast<ID3D11ShaderResourceView*>(tex.pTexture)->Release();
            tex.pTexture = nullptr;
        }
    }
    s_mapIcons.clear();
}

// -----------------------------------------------------------------------
// GetIcon: return DX11 texture for a given weapon name
// -----------------------------------------------------------------------
ImTextureID WeaponIcons::GetIcon(const std::string& szWeaponName)
{
    auto it = s_mapIcons.find(szWeaponName);
    if (it != s_mapIcons.end())
        return it->second.pTexture;

    // Try common name mappings (CS2 uses some aliases)
    // e.g. "weapon_ak47" -> "ak47", "weapon_usp_silencer" -> "usp_silencer"
    if (szWeaponName.substr(0, 7) == "weapon_")
    {
        std::string strShort = szWeaponName.substr(7);
        auto it2 = s_mapIcons.find(strShort);
        if (it2 != s_mapIcons.end())
            return it2->second.pTexture;
    }

    return nullptr;
}

// -----------------------------------------------------------------------
// HasIcon: check if we have a texture for this weapon
// -----------------------------------------------------------------------
bool WeaponIcons::HasIcon(const std::string& szWeaponName)
{
    return GetIcon(szWeaponName) != nullptr;
}

// -----------------------------------------------------------------------
// GetIconSize: retrieve original dimensions for proper aspect ratio
// -----------------------------------------------------------------------
bool WeaponIcons::GetIconSize(const std::string& szWeaponName, int& iWidth, int& iHeight)
{
    auto it = s_mapIcons.find(szWeaponName);
    if (it == s_mapIcons.end())
    {
        if (szWeaponName.substr(0, 7) == "weapon_")
        {
            it = s_mapIcons.find(szWeaponName.substr(7));
        }
    }

    if (it != s_mapIcons.end())
    {
        iWidth = it->second.iWidth;
        iHeight = it->second.iHeight;
        return true;
    }

    return false;
}
