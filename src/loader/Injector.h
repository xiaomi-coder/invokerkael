#pragma once

#include <Windows.h>
#include <string>

namespace Injector
{
    // Injects szDllPath into the first process named szProcessName.
    // On failure returns false and fills strError with a user-facing message.
    bool Inject(const char* szProcessName, const std::string& strDllPath, std::string& strError);

    // Full path to a file sitting next to the running executable.
    std::string GetPathNextToExe(const char* szFileName);
}
