#include "Injector.h"

#include <TlHelp32.h>
#include <filesystem>
#include <cstring>
#include <winternl.h>
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")

// -----------------------------------------------------------------------
// Manual PE mapping-based DLL injection.
// Loads DLL directly into target process, bypassing LoadLibrary hooks.
// -----------------------------------------------------------------------

#ifndef TH32CS_SNAPMODULE64
#define TH32CS_SNAPMODULE64 0x00000010
#endif

#ifndef IMAGE_REL_BASED_ADDR64
#define IMAGE_REL_BASED_ADDR64 10
#endif

namespace
{
    DWORD FindProcessID(const char* szProcessName)
    {
        const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32 entry{};
        entry.dwSize = sizeof(PROCESSENTRY32);

        DWORD dwPID = 0;
        if (Process32First(hSnapshot, &entry))
        {
            do
            {
                if (_stricmp(szProcessName, entry.szExeFile) == 0)
                {
                    dwPID = entry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &entry));
        }

        CloseHandle(hSnapshot);
        return dwPID;
    }

    ULONGLONG FindKernel32Base(HANDLE hProcess)
    {
        ULONGLONG uBase = 0;
        printf("[FindK32] Using EnumProcessModules\n"); fflush(stdout);

        HMODULE hMods[1024]{};
        DWORD cbNeeded = 0;

        if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
        {
            printf("[FindK32] EnumProcessModules failed: %lu\n", GetLastError()); fflush(stdout);
            return 0;
        }

        DWORD cModules = cbNeeded / sizeof(HMODULE);
        printf("[FindK32] Found %lu modules\n", cModules); fflush(stdout);

        for (DWORD i = 0; i < cModules; ++i)
        {
            char szModName[MAX_PATH]{};
            if (!GetModuleFileNameExA(hProcess, hMods[i], szModName, sizeof(szModName)))
                continue;

            char szLower[MAX_PATH]{};
            strcpy_s(szLower, sizeof(szLower), szModName);
            for (char* p = szLower; *p; ++p)
                *p = tolower(*p);

            if (i < 20 || strstr(szLower, "kernel32") != nullptr)
                printf("[FindK32] Module %lu: %s\n", i, szModName);
            fflush(stdout);

            if (strstr(szLower, "kernel32") != nullptr)
            {
                uBase = reinterpret_cast<ULONGLONG>(hMods[i]);
                printf("[FindK32] *** Found kernel32 at 0x%llx ***\n", uBase); fflush(stdout);
                break;
            }
        }

        return uBase;
    }

    typedef HMODULE(WINAPI* LoadLibraryA_t)(LPCSTR);
    typedef FARPROC(WINAPI* GetProcAddress_t)(HMODULE, LPCSTR);
    typedef int (WINAPI* DllMain_t)(HINSTANCE, DWORD, LPVOID);

    ULONGLONG GetRemoteKernel32Export(HANDLE hProcess, const char* szExportName)
    {
        printf("[GetExport] Looking for: %s\n", szExportName); fflush(stdout);

        ULONGLONG uK32Base = FindKernel32Base(hProcess);
        printf("[GetExport] Remote K32 base: 0x%llx\n", uK32Base); fflush(stdout);
        if (uK32Base == 0)
        {
            printf("[GetExport] K32 base is zero!\n"); fflush(stdout);
            return 0;
        }

        HMODULE hLocalK32 = GetModuleHandleA("kernel32.dll");
        printf("[GetExport] Local K32 handle: 0x%llx\n", (ULONGLONG)hLocalK32); fflush(stdout);
        if (!hLocalK32)
        {
            printf("[GetExport] GetModuleHandleA failed\n"); fflush(stdout);
            return 0;
        }

        FARPROC pLocal = GetProcAddress(hLocalK32, szExportName);
        printf("[GetExport] Local proc address: 0x%llx\n", (ULONGLONG)pLocal); fflush(stdout);
        if (!pLocal)
        {
            printf("[GetExport] GetProcAddress failed\n"); fflush(stdout);
            return 0;
        }

        ULONGLONG uLocalOffset = reinterpret_cast<ULONGLONG>(pLocal) - reinterpret_cast<ULONGLONG>(hLocalK32);
        ULONGLONG uResult = uK32Base + uLocalOffset;
        printf("[GetExport] Result: 0x%llx\n", uResult); fflush(stdout);
        return uResult;
    }

