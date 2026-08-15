#include "Skins.h"

// =====================================================================
//  SKIN CHANGER  ::  o'yin xotirasiga qo'llash
//
//  Ish tartibi (FemboyChanger mantig'idan ko'chirilgan):
//    1. localPawn -> m_pWeaponServices -> m_hMyWeapons  (qurollar ro'yxati)
//    2. har bir qurol uchun  itemView = weapon + m_AttributeManager + m_Item
//    3. pichoq bo'lsa      -> m_iItemDefinitionIndex ni qayta yozamiz + modelni yangilaymiz
//    4. m_iItemIDHigh = -1 -> "biz qo'lladik" belgisi
//    5. fallback maydonlari (paint kit / wear / seed / stattrak)
//    6. attribute list (6 = paint kit, 7 = seed, 8 = wear)
//    7. RegenerateWeaponSkins ni chaqirib materialni qayta yig'ish
// =====================================================================

namespace Skins
{
    // ---------------- o'zgarmas qiymatlar ----------------
    static const char* const kRegenerateSig =
        "48 83 EC ? E8 ? ? ? ? 48 85 C0 0F 84 ? ? ? ? 48 8B 10";
    static const int      kRegeneratePatchOffset = 0x52;
    static const uint16_t kRegenerateOriginalWord = 0x0AA8;

    static const char* const kSetModelSig =
        "40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 0D ? ? ? ? 48 8D 54 24 40 48 8B 01 "
        "FF 50 60 48 8B 54 24 40 48 8B CB E8 ? ? ? ? 48 83 C4 20 5B C3";

    static const int kGetStaticDataVIndex = 16;   // C_EconItemView
    static const int kGetModelPathVIndex  = 7;    // item definition

    static const int kDefIndexKnifeCT = 42;
    static const int kDefIndexKnifeT  = 59;
    static const int kKnifeDefMin     = 500;
    static const int kKnifeDefMax     = 599;
    static const int kGloveDefMin     = 4725;     // Broken Fang Gloves
    static const int kGloveDefMax     = 5100;

    static const uint16_t kAttrPaintKit = 6;
    static const uint16_t kAttrSeed     = 7;
    static const uint16_t kAttrWear     = 8;

    static const int      kAttributeSize          = 72;    // sizeof(CEconItemAttribute)
    static const int      kAttrDefIndexOffset     = 0x30;
    static const int      kAttrValueOffset        = 0x34;
    static const int      kAttrInitialValueOffset = 0x38;
    static const int      kAllocationCountOffset  = 0x14;
    static const uint32_t kExternalConstBuffer    = 0xC0000000;

    bool IsKnifeDef(int nDefIndex)
    {
        return nDefIndex == kDefIndexKnifeCT || nDefIndex == kDefIndexKnifeT ||
               (nDefIndex >= kKnifeDefMin && nDefIndex <= kKnifeDefMax);
    }

    bool IsGloveDef(int nDefIndex)
    {
        return nDefIndex >= kGloveDefMin && nDefIndex <= kGloveDefMax;
    }

    // =================================================================
    //  Jarayon bilan ishlash (o'z handle'imiz — alloc / remote thread uchun)
    // =================================================================
    static HANDLE s_hProcess = nullptr;
    static DWORD  s_dwPid    = 0;

    static HANDLE ProcessHandle()
    {
        if (s_hProcess)
            return s_hProcess;

        // cs2.exe PID ni topamiz
        PROCESSENTRY32 pe = {};
        pe.dwSize = sizeof(pe);
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap == INVALID_HANDLE_VALUE) return nullptr;

        if (Process32First(hSnap, &pe))
        {
            do
            {
                if (_stricmp(pe.szExeFile, "cs2.exe") == 0)
                {
                    s_dwPid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &pe));
        }
        CloseHandle(hSnap);

        if (s_dwPid == 0) return nullptr;

