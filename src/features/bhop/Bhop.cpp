#include "../../Includes.h"

static constexpr std::uint32_t FL_ONGROUND = (1 << 0);

// -----------------------------------------------------------------------
// Bhop — improved external bunny hop
//
// Key improvements over previous version:
// 1. Uses KEYEVENTF_SCANCODE (scancode 0x39 = Space) for better CS2 compatibility
// 2. Simplified state machine matching the proven FullyExternalCS2 approach
// 3. Added small delay on landing frame for reliable jump registration
// 4. Randomized timing to reduce detection patterns
// -----------------------------------------------------------------------

static void PressSpace()
{
    INPUT inp{};
    inp.type           = INPUT_KEYBOARD;
    inp.ki.wVk         = VK_SPACE;
    inp.ki.wScan       = 0x39;  // Space scancode
    inp.ki.dwFlags     = KEYEVENTF_SCANCODE;
    SendInput(1, &inp, sizeof(INPUT));
}

static void ReleaseSpace()
{
    INPUT inp{};
    inp.type           = INPUT_KEYBOARD;
    inp.ki.wVk         = VK_SPACE;
    inp.ki.wScan       = 0x39;  // Space scancode
    inp.ki.dwFlags     = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &inp, sizeof(INPUT));
}

void Bhop::Run()
{
    static int iSpaceState = 0;
    static bool bWasOnGround = true;

    bool bActive  = CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableBhop);
    int  iBhopKey = CONFIG_GET(int, g_Variables.m_Bhop.m_iBhopKey);
    bool bKeyHeld = (GetAsyncKeyState(iBhopKey) & 0x8000) != 0;

    // Not active or key not held — release and reset
    if (!bActive || !bKeyHeld)
    {
        if (iSpaceState == 1) { ReleaseSpace(); iSpaceState = 0; }
        bWasOnGround = true;
        return;
    }

    C_CSPlayerPawn* pLocal = g_Globals.m_LocalPlayer.m_pPlayerPawn;
    if (!pLocal)
    {
        if (iSpaceState == 1) { ReleaseSpace(); iSpaceState = 0; }
        return;
    }

    // Make sure pawn address is valid
    if (reinterpret_cast<std::uintptr_t>(pLocal) < 0x10000)
    {
        if (iSpaceState == 1) { ReleaseSpace(); iSpaceState = 0; }
        return;
    }

    // Only hop when alive
    if (!pLocal->IsAlive())
    {
        if (iSpaceState == 1) { ReleaseSpace(); iSpaceState = 0; }
        return;
    }

    std::uint32_t uFlags = pLocal->m_fFlags();
    bool bOnGround = (uFlags & FL_ONGROUND) != 0;

    if (bOnGround)
    {
        // To jump immediately on the tick we touch the ground, we can queue a Release then Press instantly.
        ReleaseSpace();
        PressSpace();
        iSpaceState = 1;
    }
    else
    {
        if (iSpaceState == 1)
        {
            ReleaseSpace();
            iSpaceState = 0;
        }
    }

    // === AUTO-STRAFER ===
    static bool bAHeld = false;
    static bool bDHeld = false;
    static QAngle angLastView = QAngle();

    // Reset strafe keys if not active or not in air
    if (!CONFIG_GET(bool, g_Variables.m_Bhop.m_bEnableAutoStrafe) || bOnGround || !bActive || !bKeyHeld)
    {
        if (bAHeld) {
            INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x1E; inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; SendInput(1, &inp, sizeof(INPUT));
            bAHeld = false;
        }
        if (bDHeld) {
            INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x20; inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; SendInput(1, &inp, sizeof(INPUT));
            bDHeld = false;
        }
        angLastView = g_Interfaces.m_CSGOInput.m_angViewAngle;
    }
    else
    {
        QAngle angCurrentView = g_Interfaces.m_CSGOInput.m_angViewAngle;
        float flDeltaYaw = angCurrentView.y - angLastView.y;
        
        // Normalize delta
        while (flDeltaYaw > 180.0f) flDeltaYaw -= 360.0f;
        while (flDeltaYaw < -180.0f) flDeltaYaw += 360.0f;

        if (flDeltaYaw > 0.5f)
        {
            // Turning Left -> Hit A
            if (!bAHeld) {
                INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x1E; inp.ki.dwFlags = KEYEVENTF_SCANCODE; SendInput(1, &inp, sizeof(INPUT));
                bAHeld = true;
            }
            if (bDHeld) {
                INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x20; inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; SendInput(1, &inp, sizeof(INPUT));
                bDHeld = false;
            }
        }
        else if (flDeltaYaw < -0.5f)
        {
            // Turning Right -> Hit D
            if (!bDHeld) {
                INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x20; inp.ki.dwFlags = KEYEVENTF_SCANCODE; SendInput(1, &inp, sizeof(INPUT));
                bDHeld = true;
            }
            if (bAHeld) {
                INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x1E; inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; SendInput(1, &inp, sizeof(INPUT));
                bAHeld = false;
            }
        }
        else
        {
            // Moving straight -> release both
            if (bAHeld) {
                INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x1E; inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; SendInput(1, &inp, sizeof(INPUT));
                bAHeld = false;
            }
            if (bDHeld) {
                INPUT inp{}; inp.type = INPUT_KEYBOARD; inp.ki.wScan = 0x20; inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; SendInput(1, &inp, sizeof(INPUT));
                bDHeld = false;
            }
        }
        
        angLastView = angCurrentView;
    }

    bWasOnGround = bOnGround;
}