    struct ManualMapCtx
    {
        BYTE* pFileData;
        SIZE_T uFileSize;
        HANDLE hProcess;
        ULONGLONG uRemoteBase;
        ULONGLONG uLocalBase;
        IMAGE_DOS_HEADER* pDos;
        IMAGE_NT_HEADERS64* pNt;
    };

    DWORD RvaToFileOffset(ManualMapCtx& ctx, DWORD dwRva)
    {
        IMAGE_SECTION_HEADER* pSection = IMAGE_FIRST_SECTION(ctx.pNt);
        for (DWORD i = 0; i < ctx.pNt->FileHeader.NumberOfSections; ++i, ++pSection)
        {
            if (dwRva >= pSection->VirtualAddress &&
                dwRva < pSection->VirtualAddress + pSection->Misc.VirtualSize)
            {
                return dwRva - pSection->VirtualAddress + pSection->PointerToRawData;
            }
        }
        return dwRva;
    }

    bool LoadFileToMemory(const char* szPath, BYTE*& pOut, SIZE_T& uOut)
    {
        HANDLE hFile = CreateFileA(szPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
            return false;

        DWORD dwSize = GetFileSize(hFile, nullptr);
        if (dwSize == 0 || dwSize == INVALID_FILE_SIZE)
        {
            CloseHandle(hFile);
            return false;
        }

        BYTE* pBuffer = new BYTE[dwSize];
        DWORD dwRead = 0;
        bool bOk = !!ReadFile(hFile, pBuffer, dwSize, &dwRead, nullptr);
        CloseHandle(hFile);

        if (!bOk || dwRead != dwSize)
        {
            delete[] pBuffer;
            return false;
        }

        pOut = pBuffer;
        uOut = dwSize;
        return true;
    }

    bool ValidatePeHeaders(ManualMapCtx& ctx, std::string& strError)
    {
        if (ctx.uFileSize < sizeof(IMAGE_DOS_HEADER))
        {
            strError = "DLL juda kichik (fsize check).";
            return false;
        }

        ctx.pDos = reinterpret_cast<IMAGE_DOS_HEADER*>(ctx.pFileData);
        if (ctx.pDos->e_magic != IMAGE_DOS_SIGNATURE)
        {
            strError = "DOS signature xato (0x5A4D kutilgan).";
            return false;
        }

        if (ctx.pDos->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > ctx.uFileSize)
        {
            strError = "PE header offset tashqari.";
            return false;
        }

        ctx.pNt = reinterpret_cast<IMAGE_NT_HEADERS64*>(ctx.pFileData + ctx.pDos->e_lfanew);
        if (ctx.pNt->Signature != IMAGE_NT_SIGNATURE)
        {
            strError = "NT signature xato (0x4550 kutilgan).";
            return false;
        }

        if (ctx.pNt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64)
        {
            strError = "Machine type x64 emas.";
            return false;
        }

        return true;
    }

    bool AllocateImageMemory(ManualMapCtx& ctx, std::string& strError)
    {
        SIZE_T uAllocSize = ctx.pNt->OptionalHeader.SizeOfImage;

        ctx.uRemoteBase = reinterpret_cast<ULONGLONG>(
            VirtualAllocEx(ctx.hProcess, nullptr, uAllocSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));

        if (ctx.uRemoteBase == 0)
        {
            strError = "CS2 ichida joy ajratib bo'lmadi.";
            return false;
        }

        ctx.uLocalBase = reinterpret_cast<ULONGLONG>(ctx.pFileData);
        return true;
    }

    bool CopySections(ManualMapCtx& ctx, std::string& strError)
    {
        IMAGE_SECTION_HEADER* pSecHeader = IMAGE_FIRST_SECTION(ctx.pNt);

        for (DWORD i = 0; i < ctx.pNt->FileHeader.NumberOfSections; ++i, ++pSecHeader)
        {
            if (pSecHeader->SizeOfRawData == 0)
                continue;

            ULONGLONG uRemoteAddr = ctx.uRemoteBase + pSecHeader->VirtualAddress;
            BYTE* pSectionData = ctx.pFileData + pSecHeader->PointerToRawData;
            SIZE_T uCopySize = pSecHeader->SizeOfRawData;

            if (!WriteProcessMemory(ctx.hProcess, reinterpret_cast<LPVOID>(uRemoteAddr), pSectionData, uCopySize, nullptr))
            {
                strError = "Seksiyani yozib bo'lmadi.";
                return false;
            }
        }

        return true;
    }

    bool ApplyBaseRelocations(ManualMapCtx& ctx, std::string& strError)
    {
        IMAGE_DATA_DIRECTORY& relocDir = ctx.pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size == 0)
            return true;

        ULONGLONG uDelta = ctx.uRemoteBase - ctx.pNt->OptionalHeader.ImageBase;

        // Relocation directory may extend beyond file size (padded in memory)
        if (relocDir.VirtualAddress >= ctx.uFileSize)
        {
            // No relocation data in file
            return true;
        }

        BYTE* pRelocData = ctx.pFileData + relocDir.VirtualAddress;
        DWORD uAvailableSize = (DWORD)(ctx.uFileSize - relocDir.VirtualAddress);
        DWORD uRelocSize = relocDir.Size;
        DWORD uProcessed = 0;

        while (uProcessed < uRelocSize && uProcessed < uAvailableSize)
        {
            if (uProcessed + sizeof(IMAGE_BASE_RELOCATION) > uAvailableSize)
                break;

            IMAGE_BASE_RELOCATION* pRelocBlock = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pRelocData + uProcessed);
            if (pRelocBlock->SizeOfBlock == 0 || pRelocBlock->SizeOfBlock > uAvailableSize - uProcessed)
                break;

            DWORD uNumRelocs = (pRelocBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            WORD* pRelocs = reinterpret_cast<WORD*>(pRelocBlock + 1);

            for (DWORD i = 0; i < uNumRelocs; ++i)
            {
                WORD wReloc = pRelocs[i];
                DWORD uType = wReloc >> 12;
                DWORD uOffset = wReloc & 0xFFF;

                if (uType == IMAGE_REL_BASED_ADDR64)
                {
                    ULONGLONG uAddr = ctx.uRemoteBase + pRelocBlock->VirtualAddress + uOffset;
                    ULONGLONG uValue = 0;

                    if (!ReadProcessMemory(ctx.hProcess, reinterpret_cast<LPVOID>(uAddr), &uValue, 8, nullptr))
                    {
                        // Skip read errors — offset may be in a section with no data
                        continue;
                    }

                    uValue += uDelta;

                    if (!WriteProcessMemory(ctx.hProcess, reinterpret_cast<LPVOID>(uAddr), &uValue, 8, nullptr))
                    {
                        // Skip write errors
                        continue;
                    }
                }
                else if (uType != IMAGE_REL_BASED_ABSOLUTE)
                {
                    // x64 mostly uses ADDR64 or ABSOLUTE; skip others
                }
            }

            uProcessed += pRelocBlock->SizeOfBlock;
        }

        return true;
    }

