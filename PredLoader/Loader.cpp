#include "Loader.h"
#include "Injector.h"

#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

#include "CommonConstants.h"
#include "NamedPipeClient.h"

using namespace PredCommon;

static DWORD FindProcessId(const std::string& processName)
{
    DWORD pid = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 pe{};
        pe.dwSize = sizeof(pe);
        if (Process32First(snap, &pe))
        {
            do
            {
                // Convert wide string (szExeFile) to std::string
                std::wstring wExeFile(pe.szExeFile);
                std::string exeFile(wExeFile.begin(), wExeFile.end());

                if (exeFile == processName)
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
    }
    return pid;
}

Loader::Loader()
    : m_TargetProcessName("PredecessorClient-Win64-Shipping.exe"),
    m_PayloadDllName("PredPayload.dll"),
    m_TargetPID(0),
    m_Running(true)
{
}

Loader::~Loader()
{ }

bool Loader::Init()
{
    std::cout << "[Loader] Initializing...\n";

    // Step 1: Find process & inject
    if (!FindAndInject())
    {
        std::cerr << "[Loader] Injection failed or process not found.\n";
        return false;
    }

    // 1.a Wait a little (Just so the payload actually has time to setup)
    Sleep(500);

    // Step 2: Connect to named pipe
    if (!ConnectToPayloadServer())
    {
        std::cerr << "[Loader] Could not connect to Payload's named pipe.\n";
        return false;
    }

    std::cout << "[Loader] Initialization complete.\n";
    return true;
}

bool Loader::FindAndInject()
{
    std::cout << "[Loader] Looking for process: " << m_TargetProcessName << "\n";
    m_TargetPID = FindProcessId(m_TargetProcessName);
    if (m_TargetPID == 0)
    {
        std::cerr << "[Loader] Could not find target process.\n";
        return false;
    }

    std::cout << "[Loader] Found PID: " << m_TargetPID << "\n";

    // Inject
    if (!Injector::InjectDLL(m_TargetPID, m_PayloadDllName))
    {
        std::cerr << "[Loader] DLL injection failed.\n";
        return false;
    }

    std::cout << "[Loader] DLL injected successfully.\n";
    return true;
}

bool Loader::ConnectToPayloadServer()
{
    std::cout << "[Loader] Waiting for payload to initialize...\n";
    
    // Wait for payload to signal it's ready
    HANDLE readyEvent = OpenEventA(SYNCHRONIZE, FALSE, "Global\\PredecessorPayloadReady");
    if (!readyEvent)
    {
        std::cerr << "[Loader] Failed to open ready event. Error: " << GetLastError() << "\n";
        return false;
    }

    // Wait up to 10 seconds for the payload to be ready
    if (WaitForSingleObject(readyEvent, 10000) != WAIT_OBJECT_0)
    {
        std::cerr << "[Loader] Timeout waiting for payload to initialize.\n";
        CloseHandle(readyEvent);
        return false;
    }
    CloseHandle(readyEvent);

    std::cout << "[Loader] Payload is ready. Connecting to pipe...\n";

    const int maxAttempts = 10;
    int attempts = 0;

    while (attempts < maxAttempts)
    {
        if (m_PipeClient.Connect(PredCommon::PIPE_NAME))
        {
            std::cout << "[Loader] Connected to payload's pipe!\n";
            return true;
        }

        std::cout << "[Loader] Failed to connect, retrying...\n";
        Sleep(1000);
        attempts++;
    }

    std::cerr << "[Loader] Could not connect after " << maxAttempts << " attempts.\n";
    return false;
}

void Loader::RunLoop()
{
    std::cout << "[Loader] Entering main loop. Press Ctrl+C to exit.\n";

    while (m_Running)
    {
        // Read and display messages from the payload
        PredCommon::MessageType msgType;
        std::string msgText;
        
        if (!m_PipeClient.ReadMessage(msgType, msgText))
        {
            DWORD error = GetLastError();
            if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA)
            {
                std::cout << "[Loader] Pipe disconnected, payload has terminated.\n";
                break;
            }
            Sleep(100);
            continue;
        }

        // Print the message
        std::cout << msgText;
    }
}
