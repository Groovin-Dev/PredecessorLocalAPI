#include <Windows.h>
#include <atomic>

// Forward-declare our worker thread function
DWORD WINAPI PayloadWorkerThread(LPVOID lpParam);

// Global flags
static std::atomic_bool g_Unloading{false};
static HMODULE g_hModule = nullptr;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        g_hModule = hModule;
        {
            // Spawn a worker thread
            HANDLE hThread = CreateThread(nullptr, 0, PayloadWorkerThread, nullptr, 0, nullptr);
            if (hThread) CloseHandle(hThread);
        }
        break;

    case DLL_PROCESS_DETACH:
        // If we haven't already triggered an unload, do minimal
        if (!g_Unloading)
        {
            g_Unloading = true;
        }
        break;
    }
    return TRUE;
}