    bool FixImports(ManualMapCtx& ctx, std::string& strError)
    {
        printf("[FixImports] Starting\n"); fflush(stdout);

        IMAGE_DATA_DIRECTORY& importDir = ctx.pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (importDir.Size == 0)
        {
            printf("[FixImports] No imports\n"); fflush(stdout);
            return true;
        }

        printf("[FixImports] Import dir RVA: 0x%lx, size %lu\n", importDir.VirtualAddress, importDir.Size); fflush(stdout);

        DWORD dwFileOffset = RvaToFileOffset(ctx, importDir.VirtualAddress);
        printf("[FixImports] Import dir file offset: 0x%lx\n", dwFileOffset); fflush(stdout);

        if (dwFileOffset >= ctx.uFileSize)
        {
            printf("[FixImports] Import dir outside file\n"); fflush(stdout);
            return true;
        }

        IMAGE_IMPORT_DESCRIPTOR* pImportDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(
            ctx.pFileData + dwFileOffset);

        printf("[FixImports] Getting exports...\n"); fflush(stdout);
        LoadLibraryA_t pLoadLibrary = reinterpret_cast<LoadLibraryA_t>(
            GetRemoteKernel32Export(ctx.hProcess, "LoadLibraryA"));

        GetProcAddress_t pGetProcAddress = reinterpret_cast<GetProcAddress_t>(
            GetRemoteKernel32Export(ctx.hProcess, "GetProcAddress"));

        printf("[FixImports] LoadLibraryA: 0x%llx, GetProcAddress: 0x%llx\n",
               (ULONGLONG)pLoadLibrary, (ULONGLONG)pGetProcAddress); fflush(stdout);

        if (!pLoadLibrary || !pGetProcAddress)
        {
            strError = "Kernel32 exportlari topilmadi.";
            printf("[FixImports] Exports not found!\n"); fflush(stdout);
            return false;
        }

        printf("[FixImports] Processing import descriptors...\n"); fflush(stdout);
        int iDllCount = 0;
        while (pImportDesc->Name != 0 && iDllCount < 100)
        {
            printf("[FixImports] Processing DLL %d at RVA 0x%lx\n", iDllCount, pImportDesc->Name); fflush(stdout);

            DWORD dwNameOffset = RvaToFileOffset(ctx, pImportDesc->Name);
            if (dwNameOffset >= ctx.uFileSize)
            {
                printf("[FixImports] DLL name offset outside file\n"); fflush(stdout);
                break;
            }

            const char* szDllName = reinterpret_cast<const char*>(ctx.pFileData + dwNameOffset);
            printf("[FixImports] DLL: %s\n", szDllName); fflush(stdout);

            HMODULE hDll = LoadLibraryA(szDllName);
            if (!hDll)
            {
                strError = std::string("DLL yuklash shart: ") + szDllName;
                printf("[FixImports] LoadLibraryA failed for %s\n", szDllName); fflush(stdout);
                return false;
            }
            printf("[FixImports] Loaded %s at 0x%llx\n", szDllName, (ULONGLONG)hDll); fflush(stdout);

            DWORD dwThunkOffset = RvaToFileOffset(ctx, pImportDesc->FirstThunk);
            if (dwThunkOffset >= ctx.uFileSize)
            {
                printf("[FixImports] FirstThunk offset outside file\n"); fflush(stdout);
                ++pImportDesc;
                ++iDllCount;
                continue;
            }

            IMAGE_THUNK_DATA64* pThunk = reinterpret_cast<IMAGE_THUNK_DATA64*>(
                ctx.pFileData + dwThunkOffset);

            DWORD iThunk = 0;
            while (pThunk->u1.AddressOfData != 0 && iThunk < 1000)
            {
                FARPROC pFunc = nullptr;

                if (IMAGE_SNAP_BY_ORDINAL64(pThunk->u1.Ordinal))
                {
                    pFunc = GetProcAddress(hDll, reinterpret_cast<const char*>(IMAGE_ORDINAL64(pThunk->u1.Ordinal)));
                }
                else
                {
                    DWORD dwNameDataOffset = RvaToFileOffset(ctx, (DWORD)pThunk->u1.AddressOfData);
                    if (dwNameDataOffset >= ctx.uFileSize)
                    {
                        printf("[FixImports] Import by name offset outside file\n"); fflush(stdout);
                        ++pThunk;
                        ++iThunk;
                        continue;
                    }

                    IMAGE_IMPORT_BY_NAME* pName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
                        ctx.pFileData + dwNameDataOffset);
                    pFunc = GetProcAddress(hDll, pName->Name);
                }

                if (!pFunc)
                {
                    printf("[FixImports] Could not resolve export\n"); fflush(stdout);
                    ++pThunk;
                    ++iThunk;
                    continue;
                }

                ULONGLONG uRemoteIat = ctx.uRemoteBase + pImportDesc->FirstThunk + (iThunk * sizeof(IMAGE_THUNK_DATA64));
                ULONGLONG uFuncAddr = reinterpret_cast<ULONGLONG>(pFunc);

                if (!WriteProcessMemory(ctx.hProcess, reinterpret_cast<LPVOID>(uRemoteIat), &uFuncAddr, 8, nullptr))
                {
                    printf("[FixImports] WriteProcessMemory failed for IAT\n"); fflush(stdout);
                    ++pThunk;
                    ++iThunk;
                    continue;
                }

                ++pThunk;
                ++iThunk;
            }

            ++pImportDesc;
            ++iDllCount;
        }

