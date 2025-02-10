#include "Injector.h"
#include <Windows.h>
#include <iostream>

namespace Injector
{
    bool InjectDLL(unsigned long processId, const std::string& dllPath)
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
        if (!hProcess)
        {
            std::cerr << "[Injector] OpenProcess failed. Error: " << GetLastError() << "\n";
            return false;
        }

        // Resolve the absolute path to the DLL
        char absolutePath[MAX_PATH];
        if (!GetFullPathNameA(dllPath.c_str(), MAX_PATH, absolutePath, nullptr))
        {
            std::cerr << "[Injector] GetFullPathNameA failed. Error: " << GetLastError() << "\n";
            CloseHandle(hProcess);
            return false;
        }

        // Allocate memory in the remote process for the DLL path
        size_t size = strlen(absolutePath) + 1;
        LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!remoteMem)
        {
            std::cerr << "[Injector] VirtualAllocEx failed. Error: " << GetLastError() << "\n";
            CloseHandle(hProcess);
            return false;
        }

        // Write the DLL path to the remote process memory
        if (!WriteProcessMemory(hProcess, remoteMem, absolutePath, size, nullptr))
        {
            std::cerr << "[Injector] WriteProcessMemory failed. Error: " << GetLastError() << "\n";
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        // Get address of LoadLibraryA
        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (!hKernel32)
        {
            std::cerr << "[Injector] Could not get handle for kernel32.dll\n";
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        FARPROC loadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");
        if (!loadLibAddr)
        {
            std::cerr << "[Injector] GetProcAddress(LoadLibraryA) failed.\n";
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        // Create a remote thread that calls LoadLibraryA(dllPath)
        HANDLE hThread = CreateRemoteThread(
            hProcess,
            nullptr,
            0,
            (LPTHREAD_START_ROUTINE)loadLibAddr,
            remoteMem,
            0,
            nullptr
        );

        if (!hThread)
        {
            std::cerr << "[Injector] CreateRemoteThread failed. Error: " << GetLastError() << "\n";
            VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        // Wait for the thread to finish
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        // Free the memory we allocated (optional, though often done)
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);

        // Done
        CloseHandle(hProcess);
        return true;
    }
}
