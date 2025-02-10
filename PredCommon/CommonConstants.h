#pragma once

#include <cstdint>
#include <string>

namespace PredCommon
{
    inline constexpr auto PIPE_NAME = R"(\\.\pipe\PredecessorLocalAPIPipe)";

    enum class MessageType : uint8_t
    {
        UNLOAD = 1,
        LOG = 2,
        OTHER = 3
    };

    inline const char* MessageTypeToString(MessageType type)
    {
        switch (type)
        {
        case MessageType::UNLOAD: return "UNLOAD";
        case MessageType::LOG: return "LOG";
        case MessageType::OTHER: return "OTHER";
        default: return "UNKNOWN";
        }
    }

    inline constexpr bool DEBUG_CONSOLE = true;
}
