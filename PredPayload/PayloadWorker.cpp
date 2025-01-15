#include "PayloadWorker.h"

#include <thread>
#include <atomic>
#include <string>
#include <sstream>

#include "KPE_HookLib/KEI_PE_HOOK.hpp"
#include "SDK/Engine_classes.hpp"
#include "SDK/Engine_parameters.hpp"

#include "NamedPipeServer.h"
#include "CommonConstants.h"

#include "Logging.h"
#include "Hooks.h"
#include "WebSocketServer.h"

// Static or global flags
static std::atomic_bool g_Running = true;
static std::atomic_bool g_Unloading = false;
static PredCommon::NamedPipeServer g_PipeServer;
static HANDLE g_ConsoleHandle = nullptr;
// static std::unique_ptr<WebSocketServer> g_WebSocketServer;
std::unique_ptr<WebSocketServer> g_WebSocketServer;

// Forward declarations
static void MainLoop();
static BOOL WINAPI ConsoleHandler(DWORD ctrlType);
static void CleanupAndExit();

DWORD WINAPI PayloadWorkerThread(LPVOID /*lpParam*/)
{
    // 1) Console setup
    if (PredCommon::DEBUG_CONSOLE)
    {
        AllocConsole();
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        SetConsoleTitle(L"Predecessor Payload Debug Console");
        setvbuf(stdout, nullptr, _IONBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        // Store console handle and set up control handler
        g_ConsoleHandle = GetConsoleWindow();
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    }

    // 2) Initialize logging (console only for now)
    if (!InitializeLogger())
    {
        printf("Failed to initialize logger\n");
        return 0;
    }
    LogInfo("[Payload] Logger initialized");

    // 3) Create pipe server first
    LogInfo("[Payload] Creating named pipe server...");
    if (!g_PipeServer.Create(PredCommon::PIPE_NAME))
    {
        LogError("[Payload] Failed to create pipe server. Exiting.");
        CleanupAndExit();
        return 0;
    }
    LogInfo("[Payload] Pipe created.");

    // 4) Create and set the ready event
    LogInfo("[Payload] Creating ready event...");
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    HANDLE readyEvent = CreateEventA(&sa, TRUE, FALSE, "Global\\PredecessorPayloadReady");
    if (!readyEvent)
    {
        LogError("[Payload] Failed to create ready event. Error: %d", GetLastError());
        CleanupAndExit();
        return 0;
    }
    SetEvent(readyEvent); // Signal we're ready
    LogInfo("[Payload] Ready event set");

    // 5) Wait for client connection
    LogInfo("[Payload] Waiting for client...");
    if (!g_PipeServer.WaitForClient(5000)) // 5 second timeout
    {
        LogError("[Payload] Timeout waiting for client.");
        CloseHandle(readyEvent);
        CleanupAndExit();
        return 0;
    }
    LogInfo("[Payload] Client connected to named pipe.");
    SetLoggerPipe(&g_PipeServer);
    LogInfo("[Payload] Pipe connection established for logging");

    // 6) Initialize WebSocket server
    LogInfo("[Payload] Starting WebSocket server...");
    g_WebSocketServer = std::make_unique<WebSocketServer>();
    if (!g_WebSocketServer->Start())
    {
        LogError("[Payload] Failed to start WebSocket server");
        CleanupAndExit();
        return 0;
    }
    LogInfo("[Payload] WebSocket server started");

    // 7) Initialize engine access
    LogInfo("[Payload] Initializing engine access...");
    UWorld* world = UWorld::GetWorld();
    UGameEngine* gameEngine = UGameEngine::GetEngine();
    LogInfo("[Payload] UWorld at 0x%p", world);
    LogInfo("[Payload] UGameEngine at 0x%p", gameEngine);

    // 8) Install hooks
    LogInfo("[Payload] Installing hooks...");
    InstallHooks();
    LogInfo("[Payload] Hooks installation complete");

    // 9) Main loop
    LogInfo("[Payload] Entering main loop");
    MainLoop();

    // 10) Cleanup
    LogInfo("[Payload] Beginning cleanup...");
    CloseHandle(readyEvent);
    CleanupAndExit();

    return 0;
}

static bool SendHeartbeat()
{
    if (!g_PipeServer.SendMessageToClient(PredCommon::MessageType::LOG, "[LOG] [INF] [Payload] Payload heartbeat\n"))
    {
        DWORD err = GetLastError();
        if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA)
        {
            LogError("[Payload] Loader disconnected, shutting down...");
            return false;
        }
    }
    return true;
}

static void MainLoop()
{
    while (g_Running && !g_Unloading)
    {
        if (!SendHeartbeat())
        {
            g_Running = false;
            g_Unloading = true;
            break;
        }

        Sleep(1000);
    }

    LogInfo("[Payload] Exiting main loop.");
    g_Running = false;
}

static void CleanupAndExit()
{
    static std::atomic_bool cleaningUp(false);
    bool expected = false;

    if (!cleaningUp.compare_exchange_strong(expected, true))
        return;

    LogInfo("[Payload] Starting cleanup process...");

    g_Running = false;
    g_Unloading = true;

    LogInfo("[Payload] Removing hooks...");
    RemoveHooks();

    LogInfo("[Payload] Disconnecting pipe...");
    g_PipeServer.Disconnect();

    LogInfo("[Payload] Shutting down logger...");
    ShutdownLogger();

    if (g_WebSocketServer)
    {
        LogInfo("[Payload] Stopping WebSocket server...");
        g_WebSocketServer->Stop();
        g_WebSocketServer.reset();
    }

    if (g_ConsoleHandle)
    {
        printf("[Payload] Cleanup complete. It's (probably) safe to close the console.\n");
        FreeConsole();
        g_ConsoleHandle = nullptr;
    }

    // Yes... I know we are unloading the module and not saying anyting to the console but
    // this is the best way I could do it so whatever
    HMODULE hModule = GetModuleHandleA("PredPayload.dll");
    if (hModule)
    {
        Sleep(100);
        FreeLibraryAndExitThread(hModule, 0);
    }

    ExitThread(0);
}


static BOOL WINAPI ConsoleHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        // Initiate graceful shutdown
        g_Running = false;
        g_Unloading = true;

    // Give the main loop a little time to exit gracefully
        Sleep(500);

    // Force cleanup if needed
        CleanupAndExit();
        return TRUE;
    default:
        return FALSE;
    }
}
