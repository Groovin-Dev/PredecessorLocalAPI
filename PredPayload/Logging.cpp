#include "Logging.h"
#include <Windows.h>
#include <cstdio>
#include <string>
#include <cstdarg>

#include "CommonConstants.h" // For DEBUG_CONSOLE
using namespace PredCommon;

static bool g_LoggerInitialized = false;
static NamedPipeServer* g_LoggerPipe = nullptr;
static CRITICAL_SECTION g_LogLock; // for thread safety

bool InitializeLogger()
{
    // Initialize a critical section for logging
    InitializeCriticalSection(&g_LogLock);

    g_LoggerInitialized = true;
    return true;
}

void ShutdownLogger()
{
    if (!g_LoggerInitialized) return;

    // If we allocated a console in your worker code, we can free it here or there.
    // This library doesn't allocate it directly, so we won't do it here.

    DeleteCriticalSection(&g_LogLock);
    g_LoggerInitialized = false;
}

void SetLoggerPipe(NamedPipeServer* pipe)
{
    EnterCriticalSection(&g_LogLock);
    g_LoggerPipe = pipe;
    LeaveCriticalSection(&g_LogLock);
}

// The low-level function that does the actual printing
void LogPrintf(const char* prefix, const char* fmt, va_list args)
{
    if (!g_LoggerInitialized) return;

    char buffer[1024];
    vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);

    // Build final string with prefix
    std::string out = std::string(prefix) + buffer + "\n";

    EnterCriticalSection(&g_LogLock);

    // Always try to write to stdout, even if console isn't allocated
    // it won't hurt anything
    printf("%s", out.c_str());
    fflush(stdout);

    // Also send to pipe if we have one
    if (g_LoggerPipe && g_LoggerPipe->IsConnected())
    {
        g_LoggerPipe->SendMessageToClient(MessageType::LOG, out);
    }

    LeaveCriticalSection(&g_LogLock);
}

// Convenience for info-level logs
void LogInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [INF] ", fmt, args);
    va_end(args);
}

// Convenience for error-level logs
void LogError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [ERR] ", fmt, args);
    va_end(args);
}

// Convenience for warning-level logs
void LogWarning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [WRN] ", fmt, args);
    va_end(args);
}

// Convenience for debug-level logs
void LogDebug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [DBG] ", fmt, args);
    va_end(args);
}
