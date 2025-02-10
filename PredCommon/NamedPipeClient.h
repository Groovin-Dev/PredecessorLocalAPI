#pragma once

#include <Windows.h>
#include <string>
#include <atomic>
#include "CommonConstants.h"

namespace PredCommon
{
    class NamedPipeClient
    {
    public:
        NamedPipeClient();
        ~NamedPipeClient();

        // Disable copy
        NamedPipeClient(const NamedPipeClient&) = delete;
        NamedPipeClient& operator=(const NamedPipeClient&) = delete;

        bool Connect(const char* pipeName);
        bool IsConnected() const;
        void Disconnect();
        
        // Only keep the read functionality
        bool ReadMessage(MessageType& outType, std::string& outText);

    private:
        HANDLE m_Pipe;
        std::atomic<bool> m_Disconnecting;
    };
}
