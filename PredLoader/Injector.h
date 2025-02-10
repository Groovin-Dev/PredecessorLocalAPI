#pragma once
#include <string>

namespace Injector
{
    // Injects a DLL into the specified process by PID, returns true on success
    bool InjectDLL(unsigned long processId, const std::string& dllPath);
}
