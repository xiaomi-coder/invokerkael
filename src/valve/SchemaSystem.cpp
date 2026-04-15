#include "../Includes.h"

bool SchemaSystem::Setup()
{
    std::uintptr_t uSchemaInterfaceAddress = g_Memory.PatternScan(SCHEMASYSTEM_DLL, X("48 89 05 ? ? ? ? 4C 8D 0D ? ? ? ? 33 C0 48 C7 05 ? ? ? ? ? ? ? ? 89 05"), EPatternScanFlags::SCAN_RESOLVE_RIP, 0x3, 0x7);
    std::uintptr_t uSchemaSystemScopeArrayPtr = 0U;
    if (!g_Memory.ReadMemoryRaw(uSchemaInterfaceAddress + CS_OFFSETOF(CSchemaSystem, m_pScopeArray), &uSchemaSystemScopeArrayPtr, sizeof(std::uintptr_t)))
    {
        std::cout << X("Failed to read scope array ptr") << std::endl;
        return false;
    }
  
    int nScopeSize = g_Memory.ReadMemory<int>(uSchemaInterfaceAddress + CS_OFFSETOF(CSchemaSystem, m_nScopeSize));
    void** ppScopeArray = new void* [nScopeSize];

    if (!g_Memory.ReadMemoryRaw(uSchemaSystemScopeArrayPtr, ppScopeArray, (nScopeSize * sizeof(void*))))
    {
        std::cout << X("Failed to read scope array") << std::endl;
        return false;
    }

    for (std::uint16_t i = 0U; i <= nScopeSize; ++i)
    {
        CSchemaSystemTypeScope schemaScope{};
        if (!g_Memory.ReadMemoryRaw(ppScopeArray[i], &schemaScope, sizeof(CSchemaSystemTypeScope)) || !schemaScope.m_pDeclaredClasses)
            continue;

        CSchemaDeclaredClassEntry* pDeclaredClassEntries = new CSchemaDeclaredClassEntry[schemaScope.m_uNumDeclaredClasses + 1U];
        if (!g_Memory.ReadMemoryRaw(schemaScope.m_pDeclaredClasses, pDeclaredClassEntries, (schemaScope.m_uNumDeclaredClasses + 1U) * sizeof(CSchemaDeclaredClassEntry)))
            continue;
     
        if (strcmp(schemaScope.m_szName, X("client.dll")) == 0)
        {     
            for (std::uint16_t j = 0U; j < schemaScope.m_uNumDeclaredClasses; ++j)
            {
                CSchemaDeclaredClass declaredClass{ };
                if (!g_Memory.ReadMemoryRaw(pDeclaredClassEntries[j].m_pDeclaredClass, &declaredClass, sizeof(CSchemaDeclaredClass)))
                    continue;

                CSchemaClass schemaClass{ };
                if (!g_Memory.ReadMemoryRaw(declaredClass.m_Class, &schemaClass, sizeof(CSchemaClass)))
                    continue;

                char szClassName[128]{};
                if (!g_Memory.ReadMemoryRaw((void*)(declaredClass.m_szName), szClassName, sizeof(szClassName)))
                    continue;

                std::uintptr_t uClassFieldsPtr = reinterpret_cast<uintptr_t>(schemaClass.m_pFields);
                if (uClassFieldsPtr)
                {
                    for (std::uint16_t k = 0; k < schemaClass.m_uNumFields; ++k)
                    {
                        CSchemaField schemaField = g_Memory.ReadMemory<CSchemaField>(uClassFieldsPtr + (sizeof(CSchemaField) * k));
                        if (!schemaField.m_pType)
                            continue;

                        char szFieldName[128] = { 0 };
                        if (!g_Memory.ReadMemoryRaw((void*)(schemaField.m_szName), szFieldName, sizeof(szFieldName)))
                            continue;

                        const std::string strSchemaField = std::vformat(X("{}->{}"), std::make_format_args(szClassName, szFieldName));
                        m_mapSchemaOffsets[FNV1A::Hash(strSchemaField.c_str())] = schemaField.m_uOffset;
                    }
                }
            }
        }    
    }

    delete[] ppScopeArray;

    // Dump Econ-related schema entries to a log file for debugging
    {
        std::ofstream ofs("schema_dump.txt", std::ios::out | std::ios::trunc);
        if (ofs.is_open())
        {
            ofs << "=== Schema Dump - Total entries: " << m_mapSchemaOffsets.size() << " ===" << std::endl;
            ofs << std::endl;

            // We need to re-scan to get the actual string names, since we only stored hashes
            // Re-iterate the schema to dump Econ/Fallback related entries
            std::uintptr_t uSchemaInterfaceAddress2 = g_Memory.PatternScan(SCHEMASYSTEM_DLL, X("48 89 05 ? ? ? ? 4C 8D 0D ? ? ? ? 33 C0 48 C7 05 ? ? ? ? ? ? ? ? 89 05"), EPatternScanFlags::SCAN_RESOLVE_RIP, 0x3, 0x7);
            std::uintptr_t uScopePtr2 = 0U;
            g_Memory.ReadMemoryRaw(uSchemaInterfaceAddress2 + CS_OFFSETOF(CSchemaSystem, m_pScopeArray), &uScopePtr2, sizeof(std::uintptr_t));
            int nSize2 = g_Memory.ReadMemory<int>(uSchemaInterfaceAddress2 + CS_OFFSETOF(CSchemaSystem, m_nScopeSize));
            void** ppArr2 = new void*[nSize2];
            g_Memory.ReadMemoryRaw(uScopePtr2, ppArr2, (nSize2 * sizeof(void*)));

            for (std::uint16_t i = 0U; i <= nSize2; ++i)
            {
                CSchemaSystemTypeScope scope{};
                if (!g_Memory.ReadMemoryRaw(ppArr2[i], &scope, sizeof(CSchemaSystemTypeScope)) || !scope.m_pDeclaredClasses)
                    continue;
                if (strcmp(scope.m_szName, X("client.dll")) != 0)
                    continue;

                CSchemaDeclaredClassEntry* entries = new CSchemaDeclaredClassEntry[scope.m_uNumDeclaredClasses + 1U];
                if (!g_Memory.ReadMemoryRaw(scope.m_pDeclaredClasses, entries, (scope.m_uNumDeclaredClasses + 1U) * sizeof(CSchemaDeclaredClassEntry)))
                    continue;

                for (std::uint16_t j = 0U; j < scope.m_uNumDeclaredClasses; ++j)
                {
                    CSchemaDeclaredClass decl{};
                    if (!g_Memory.ReadMemoryRaw(entries[j].m_pDeclaredClass, &decl, sizeof(CSchemaDeclaredClass)))
                        continue;
                    CSchemaClass cls{};
                    if (!g_Memory.ReadMemoryRaw(decl.m_Class, &cls, sizeof(CSchemaClass)))
                        continue;
                    char clsName[128]{};
                    if (!g_Memory.ReadMemoryRaw((void*)(decl.m_szName), clsName, sizeof(clsName)))
                        continue;

                    std::string sClsName(clsName);


                    std::uintptr_t fieldsPtr = reinterpret_cast<uintptr_t>(cls.m_pFields);
                    if (!fieldsPtr) continue;

                    ofs << "[" << sClsName << "] (" << cls.m_uNumFields << " fields)" << std::endl;
                    for (std::uint16_t k = 0; k < cls.m_uNumFields; ++k)
                    {
                        CSchemaField field = g_Memory.ReadMemory<CSchemaField>(fieldsPtr + (sizeof(CSchemaField) * k));
                        if (!field.m_pType) continue;
                        char fName[128]{};
                        if (!g_Memory.ReadMemoryRaw((void*)(field.m_szName), fName, sizeof(fName)))
                            continue;
                        ofs << "  " << sClsName << "->" << fName << " = 0x" << std::hex << field.m_uOffset << std::dec << std::endl;
                    }
                    ofs << std::endl;
                }
                delete[] entries;
            }
            delete[] ppArr2;
            ofs << "=== End of dump ===" << std::endl;
            ofs.close();
        }
    }

	return m_mapSchemaOffsets.size() > 0;
}