#pragma once

// This file contains code from
// https://github.com/stevemk14ebr/PolyHook_2_0/blob/master/sources/IatHook.cpp
// which is licensed under the MIT License.
// See PolyHook_2_0-LICENSE for more information.

#include <Windows.h>

inline void* RVA2VA(void* base, ULONG_PTR rva)
{
	return (void*)((ULONG_PTR)base + rva);
}

inline void* DataDirectoryFromModuleBase(void *moduleBase, size_t entryID)
{
    PIMAGE_DOS_HEADER dosHdr = (PIMAGE_DOS_HEADER)(moduleBase);
    PIMAGE_NT_HEADERS ntHdr = RVA2VA(moduleBase, dosHdr->e_lfanew);
	IMAGE_DATA_DIRECTORY* dataDir = ntHdr->OptionalHeader.DataDirectory;
    return RVA2VA(moduleBase, dataDir[entryID].VirtualAddress);
}

PIMAGE_THUNK_DATA FindAddressByName(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, const char *funcName)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
			continue;

        PIMAGE_IMPORT_BY_NAME import = (PIMAGE_IMPORT_BY_NAME)RVA2VA(moduleBase, impName->u1.AddressOfData);
		if (strcmp(import->Name, funcName) != 0)
			continue;
		return impAddr;
	}
	return NULL;
}

PIMAGE_THUNK_DATA FindAddressByOrdinal(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}
	return NULL;
}

PIMAGE_THUNK_DATA FindIatThunkInModule(void *moduleBase, const char *dllName, const char *funcName)
{
    PIMAGE_IMPORT_DESCRIPTOR imports = DataDirectoryFromModuleBase(moduleBase, IMAGE_DIRECTORY_ENTRY_IMPORT);
	for (; imports->Name; ++imports)
	{
		if (_stricmp((LPCSTR)RVA2VA(moduleBase, imports->Name), dllName) != 0)
			continue;

        PIMAGE_THUNK_DATA origThunk = RVA2VA(moduleBase, imports->OriginalFirstThunk);
        PIMAGE_THUNK_DATA thunk = RVA2VA(moduleBase, imports->FirstThunk);
		return FindAddressByName(moduleBase, origThunk, thunk, funcName);
	}
	return NULL;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, const char *funcName)
{
    PIMAGE_DELAYLOAD_DESCRIPTOR imports = DataDirectoryFromModuleBase(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp((LPCSTR)RVA2VA(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

        PIMAGE_THUNK_DATA impName = RVA2VA(moduleBase, imports->ImportNameTableRVA);
        PIMAGE_THUNK_DATA impAddr = RVA2VA(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByName(moduleBase, impName, impAddr, funcName);
	}
	return NULL;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModuleOrd(void *moduleBase, const char *dllName, uint16_t ordinal)
{
    PIMAGE_DELAYLOAD_DESCRIPTOR imports = DataDirectoryFromModuleBase(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp((LPCSTR)RVA2VA(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

        PIMAGE_THUNK_DATA impName = RVA2VA(moduleBase, imports->ImportNameTableRVA);
        PIMAGE_THUNK_DATA impAddr = RVA2VA(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}
	return NULL;
}
