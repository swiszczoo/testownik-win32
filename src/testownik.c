#include <testownik.h>

#include <PathCch.h>
#include <Shlwapi.h>

#include <string.h>

static testownik APP;

void testownik_init()
{
    GetModuleFileName(GetModuleHandle(NULL), APP.db_path, MAX_PATH + 16);
    PathCchRemoveFileSpec(APP.db_path, MAX_PATH + 16);
    PathCombine(APP.db_path, APP.db_path, L"qdb");
}

bool testownik_try_load_database()
{
    if (!PathIsDirectory(APP.db_path)) {
        return false;
    }

    WIN32_FIND_DATA ffd;
    HANDLE search_handle;
    TCHAR filter[MAX_PATH + 16];

    PathCombine(filter, APP.db_path, L"*.txt");

    search_handle = FindFirstFile(filter, &ffd);
    if (search_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {

    } while (FindNextFile(search_handle, &ffd));

    DWORD findError = GetLastError();
    FindClose(search_handle);

    return findError == ERROR_NO_MORE_FILES;
}