        printf("[FixImports] Done! Processed %d DLLs\n", iDllCount); fflush(stdout);

        return true;
    }

    bool CallDllMain(ManualMapCtx& ctx, std::string& strError)
    {
        printf("[CallDllMain] Entry point: 0x%llx\n", ctx.pNt->OptionalHeader.AddressOfEntryPoint); fflush(stdout);
        printf("[CallDllMain] Remote base: 0x%llx\n", ctx.uRemoteBase); fflush(stdout);

        ULONGLONG uEntryPoint = ctx.uRemoteBase + ctx.pNt->OptionalHeader.AddressOfEntryPoint;
        printf("[CallDllMain] Calculated entry: 0x%llx\n", uEntryPoint); fflush(stdout);

        BYTE simpleShell[48] = {
            0x48, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 0:  mov rcx, <hDll>
            0x48, 0xba, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 10: mov rdx, 1
            0x4d, 0x31, 0xc0,                                             // 20: xor r8, r8
            0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 23: mov rax, <DllMain>
            0xff, 0xd0,                                                   // 33: call rax
            0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00   // 35: ret + padding
        };

        *reinterpret_cast<ULONGLONG*>(&simpleShell[2]) = ctx.uRemoteBase;
        *reinterpret_cast<ULONGLONG*>(&simpleShell[25]) = uEntryPoint;

        printf("[CallDllMain] Allocating shellcode...\n"); fflush(stdout);
        LPVOID pShellcode = VirtualAllocEx(ctx.hProcess, nullptr, sizeof(simpleShell), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!pShellcode)
        {
            strError = "Shellcode uchun joy topilmadi.";
            printf("[CallDllMain] VirtualAllocEx failed: %lu\n", GetLastError()); fflush(stdout);
            return false;
        }
        printf("[CallDllMain] Shellcode allocated at 0x%llx\n", (ULONGLONG)pShellcode); fflush(stdout);

        printf("[CallDllMain] Writing shellcode...\n"); fflush(stdout);
        if (!WriteProcessMemory(ctx.hProcess, pShellcode, simpleShell, sizeof(simpleShell), nullptr))
        {
            VirtualFreeEx(ctx.hProcess, pShellcode, 0, MEM_RELEASE);
            strError = "Shellcode yozib bo'lmadi.";
            printf("[CallDllMain] WriteProcessMemory failed: %lu\n", GetLastError()); fflush(stdout);
            return false;
        }

        printf("[CallDllMain] Creating remote thread...\n"); fflush(stdout);
        HANDLE hThread = CreateRemoteThread(ctx.hProcess, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(pShellcode), nullptr, 0, nullptr);
        if (!hThread)
        {
            VirtualFreeEx(ctx.hProcess, pShellcode, 0, MEM_RELEASE);
            strError = "Shellcode threadi yaratib bo'lmadi.";
            printf("[CallDllMain] CreateRemoteThread failed: %lu\n", GetLastError()); fflush(stdout);
            return false;
        }
        printf("[CallDllMain] Remote thread created: 0x%llx\n", (ULONGLONG)hThread); fflush(stdout);

        printf("[CallDllMain] Waiting for thread...\n"); fflush(stdout);
        DWORD dwWaitResult = WaitForSingleObject(hThread, 5000);
        printf("[CallDllMain] Thread wait result: %lu\n", dwWaitResult); fflush(stdout);

        CloseHandle(hThread);

        VirtualFreeEx(ctx.hProcess, pShellcode, 0, MEM_RELEASE);
        printf("[CallDllMain] Success!\n"); fflush(stdout);
        return true;
    }
}

