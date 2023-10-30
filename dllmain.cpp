#include <Windows.h>
#include <iostream>
#include "memory.hpp"
#include <winhttp.h>


typedef BOOL(__stdcall* prototype_wglSwapBuffers)(HDC hdc);
prototype_wglSwapBuffers trampoline_wglSwapBuffers = nullptr;

BOOL __stdcall detour_wglSwapBuffers(HDC hdc) {
    printf("rendering\n");
    return trampoline_wglSwapBuffers(hdc);
}


DWORD WINAPI main_thread(HMODULE hModule) {
    char* wglSwapBuffers = Memory::get_exported_function_address("OPENGL32.dll", "wglSwapBuffers");

    trampoline_wglSwapBuffers = reinterpret_cast<prototype_wglSwapBuffers>(Memory::hook_function(wglSwapBuffers, reinterpret_cast<char*>(detour_wglSwapBuffers)));
    if (trampoline_wglSwapBuffers == nullptr) {
        FreeLibraryAndExitThread(hModule, 0);
    }

    while (true) {
        if (GetAsyncKeyState(VK_END))
            break;
    }

    Memory::unhook_functions();
    FreeLibraryAndExitThread(hModule, 0);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    DisableThreadLibraryCalls(hModule);
    
    static FILE* f = nullptr;

    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        AllocConsole();
        freopen_s(&f, "CONOUT$", "w", stdout);

        if (f == nullptr) {
            FreeConsole();
            FreeLibraryAndExitThread(hModule, 1);
        }

        HANDLE hThread = CreateThread(0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(main_thread), hModule, 0, 0);
        if (!hThread) {
            FreeConsole();
            FreeLibraryAndExitThread(hModule, 1);
        }

        CloseHandle(hThread);

    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        if (f != nullptr) fclose(f);
        FreeConsole();
    }

    return 1;
}

