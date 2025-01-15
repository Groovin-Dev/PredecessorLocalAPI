#pragma once

#include <Windows.h>
#include <string>
#include <atomic>
#include "CommonConstants.h"

namespace PredCommon
{
    class NamedPipeServer
    {
    public:
        NamedPipeServer();
        ~NamedPipeServer();

        // Disable copy
        NamedPipeServer(const NamedPipeServer&) = delete;
        NamedPipeServer& operator=(const NamedPipeServer&) = delete;

        // Create pipe and prepare to accept one client
        bool Create(const char* pipeName);

        // Block until a client connects or fails
        bool WaitForClient(DWORD timeout_ms = INFINITE);

        // Check if connected
        bool IsConnected() const;

        // Close or disconnect the pipe
        void Disconnect();

        // Send a message to the connected client
        bool SendMessageToClient(MessageType type, const std::string& text);

    private:
        HANDLE m_Pipe;
        std::atomic<bool> m_Disconnecting;
    };
}
