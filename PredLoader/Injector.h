#pragma once
#include <string>

namespace Injector
{
    bool InjectDLL(unsigned long processId, const std::string& dllPath);
}
