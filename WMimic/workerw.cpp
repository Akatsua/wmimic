#include "workerw.h"

BOOL CALLBACK searchWallpaperHandle(HWND handle, LPARAM result)
{
    HWND  shellHandle  = FindWindowEx(handle, nullptr, L"SHELLDLL_DefView", nullptr);
    HWND* resultHandle = (HWND*)result;

    if (shellHandle != nullptr)
    {
        *resultHandle = FindWindowEx(nullptr, handle, L"WorkerW", nullptr);
    }

    return true;
}

HWND SpawnWorkerW()
{
    // Based on https://www.codeproject.com/articles/856020/draw-behind-desktop-icons-in-windows-plus
    // Improve: Test this thoroughly
    HWND progHandle = FindWindowA("Progman", nullptr);
    
    DWORD_PTR data;
    LRESULT result = SendMessageTimeoutA(progHandle, 0x052C, 0, 0, SMTO_NORMAL, 1000, &data);

    //SendMessageA(progHandle, 0x052C, 0xD, 0);
    //SendMessageA(progHandle, 0x052C, 0xD, 1);

    HWND workerwHandle = nullptr;
    int success = EnumWindows(searchWallpaperHandle, (LPARAM)&workerwHandle);

    return workerwHandle;
}