std::string Injector::GetPathNextToExe(const char* szFileName)
{
    char szExePath[MAX_PATH]{};
    if (!GetModuleFileNameA(nullptr, szExePath, MAX_PATH))
        return szFileName;

    std::filesystem::path path(szExePath);
    path.replace_filename(szFileName);
    return path.string();
}

bool Injector::Inject(const char* szProcessName, const std::string& strDllPath, std::string& strError)
{
    if (!std::filesystem::exists(strDllPath))
    {
        strError = "SkinCore.dll topilmadi:\n" + strDllPath;
        return false;
    }

    const DWORD dwPID = FindProcessID(szProcessName);
    if (dwPID == 0)
    {
        strError = "CS2 topilmadi. Avval o'yinni yoqing.";
        return false;
    }

    const HANDLE hProcess = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION  | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, dwPID);

    if (!hProcess)
    {
        strError = "CS2 ga ulanib bo'lmadi. Dasturni Administrator nomidan ishlating.";
        return false;
    }

    ManualMapCtx ctx{};
    ctx.hProcess = hProcess;
    bool bSuccess = false;

    do
    {
        printf("[Inject] Step 1: LoadFileToMemory\n"); fflush(stdout);
        if (!LoadFileToMemory(strDllPath.c_str(), ctx.pFileData, ctx.uFileSize))
        {
            strError = "[1/7] DLL faylni o'qib bo'lmadi.";
            break;
        }

        printf("[Inject] Step 2: ValidatePeHeaders\n"); fflush(stdout);
        if (!ValidatePeHeaders(ctx, strError))
        {
            strError = "[2/7] PE Headers: " + strError;
            break;
        }

        printf("[Inject] Step 3: AllocateImageMemory\n"); fflush(stdout);
        if (!AllocateImageMemory(ctx, strError))
        {
            strError = "[3/7] Allocate: " + strError;
            break;
        }

        printf("[Inject] Step 4: CopySections\n"); fflush(stdout);
        if (!CopySections(ctx, strError))
        {
            strError = "[4/7] Sections: " + strError;
            break;
        }

        printf("[Inject] Step 5: ApplyBaseRelocations\n"); fflush(stdout);
        if (!ApplyBaseRelocations(ctx, strError))
        {
            strError = "[5/7] Relocs: " + strError;
            break;
        }

        printf("[Inject] Step 6: FixImports\n"); fflush(stdout);
        if (!FixImports(ctx, strError))
        {
            strError = "[6/7] Imports: " + strError;
            break;
        }

        printf("[Inject] Step 7: CallDllMain\n"); fflush(stdout);
        if (!CallDllMain(ctx, strError))
        {
            strError = "[7/7] DllMain: " + strError;
            break;
        }

        printf("[Inject] Success!\n"); fflush(stdout);
        bSuccess = true;

    } while (false);

    if (ctx.pFileData)
        delete[] ctx.pFileData;

    CloseHandle(hProcess);
    return bSuccess;
}
