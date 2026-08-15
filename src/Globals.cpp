#include "Includes.h"

bool CInterfaces::Update()
{
	m_GlobalVars = g_Memory.ReadMemory<CGlobalVars>(g_Memory.ReadMemory<std::uintptr_t>(g_Globals.m_Offsets.m_uGlobalVars));
	m_CSGOInput = g_Memory.ReadMemory<CCSGOInput>(g_Memory.ReadMemory<std::uintptr_t>(g_Globals.m_Offsets.m_uCSGOInput));
	m_NetworkGameClient = g_Memory.ReadMemory<CNetWorkGameClient>(g_Memory.ReadMemory<std::uintptr_t>(g_Globals.m_Offsets.m_uNetworkGameClient));
	m_GameEntitySystem = g_Memory.ReadMemory<CGameEntitySystem>(g_Memory.ReadMemory<std::uintptr_t>(g_Globals.m_Offsets.m_uEntitySystem));
	return true;
}

bool CGlobals::Update()
{
	static std::once_flag flag;
	std::call_once(flag, []()
	{
		// Default latest fallback offsets (a2x/cs2-dumper live — build 6354324)
		std::uintptr_t uEntityList = 39141456;
		std::uintptr_t uViewMatrix = 37414224;
		std::uintptr_t uLocalPlayerController = 37240240;
		std::uintptr_t uPlantedC4 = 37173400;
		std::uintptr_t uGlobalVars = 34164024;
		std::uintptr_t uCSGOInput = 37481248;
		std::uintptr_t uNetworkGameClient = 9491600;
		std::uintptr_t uEntitySystem = 39141456;
		std::uintptr_t uSensitivity = 37381160;

		// Only use hardcoded fallbacks, disable local json and dynamic download to make it strictly built-in offsets.
		bool bLoadedLocal = false;

		g_Globals.m_Offsets.m_uEntityList = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uEntityList;
		g_Globals.m_Offsets.m_uViewMatrix = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uViewMatrix;
		g_Globals.m_Offsets.m_uLocalPlayerController = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uLocalPlayerController;
		g_Globals.m_Offsets.m_uPlantedC4 = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uPlantedC4;
		g_Globals.m_Offsets.m_uAutoAcceptArray = g_Memory.PatternScan(CLIENT_DLL, X("48 89 05 ? ? ? ? E8 ? ? ? ? 48 85 DB"), EPatternScanFlags::SCAN_RESOLVE_RIP, 0x3, 0x7);
		
		g_Globals.m_Offsets.m_uGlobalVars = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uGlobalVars;
		g_Globals.m_Offsets.m_uCSGOInput = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uCSGOInput;
		g_Globals.m_Offsets.m_uNetworkGameClient = g_Memory.GetModule(ENGINE2_DLL).m_uBaseAddress + uNetworkGameClient;
		g_Globals.m_Offsets.m_uEntitySystem = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uEntitySystem;
		g_Globals.m_Offsets.m_uSensitivity = g_Memory.GetModule(CLIENT_DLL).m_uBaseAddress + uSensitivity;
	});

	g_Globals.m_uEntityList = g_Globals.m_Offsets.m_uEntityList;
	g_Globals.m_LocalPlayer.m_pController = g_Memory.ReadMemory<CCSPlayerController*>(g_Globals.m_Offsets.m_uLocalPlayerController);
	if (g_Globals.m_LocalPlayer.m_pController)
		g_Globals.m_LocalPlayer.m_pPlayerPawn = reinterpret_cast<C_CSPlayerPawn*>(g_Globals.m_LocalPlayer.m_pController->m_hPlayerPawn().Get());
	else
		g_Globals.m_LocalPlayer.m_pPlayerPawn = nullptr;
	g_Globals.m_matViewMatrix = g_Memory.ReadMemory<ViewMatrix_t>(g_Globals.m_Offsets.m_uViewMatrix);

	return true;
}