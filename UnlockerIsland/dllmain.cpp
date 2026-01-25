#define WIN32_LEAN_AND_MEAN
#include "pch.h"
#include "../shared.h"
#include <atomic>


namespace UnlockerIsland
{
    HMODULE GameAssemblyD;
    HMODULE UnityPlayerD;

    typedef void* (*Il2CppResolveICallFunc)(const char* name);

    typedef void (*SetTargetFrameRateFunc)(int value);

    typedef void (*Camera_set_fieldOfView_t)(void* _this, float value);

    inline Camera_set_fieldOfView_t g_OriginalSetFieldOfView = nullptr;
	float g_fov_value = 90.0f;

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

    void Hook_set_Fov(void* _this, float value) {
#ifdef _DEBUG
        std::cout << "Hook_set_Fov-value:" << value << std::endl;
#endif

		value = g_fov_value;
        return g_OriginalSetFieldOfView(_this, value);
	}

    bool ControlFOVHook(bool isInstall) {
        if (!g_resolve_icall) {
            MessageBoxW(nullptr, L"Failed to get il2cpp_resolve_icall!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
            return false;
        }
        static void* s_targetFuncAddr = nullptr;

        if (s_targetFuncAddr == nullptr) {

            s_targetFuncAddr = (void*)PatternScanner::ScanModule("40 53 48 81 EC ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 8B D9 0F 28 F1 E8 ? ? ? ? 84 C0",L"UnityPlayer.dll");
			std::cout << "set_fieldOfView addr: " << std::hex << (uintptr_t)s_targetFuncAddr << std::dec << std::endl;


            if (!s_targetFuncAddr) {
                MessageBoxW(nullptr, L"Failed to resolve set_fieldOfView!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
                return false;
            }
        }

        if (isInstall) {
            bool result = MinHookManager::Add(
                s_targetFuncAddr,
                &Hook_set_Fov,
                (void**)&g_OriginalSetFieldOfView
            );
            if (!result) {
                if (MinHookManager::Enable(s_targetFuncAddr)) {
                    return true;
                }
                MessageBoxW(nullptr, L"Failed to create/enable FOV Hook!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
                return false;
            }
            return true;
        }
        else {
            bool result = MinHookManager::Disable(s_targetFuncAddr);
            return result;
        }
    }

    bool initunlock()
    {
        while (GameAssemblyD == NULL) {
            GameAssemblyD = GetModuleHandleA("GameAssembly.dll");
            if (GameAssemblyD == NULL) {
                Sleep(1000);
            }
        }
        while (UnityPlayerD == NULL) {
            UnityPlayerD = GetModuleHandleA("UnityPlayer.dll");
            if (UnityPlayerD == NULL) {
                Sleep(1000);
            }
        }

        Sleep(1000);

        if (!InitIl2Cpp(GameAssemblyD))
        {
            return false;
        }
        return true;
    }
}

void Run()
{


    UnlockerIsland::initunlock();

	Sleep(5000);

    HANDLE map_handle = nullptr;
    SharedData* shared = nullptr;
    LONG last_seq = -1;
	bool fov_hook_then = false;

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

            UnlockerIsland::g_fov_value = shared->target_fov;
            if (shared->hookfov_enabled && !fov_hook_then)
            {
                UnlockerIsland::ControlFOVHook(true);
                fov_hook_then = true;
            }
            else if (!shared->hookfov_enabled&& fov_hook_then)
            {
                UnlockerIsland::ControlFOVHook(false);
				fov_hook_then = false;
            }
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
    {
#ifdef _DEBUG
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
#endif
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Run, NULL, 0, NULL);
    }

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

