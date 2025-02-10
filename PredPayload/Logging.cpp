#include "Logging.h"
#include <Windows.h>
#include <cstdio>
#include <string>
#include <cstdarg>

#include "CommonConstants.h"
using namespace PredCommon;

static bool g_LoggerInitialized = false;
static NamedPipeServer* g_LoggerPipe = nullptr;
static CRITICAL_SECTION g_LogLock;

bool InitializeLogger()
{
    InitializeCriticalSection(&g_LogLock);

    g_LoggerInitialized = true;
    return true;
}

void ShutdownLogger()
{
    if (!g_LoggerInitialized) return;

    DeleteCriticalSection(&g_LogLock);
    g_LoggerInitialized = false;
}

void SetLoggerPipe(NamedPipeServer* pipe)
{
    EnterCriticalSection(&g_LogLock);
    g_LoggerPipe = pipe;
    LeaveCriticalSection(&g_LogLock);
}

void LogPrintf(const char* prefix, const char* fmt, va_list args)
{
    if (!g_LoggerInitialized) return;

    char buffer[1024];
    vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);

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

void LogInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [INF] ", fmt, args);
    va_end(args);
}

void LogError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [ERR] ", fmt, args);
    va_end(args);
}

void LogWarning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [WRN] ", fmt, args);
    va_end(args);
}

void LogDebug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogPrintf("[LOG] [DBG] ", fmt, args);
    va_end(args);
}