        s_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, s_dwPid);
        return s_hProcess;
    }

    static std::uintptr_t Allocate(size_t uSize)
    {
        HANDLE h = ProcessHandle();
        if (!h) return 0;
        return reinterpret_cast<std::uintptr_t>(
            VirtualAllocEx(h, nullptr, uSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    }

    static void Release(std::uintptr_t uAddress)
    {
        HANDLE h = ProcessHandle();
        if (!h || !uAddress) return;
        VirtualFreeEx(h, reinterpret_cast<LPVOID>(uAddress), 0, MEM_RELEASE);
    }

    static bool WriteRaw(std::uintptr_t uAddress, const void* pData, size_t uSize)
    {
        HANDLE h = ProcessHandle();
        if (!h) return false;
        SIZE_T uWritten = 0;
        return WriteProcessMemory(h, reinterpret_cast<LPVOID>(uAddress), pData, uSize, &uWritten) && uWritten == uSize;
    }

    static bool WriteProtected(std::uintptr_t uAddress, const void* pData, size_t uSize)
    {
        HANDLE h = ProcessHandle();
        if (!h) return false;

        DWORD dwOld = 0;
        if (!VirtualProtectEx(h, reinterpret_cast<LPVOID>(uAddress), uSize, PAGE_EXECUTE_READWRITE, &dwOld))
            return false;

        const bool bOk = WriteRaw(uAddress, pData, uSize);
        VirtualProtectEx(h, reinterpret_cast<LPVOID>(uAddress), uSize, dwOld, &dwOld);
        return bOk;
    }

    static void CallThread(std::uintptr_t uFunction, std::uintptr_t uArg = 0)
    {
        HANDLE h = ProcessHandle();
        if (!h || !uFunction) return;

        HANDLE hThread = CreateRemoteThread(h, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(uFunction), reinterpret_cast<LPVOID>(uArg), 0, nullptr);
        if (!hThread) return;

        WaitForSingleObject(hThread, 5000);
        CloseHandle(hThread);
    }

    static bool CallStub(const std::vector<std::uint8_t>& vecCode)
    {
        if (vecCode.empty()) return false;

        const std::uintptr_t uStub = Allocate(vecCode.size());
        if (!uStub) return false;

        bool bOk = WriteRaw(uStub, vecCode.data(), vecCode.size());
        if (bOk) CallThread(uStub);

        Release(uStub);
        return bOk;
    }

    // =================================================================
    //  Offsetlar
    // =================================================================
    struct Offsets_t
    {
        std::uint32_t m_pWeaponServices = 0;
        std::uint32_t m_hMyWeapons      = 0;
        std::uint32_t m_AttributeManager = 0;
        std::uint32_t m_Item            = 0;
        std::uint32_t m_AttributeList   = 0;
        std::uint32_t m_Attributes      = 0;
        std::uint32_t m_iItemDefinitionIndex = 0;
        std::uint32_t m_iItemIDHigh     = 0;
        std::uint32_t m_iItemIDLow      = 0;
        std::uint32_t m_iAccountID      = 0;
        std::uint32_t m_iEntityQuality  = 0;
        std::uint32_t m_bInitialized    = 0;
        std::uint32_t m_nFallbackPaintKit = 0;
        std::uint32_t m_nFallbackSeed   = 0;
        std::uint32_t m_flFallbackWear  = 0;
        std::uint32_t m_nFallbackStatTrak = 0;
        std::uint32_t m_EconGloves      = 0;
        std::uint32_t m_bNeedToReApplyGloves = 0;
        std::uint32_t m_nSubclassID     = 0;
        bool          m_bResolved       = false;
    };
    static Offsets_t s_Offsets;

    static std::uint32_t SchemaOffset(const char* szField, std::uint32_t uFallback = 0)
    {
        auto it = SchemaSystem::m_mapSchemaOffsets.find(FNV1A::Hash(szField));
        const std::uint32_t uValue = (it != SchemaSystem::m_mapSchemaOffsets.end()) ? it->second : 0U;
        return (uValue != 0U) ? uValue : uFallback;
    }

    static bool ResolveOffsets()
    {
        if (s_Offsets.m_bResolved)
            return true;

        Offsets_t o;
        o.m_pWeaponServices      = SchemaOffset("C_BasePlayerPawn->m_pWeaponServices", 4616);
        o.m_hMyWeapons           = SchemaOffset("CPlayer_WeaponServices->m_hMyWeapons", 72);
        o.m_AttributeManager     = SchemaOffset("C_EconEntity->m_AttributeManager", 4520);
        o.m_Item                 = SchemaOffset("C_AttributeContainer->m_Item", 80);
        o.m_AttributeList        = SchemaOffset("C_EconItemView->m_AttributeList", 520);
        o.m_Attributes           = SchemaOffset("CAttributeList->m_Attributes", 8);
        o.m_iItemDefinitionIndex = SchemaOffset("C_EconItemView->m_iItemDefinitionIndex", 442);
        o.m_iItemIDHigh          = SchemaOffset("C_EconItemView->m_iItemIDHigh", 464);
        o.m_iItemIDLow           = SchemaOffset("C_EconItemView->m_iItemIDLow", 468);
        o.m_iAccountID           = SchemaOffset("C_EconItemView->m_iAccountID", 472);
        o.m_iEntityQuality       = SchemaOffset("C_EconItemView->m_iEntityQuality", 444);
        o.m_bInitialized         = SchemaOffset("C_EconItemView->m_bInitialized", 488);
        o.m_nFallbackPaintKit    = SchemaOffset("C_EconEntity->m_nFallbackPaintKit", 5760);
        o.m_nFallbackSeed        = SchemaOffset("C_EconEntity->m_nFallbackSeed", 5764);
        o.m_flFallbackWear       = SchemaOffset("C_EconEntity->m_flFallbackWear", 5768);
        o.m_nFallbackStatTrak    = SchemaOffset("C_EconEntity->m_nFallbackStatTrak", 5772);
        o.m_EconGloves           = SchemaOffset("C_CSPlayerPawn->m_EconGloves", 5776);
        o.m_bNeedToReApplyGloves = SchemaOffset("C_CSPlayerPawn->m_bNeedToReApplyGloves", 5773);
        o.m_nSubclassID          = SchemaOffset("C_BaseEntity->m_nSubclassID", 896);

        if (o.m_pWeaponServices == 0 || o.m_AttributeManager == 0 || o.m_iItemDefinitionIndex == 0)
        {
            g_strStatus = "offsetlar topilmadi";
            return false;
        }

        o.m_bResolved = true;
        s_Offsets = o;
        return true;
    }

    // =================================================================
    //  Attribute list
    // =================================================================
    struct UtlVectorHead_t
    {
        int            m_nSize;
        int            m_nPad;
        std::uintptr_t m_pElements;
    };

    static std::uintptr_t s_uAttributeVTable = 0;
    static std::vector<std::uintptr_t> s_vecOwnedBlocks;

    static std::uintptr_t AttributeListAddress(std::uintptr_t uItemView)
    {
        return uItemView + s_Offsets.m_AttributeList + s_Offsets.m_Attributes;
    }

    static void ObserveVTable(std::uintptr_t uItemView)
    {
        if (s_uAttributeVTable != 0) return;

        UtlVectorHead_t head = g_Memory.ReadMemory<UtlVectorHead_t>(AttributeListAddress(uItemView));
        if (head.m_nSize <= 0 || head.m_nSize > 64 || head.m_pElements == 0) return;

        const std::uintptr_t uVTable = g_Memory.ReadMemory<std::uintptr_t>(head.m_pElements);
        if (uVTable > 0x10000)
            s_uAttributeVTable = uVTable;
    }

    // Mavjud ro'yxatdagi 6/7/8 atributlarini yangilaymiz
    static bool UpdateAttributesInPlace(const UtlVectorHead_t& head, const SkinConfig_t& cfg)
    {
        if (head.m_nSize > 64) return false;

        bool bWrotePaintKit = false;
        for (int i = 0; i < head.m_nSize; i++)
        {
            const std::uintptr_t uEntry = head.m_pElements + static_cast<std::uintptr_t>(i) * kAttributeSize;
            const std::uint16_t uDefIndex = g_Memory.ReadMemory<std::uint16_t>(uEntry + kAttrDefIndexOffset);

            float flValue = 0.f;
            if      (uDefIndex == kAttrPaintKit) flValue = static_cast<float>(cfg.m_nPaintKit);
            else if (uDefIndex == kAttrSeed)     flValue = static_cast<float>(cfg.m_nSeed);
            else if (uDefIndex == kAttrWear)     flValue = cfg.m_flWear;
            else continue;

            WriteRaw(uEntry + kAttrValueOffset,        &flValue, sizeof(float));
            WriteRaw(uEntry + kAttrInitialValueOffset, &flValue, sizeof(float));

            if (uDefIndex == kAttrPaintKit) bWrotePaintKit = true;
        }
        return bWrotePaintKit;
    }

    // Bo'sh ro'yxatga yangi atributlar o'rnatamiz
    static bool CreateAttributes(std::uintptr_t uItemView, const SkinConfig_t& cfg)
    {
        const std::uintptr_t uListAddr = AttributeListAddress(uItemView);
        UtlVectorHead_t head = g_Memory.ReadMemory<UtlVectorHead_t>(uListAddr);

        // O'yinning o'z ro'yxatini hech qachon buzmaymiz
        if (head.m_nSize != 0 || head.m_pElements != 0)
            return false;

        const int nCount = 3;
        std::vector<std::uint8_t> vecBlock(static_cast<size_t>(kAttributeSize) * nCount, 0);

        const std::uint16_t uIds[3]    = { kAttrPaintKit, kAttrSeed, kAttrWear };
        const float         flVals[3]  = { static_cast<float>(cfg.m_nPaintKit),
                                           static_cast<float>(cfg.m_nSeed),
                                           cfg.m_flWear };

        for (int i = 0; i < nCount; i++)
        {
            std::uint8_t* p = vecBlock.data() + static_cast<size_t>(i) * kAttributeSize;
            *reinterpret_cast<std::uintptr_t*>(p + 0x00) = s_uAttributeVTable;   // vtable
            *reinterpret_cast<std::uintptr_t*>(p + 0x08) = 0;                    // owner
            *reinterpret_cast<std::uint16_t*>(p + kAttrDefIndexOffset)     = uIds[i];
            *reinterpret_cast<float*>(p + kAttrValueOffset)                = flVals[i];
            *reinterpret_cast<float*>(p + kAttrInitialValueOffset)         = flVals[i];
        }

        const std::uintptr_t uBlock = Allocate(vecBlock.size());
        if (!uBlock) return false;

        if (!WriteRaw(uBlock, vecBlock.data(), vecBlock.size()))
        {
            Release(uBlock);
            return false;
        }

        // Avval pointer, keyin hajm — o'yin hech qachon bo'sh massivni ko'rmasin
        WriteRaw(uListAddr + 8, &uBlock, sizeof(std::uintptr_t));
        const std::uint32_t uMarker = kExternalConstBuffer;
        WriteRaw(uListAddr + kAllocationCountOffset, &uMarker, sizeof(uMarker));
        const int nSize = nCount;
        WriteRaw(uListAddr, &nSize, sizeof(int));

        s_vecOwnedBlocks.push_back(uBlock);
        return true;
    }

    static bool ApplyAttributes(std::uintptr_t uItemView, const SkinConfig_t& cfg)
    {
        if (cfg.m_nPaintKit <= 0) return false;

        UtlVectorHead_t head = g_Memory.ReadMemory<UtlVectorHead_t>(AttributeListAddress(uItemView));
        if (head.m_nSize > 0 && head.m_pElements != 0)
            return UpdateAttributesInPlace(head, cfg);

        return CreateAttributes(uItemView, cfg);
    }

    // =================================================================
    //  Model yangilash (shellcode)
    // =================================================================
    static std::uintptr_t s_uSetModel = 0;
    static bool           s_bSetModelTried = false;

    static void PushU64(std::vector<std::uint8_t>& vec, std::uint64_t uValue)
    {
        for (int i = 0; i < 8; i++)
            vec.push_back(static_cast<std::uint8_t>((uValue >> (i * 8)) & 0xFF));
    }

    static void PushU32(std::vector<std::uint8_t>& vec, std::uint32_t uValue)
    {
        for (int i = 0; i < 4; i++)
            vec.push_back(static_cast<std::uint8_t>((uValue >> (i * 8)) & 0xFF));
    }

    static bool RefreshModel(std::uintptr_t uWeapon, std::uintptr_t uItemView)
    {
        if (!s_bSetModelTried)
        {
            s_bSetModelTried = true;
            s_uSetModel = g_Memory.PatternScan(CLIENT_DLL, kSetModelSig);
        }

        if (s_uSetModel == 0) return false;

        std::vector<std::uint8_t> code;
        code.insert(code.end(), { 0x48, 0x83, 0xEC, 0x28 });               // sub rsp, 28h
        code.insert(code.end(), { 0x48, 0xB9 }); PushU64(code, uItemView); // mov rcx, itemView
        code.insert(code.end(), { 0x48, 0x8B, 0x01 });                     // mov rax, [rcx]
        code.insert(code.end(), { 0xFF, 0x90 }); PushU32(code, kGetStaticDataVIndex * 8);
        code.insert(code.end(), { 0x48, 0x85, 0xC0 });                     // test rax, rax
        code.insert(code.end(), { 0x74, 0x27 });                           // jz done
        code.insert(code.end(), { 0x48, 0x8B, 0xC8 });                     // mov rcx, rax
        code.insert(code.end(), { 0x48, 0x8B, 0x01 });                     // mov rax, [rcx]
        code.insert(code.end(), { 0xFF, 0x50, static_cast<std::uint8_t>(kGetModelPathVIndex * 8) });
        code.insert(code.end(), { 0x48, 0x85, 0xC0 });                     // test rax, rax
        code.insert(code.end(), { 0x74, 0x19 });                           // jz done
        code.insert(code.end(), { 0x48, 0x8B, 0xD0 });                     // mov rdx, rax
        code.insert(code.end(), { 0x48, 0xB9 }); PushU64(code, uWeapon);   // mov rcx, weapon
        code.insert(code.end(), { 0x48, 0xB8 }); PushU64(code, s_uSetModel);
        code.insert(code.end(), { 0xFF, 0xD0 });                           // call rax
        code.insert(code.end(), { 0x48, 0x83, 0xC4, 0x28 });               // add rsp, 28h
        code.push_back(0xC3);                                              // ret

        return CallStub(code);
    }

    // =================================================================
    //  RegenerateWeaponSkins
    // =================================================================
    static std::uintptr_t s_uRegenerate     = 0;
    static bool           s_bRegeneratePatched = false;
    static int            s_nSigAttempts    = 0;

    static bool EnsureRegeneratePatch()
    {
        if (s_bRegeneratePatched) return true;
        if (s_nSigAttempts >= 5)  return false;

        if (s_uRegenerate == 0)
        {
            s_nSigAttempts++;
            s_uRegenerate = g_Memory.PatternScan(CLIENT_DLL, kRegenerateSig);
            if (s_uRegenerate == 0)
                return false;
        }

        const std::uintptr_t uPatchSite = s_uRegenerate + kRegeneratePatchOffset;
        const std::uint16_t  uCurrent   = g_Memory.ReadMemory<std::uint16_t>(uPatchSite);
        const std::uint16_t  uTarget    = static_cast<std::uint16_t>(
            s_Offsets.m_AttributeManager + s_Offsets.m_Item + s_Offsets.m_AttributeList + s_Offsets.m_Attributes);

        if (uCurrent == uTarget)
        {
            s_bRegeneratePatched = true;
            return true;
        }

        if (uCurrent != kRegenerateOriginalWord)
        {
            // Signatura boshqa funksiyaga tushdi — tegmaymiz
            s_uRegenerate  = 0;
            s_nSigAttempts = 5;
            return false;
        }

        if (!WriteProtected(uPatchSite, &uTarget, sizeof(uTarget)))
            return false;

        s_bRegeneratePatched = true;
        return true;
    }

    static void Regenerate()
    {
        if (s_uRegenerate == 0 || !s_bRegeneratePatched) return;
        CallThread(s_uRegenerate);
    }

    // =================================================================
    //  Weapon subclass (eksperimental)
    // =================================================================
    static std::uint32_t MakeStringToken(const std::string& strInput)
    {
        std::string str = strInput;
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);

        const std::uint32_t m = 0x5BD1E995;
        const int           r = 24;

        std::uint32_t h   = 0x31415926 ^ static_cast<std::uint32_t>(str.size());
        size_t        len = str.size();
        size_t        i   = 0;

        while (len >= 4)
        {
            std::uint32_t k = 0;
            memcpy(&k, str.data() + i, 4);
            k *= m;
            k ^= k >> r;
            k *= m;
            h *= m;
            h ^= k;
            i   += 4;
            len -= 4;
        }

        switch (len)
        {
        case 3: h ^= static_cast<std::uint32_t>(static_cast<std::uint8_t>(str[i + 2])) << 16; [[fallthrough]];
        case 2: h ^= static_cast<std::uint32_t>(static_cast<std::uint8_t>(str[i + 1])) << 8;  [[fallthrough]];
        case 1: h ^= static_cast<std::uint8_t>(str[i]); h *= m; break;
        default: break;
        }

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;
        return h;
    }

    static void EnsureSubclass(std::uintptr_t uWeapon, int nDefIndex)
    {
        if (!g_bSubclass || s_Offsets.m_nSubclassID == 0 || nDefIndex <= 0) return;

        const std::uint32_t uWant = MakeStringToken(std::to_string(nDefIndex));
        const std::uint32_t uLive = g_Memory.ReadMemory<std::uint32_t>(uWeapon + s_Offsets.m_nSubclassID);
        if (uLive == uWant) return;

        g_Memory.WriteMemory<std::uint32_t>(uWeapon + s_Offsets.m_nSubclassID, uWant);
    }

    // =================================================================
    //  Entity handle -> pointer
    // =================================================================
    static std::uintptr_t EntityFromHandle(int nHandle)
    {
        if (nHandle == 0 || nHandle == -1) return 0;

        const int nIndex = nHandle & 0x7FFF;

        const std::uintptr_t uEntitySystem = g_Memory.ReadMemory<std::uintptr_t>(g_Globals.m_uEntityList);
        if (!uEntitySystem) return 0;

        const std::uintptr_t uChunk = g_Memory.ReadMemory<std::uintptr_t>(uEntitySystem + 0x10 + 0x8 * (nIndex >> 9));
        if (!uChunk) return 0;

        const std::uintptr_t uIdentity = uChunk + 0x70 * (nIndex & 0x1FF);
        const int nStored = g_Memory.ReadMemory<int>(uIdentity + 0x10);
        if (nStored != nHandle) return 0;

        return g_Memory.ReadMemory<std::uintptr_t>(uIdentity);
    }

    // =================================================================
    //  Qurollarga qo'llash
    // =================================================================
    static bool ApplyWeapons(std::uintptr_t uPawn, bool bForce)
    {
        const std::uintptr_t uWeaponServices = g_Memory.ReadMemory<std::uintptr_t>(uPawn + s_Offsets.m_pWeaponServices);
        if (uWeaponServices < 0x10000) return false;

        const int            nCount = g_Memory.ReadMemory<int>(uWeaponServices + s_Offsets.m_hMyWeapons);
        const std::uintptr_t uArray = g_Memory.ReadMemory<std::uintptr_t>(uWeaponServices + s_Offsets.m_hMyWeapons + 8);
        if (nCount <= 0 || nCount > 64 || uArray < 0x10000) return false;

        bool bChanged = false;

        for (int i = 0; i < nCount; i++)
        {
            const int nHandle = g_Memory.ReadMemory<int>(uArray + static_cast<std::uintptr_t>(i) * 4);
            const std::uintptr_t uWeapon = EntityFromHandle(nHandle);
            if (uWeapon < 0x10000) continue;

            const std::uintptr_t uItemView = uWeapon + s_Offsets.m_AttributeManager + s_Offsets.m_Item;
            const std::uint16_t  uDefIndex = g_Memory.ReadMemory<std::uint16_t>(uItemView + s_Offsets.m_iItemDefinitionIndex);
            if (uDefIndex == 0) continue;

            ObserveVTable(uItemView);

            // Qaysi sozlama ushbu qurolga tegishli?
            SkinConfig_t cfg;
            {
                std::lock_guard<std::mutex> lock(g_mtxConfig);
                if (IsKnifeDef(uDefIndex))
                {
                    if (g_nKnifeDefIndex == 0) continue;
                    cfg = g_KnifeConfig;
                }
                else
                {
                    auto it = g_mapWeapons.find(uDefIndex);
                    if (it == g_mapWeapons.end()) continue;
                    cfg = it->second;
                }
            }

            if (cfg.m_nPaintKit <= 0 && cfg.m_nDefIndex == 0)
                continue;

            // --- pichoq: item definition ni qayta yozamiz ---
            if (cfg.m_nDefIndex != 0)
                EnsureSubclass(uWeapon, cfg.m_nDefIndex);

            if (cfg.m_nDefIndex != 0 && cfg.m_nDefIndex != static_cast<int>(uDefIndex))
            {
                const std::uint16_t uNewDef = static_cast<std::uint16_t>(cfg.m_nDefIndex);
                g_Memory.WriteMemory<std::uint16_t>(uItemView + s_Offsets.m_iItemDefinitionIndex, uNewDef);

                if (s_Offsets.m_iEntityQuality != 0)
                    g_Memory.WriteMemory<int>(uItemView + s_Offsets.m_iEntityQuality, 3);
                if (s_Offsets.m_iAccountID != 0)
                    g_Memory.WriteMemory<int>(uItemView + s_Offsets.m_iAccountID, static_cast<int>(0x1337BEEF));

                RefreshModel(uWeapon, uItemView);
                bChanged = true;
            }

            // --- "allaqachon qo'llangan" belgisini tozalash ---
            if (bForce)
                g_Memory.WriteMemory<int>(uItemView + s_Offsets.m_iItemIDHigh, 0);

            const int nItemIdHigh = g_Memory.ReadMemory<int>(uItemView + s_Offsets.m_iItemIDHigh);
            if (nItemIdHigh == -1)
                continue;   // shu hayotda allaqachon qo'llangan

            g_Memory.WriteMemory<int>(uItemView + s_Offsets.m_iItemIDHigh, -1);

            if (cfg.m_nPaintKit > 0)
            {
                g_Memory.WriteMemory<int>  (uWeapon + s_Offsets.m_nFallbackPaintKit, cfg.m_nPaintKit);
                g_Memory.WriteMemory<float>(uWeapon + s_Offsets.m_flFallbackWear,    cfg.m_flWear);
                g_Memory.WriteMemory<int>  (uWeapon + s_Offsets.m_nFallbackSeed,     cfg.m_nSeed);
                g_Memory.WriteMemory<int>  (uWeapon + s_Offsets.m_nFallbackStatTrak, cfg.m_nStatTrak);

                ApplyAttributes(uItemView, cfg);
            }

            bChanged = true;
        }

        return bChanged;
    }

    // =================================================================
    //  Qo'lqoplar (eksperimental — o'yin har doim ham ko'rsatavermaydi)
    // =================================================================
    static bool ApplyGloves(std::uintptr_t uPawn, bool bForce)
    {
        SkinConfig_t cfg;
        int nDefIndex = 0;
        {
            std::lock_guard<std::mutex> lock(g_mtxConfig);
            nDefIndex = g_nGloveDefIndex;
            cfg       = g_GloveConfig;
        }

        if (nDefIndex == 0) return false;
        if (s_Offsets.m_EconGloves == 0 || s_Offsets.m_bNeedToReApplyGloves == 0) return false;

        const std::uintptr_t uGloveItemView = uPawn + s_Offsets.m_EconGloves;
        const std::uint16_t  uCurrentDef    = g_Memory.ReadMemory<std::uint16_t>(uGloveItemView + s_Offsets.m_iItemDefinitionIndex);

        if (uCurrentDef == static_cast<std::uint16_t>(nDefIndex) && !bForce)
            return false;

        g_Memory.WriteMemory<std::uint8_t>(uGloveItemView + s_Offsets.m_bInitialized, 0);
        g_Memory.WriteMemory<std::uint16_t>(uGloveItemView + s_Offsets.m_iItemDefinitionIndex, static_cast<std::uint16_t>(nDefIndex));
        g_Memory.WriteMemory<int>(uGloveItemView + s_Offsets.m_iItemIDHigh, -1);
        if (s_Offsets.m_iItemIDLow != 0)     g_Memory.WriteMemory<int>(uGloveItemView + s_Offsets.m_iItemIDLow, -1);
        if (s_Offsets.m_iEntityQuality != 0) g_Memory.WriteMemory<int>(uGloveItemView + s_Offsets.m_iEntityQuality, 3);
        if (s_Offsets.m_iAccountID != 0)     g_Memory.WriteMemory<int>(uGloveItemView + s_Offsets.m_iAccountID, static_cast<int>(0x1337BEEF));

        ApplyAttributes(uGloveItemView, cfg);

        g_Memory.WriteMemory<std::uint8_t>(uGloveItemView + s_Offsets.m_bInitialized, 1);
        g_Memory.WriteMemory<std::uint8_t>(uPawn + s_Offsets.m_bNeedToReApplyGloves, 1);
        return true;
    }

    // =================================================================
    //  Asosiy tick
    // =================================================================
    void Run()
    {
        if (!g_bEnabled)
            return;

        if (!ResolveOffsets())
            return;

        C_CSPlayerPawn* pLocalPawn = g_Globals.m_LocalPlayer.m_pPlayerPawn;
        if (!pLocalPawn || reinterpret_cast<std::uintptr_t>(pLocalPawn) < 0x10000)
        {
            g_strStatus = "o'yinda emassiz (yoki tomoshabin)";
            return;
        }

        const bool bHasAnything = [&]
        {
            std::lock_guard<std::mutex> lock(g_mtxConfig);
            return !g_mapWeapons.empty() || g_nKnifeDefIndex != 0 || g_nGloveDefIndex != 0;
        }();

        if (!bHasAnything)
        {
            g_strStatus = "skin tanlanmagan";
            return;
        }

        if (!ProcessHandle())
        {
            g_strStatus = "cs2.exe ga ulanib bo'lmadi";
            return;
        }

        EnsureRegeneratePatch();

        const bool bForce = g_bForce.exchange(false);
        const std::uintptr_t uPawn = reinterpret_cast<std::uintptr_t>(pLocalPawn);

        bool bChanged = false;
        try
        {
            bChanged  = ApplyWeapons(uPawn, bForce);
            bChanged |= ApplyGloves(uPawn, bForce);
        }
        catch (...) { }

        if (bChanged || bForce)
            Regenerate();

        g_strStatus = s_bRegeneratePatched ? "ishlayapti" : "ishlayapti (regenerate yo'q)";
    }

    void ClearAll()
    {
        std::lock_guard<std::mutex> lock(g_mtxConfig);
        g_mapWeapons.clear();
        g_KnifeConfig    = SkinConfig_t();
        g_nKnifeDefIndex = 0;
        g_GloveConfig    = SkinConfig_t();
        g_nGloveDefIndex = 0;
        g_bForce = true;
    }

    void ForceReapply()
    {
        g_bForce = true;
    }
}
