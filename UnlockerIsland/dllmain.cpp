#define WIN32_LEAN_AND_MEAN
#include "pch.h"
#include "../shared.h"


namespace UnlockerIsland
{
    HMODULE GameAssemblyH;

    typedef void* (*Il2CppResolveICallFunc)(const char* name);

    typedef void (*SetTargetFrameRateFunc)(int value);

    Il2CppResolveICallFunc g_resolve_icall = nullptr;

    bool InitIl2Cpp(HMODULE libIl2CppHandle) {
        g_resolve_icall = (Il2CppResolveICallFunc)GetProcAddress(libIl2CppHandle, "il2cpp_resolve_icall");
        if (!g_resolve_icall) {
			MessageBoxW(nullptr, L"Failed to get il2cpp_resolve_icall!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
			return false;
        }
		return true;
    }

    bool set_fps(int targetFPS) {
        if (!g_resolve_icall) return false;
        void* funcAddr = g_resolve_icall("UnityEngine.Application::set_targetFrameRate(System.Int32)");

        if (funcAddr) {
            SetTargetFrameRateFunc setFPS = (SetTargetFrameRateFunc)funcAddr;
            setFPS(targetFPS);
        }
        else {
			MessageBoxW(nullptr, L"Failed to resolve set_targetFrameRate!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
			return false;
        }
		return true;
    }



    bool initunlock()
    {
        while (GameAssemblyH == NULL) {
            GameAssemblyH = GetModuleHandleA("GameAssembly.dll");
            if (GameAssemblyH == NULL) {
                Sleep(1000);
            }
        }
        Sleep(1000);

        if (!InitIl2Cpp(GameAssemblyH))
        {
            return false;
        }
        return true;
    }
}

void Run()
{
    UnlockerIsland::initunlock();

	Sleep(3000);

    HANDLE map_handle = nullptr;
    SharedData* shared = nullptr;
    LONG last_seq = -1;

    while (true)
    {
        if (!map_handle)
        {
            map_handle = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kMapName);
            if (map_handle)
            {
                shared = (SharedData*)MapViewOfFile(map_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
                if (!shared)
                {
                    CloseHandle(map_handle);
                    map_handle = nullptr;
                }
            }
        }

        if (shared && shared->magic == kMagic)
        {
            LONG seq_now = shared->seq;
            if (seq_now != last_seq)
            {
                last_seq = seq_now;
            }
            if (shared->unlock_enabled)
                UnlockerIsland::set_fps(shared->target_fps);
        }

        Sleep(500);
    }

}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Run, NULL, 0, NULL);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

