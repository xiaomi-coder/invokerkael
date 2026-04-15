#pragma once

namespace LoginWindow
{
    // Create small centered DX11+ImGui login window
    bool Create();

    // Run login window loop, returns true when login complete
    bool Run();

    // Destroy login window resources
    void Destroy();

    // State
    inline bool m_bInitialized = false;

    // DX11 resources (separate from overlay)
    inline HWND m_hWnd = nullptr;
    inline WNDCLASSEXW m_wc = {};
    inline ID3D11Device* m_pDevice = nullptr;
    inline ID3D11DeviceContext* m_pContext = nullptr;
    inline IDXGISwapChain* m_pSwapChain = nullptr;
    inline ID3D11RenderTargetView* m_pRTV = nullptr;
}
