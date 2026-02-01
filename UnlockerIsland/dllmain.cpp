#define WIN32_LEAN_AND_MEAN
#include "pch.h"
#include "../shared.h"
#include <atomic>


namespace UnlockerIsland
{
    HMODULE GameAssemblyD;
    HMODULE UnityPlayerD;

    struct Camera
    {
        void* klass;      // 0x00
        void* monitor;    // 0x08
        void* cachedPtr;  // 0x10
    };

    typedef void* (*Il2CppResolveICallFunc)(const char* name);

    typedef void (*SetTargetFrameRateFunc)(int value);

    typedef void (*Camera_set_fieldOfView_t)(void* _this, float value);

    inline Camera_set_fieldOfView_t g_OriginalSetFieldOfView = nullptr;
	float g_fov_value = 90.0f;

    Il2CppResolveICallFunc g_resolve_icall = nullptr;

    using Camera_get_main_t = Camera * (*)();

    typedef void (*Camera_SetEntityDither_t)(void* entity, float alpha, void* source);

    inline Camera_SetEntityDither_t g_OriginalSetEntityDither = nullptr;


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

    void* GetMainCameraNative()
    {
        static Camera_get_main_t get_main = nullptr;
        if (!get_main && g_resolve_icall)
            get_main = (Camera_get_main_t)g_resolve_icall("UnityEngine.Camera::get_main()");

        Camera* cam = get_main ? get_main() : nullptr;
        return cam ? cam->cachedPtr : nullptr;
    }

    void Hook_set_Fov(void* _this, float value)
    {
        void* mainNative = GetMainCameraNative();
        if (mainNative && _this == mainNative)
            value = g_fov_value;

        g_OriginalSetFieldOfView(_this, value);
    }

    void Hook_SetEntityDither(void* entity, float alpha, void* source)
    {
        alpha = 1.0f;
        g_OriginalSetEntityDither(entity, alpha, source);
    }


    bool ControlFOVHook(bool isInstall) {
        if (!g_resolve_icall) {
            MessageBoxW(nullptr, L"Failed to get il2cpp_resolve_icall!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
            return false;
        }
        static void* s_targetFuncAddr = nullptr;

        if (s_targetFuncAddr == nullptr) {

            s_targetFuncAddr = (void*)PatternScanner::ScanModule("40 53 48 81 EC ? ? ? ? 0F 29 B4 24 ? ? ? ? 48 8B D9 0F 28 F1 E8 ? ? ? ? 84 C0",L"UnityPlayer.dll");

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

    bool ControlDitherHook(bool isInstall) {
        if (!g_resolve_icall) {
            MessageBoxW(nullptr, L"Failed to get il2cpp_resolve_icall!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
            return false;
        }
        static void* s_targetFuncAddr2 = nullptr;

        if (s_targetFuncAddr2 == nullptr) {

            s_targetFuncAddr2 = (void*)PatternScanner::ScanModule("48 89 5C 24 ? 56 48 83 EC ? 80 3D ? ? ? ? 00 41 8B F0 0F 29 74 24 ? 48 8B D9 0F 28 F1 75 ? 48 8D 0D ? ? ? ? E8 ? ? ? ? C6 05 ? ? ? ? ? 33 D2 48 89 6C 24", L"GameAssembly.dll");

            if (!s_targetFuncAddr2) {
                MessageBoxW(nullptr, L"Failed to resolve Dither!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
                return false;
            }
        }

        if (isInstall) {
            bool result = MinHookManager::Add(
                s_targetFuncAddr2,
                &Hook_SetEntityDither,
                (void**)&g_OriginalSetEntityDither
            );
            if (!result) {
                if (MinHookManager::Enable(s_targetFuncAddr2)) {
                    return true;
                }
                MessageBoxW(nullptr, L"Failed to create/enable Dither Hook!", L"UnlockerIsland", MB_OK | MB_ICONERROR);
                return false;
            }
            return true;
        }
        else {
            bool result = MinHookManager::Disable(s_targetFuncAddr2);
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
    bool fov_Dither_then = false;

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

            if (shared->hookDither_enabled && !fov_Dither_then)
            {
                UnlockerIsland::ControlDitherHook(true);
                fov_Dither_then = true;
            }
            else if (!shared->hookDither_enabled && fov_Dither_then)
            {
                UnlockerIsland::ControlDitherHook(false);
                fov_Dither_then = false;
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

