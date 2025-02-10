#pragma once
#include <cstdarg>
#include "NamedPipeServer.h"

bool InitializeLogger();
void ShutdownLogger();
void SetLoggerPipe(PredCommon::NamedPipeServer* pipe);
void LogPrintf(const char* prefix, const char* fmt, va_list args);

void LogInfo(const char* fmt, ...);
void LogError(const char* fmt, ...);
void LogWarning(const char* fmt, ...);
void LogDebug(const char* fmt, ...);
