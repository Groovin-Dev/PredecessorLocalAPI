#pragma once
#include <cstdarg>  // for va_list
#include "NamedPipeServer.h"

// Initialize the logger (optionally create console if needed).
// This does not connect to a pipe yet; call SetLoggerPipe later once the pipe is ready.
bool InitializeLogger();

// Shutdown the logger (free console, etc.)
void ShutdownLogger();

// After the pipe is connected, set it here so logs also go to the pipe.
void SetLoggerPipe(PredCommon::NamedPipeServer* pipe);

// The main logging function with printf-style arguments
void LogPrintf(const char* prefix, const char* fmt, va_list args);

// Convenience wrappers
void LogInfo(const char* fmt, ...);
void LogError(const char* fmt, ...);
