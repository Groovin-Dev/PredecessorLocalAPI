#pragma once

#include <cstdint>
#include <string>

namespace PredCommon
{
    // Named pipe path (adjust if desired)
    inline constexpr const char* PIPE_NAME = R"(\\.\pipe\PredecessorLocalAPIPipe)";

    // Example message types; adjust as needed
    enum class MessageType : uint8_t
    {
        UNLOAD = 1,
        LOG = 2,
        OTHER = 3
    };

    // Helper to convert message types to a string (for debugging/logging)
    inline const char* MessageTypeToString(MessageType type)
    {
        switch (type)
        {
        case MessageType::UNLOAD: return "UNLOAD";
        case MessageType::LOG:    return "LOG";
        case MessageType::OTHER:  return "OTHER";
        default:                  return "UNKNOWN";
        }
    }

    // Is the debug console enabled
    inline const bool DEBUG_CONSOLE = true;
}
