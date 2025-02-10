#include "NamedPipeServer.h"
#include <iostream>

namespace PredCommon
{
    NamedPipeServer::NamedPipeServer()
        : m_Pipe(INVALID_HANDLE_VALUE),
          m_Disconnecting(false)
    {
    }

    NamedPipeServer::~NamedPipeServer()
    {
        Disconnect();
    }

    bool NamedPipeServer::Create(const char* pipeName)
    {
        // Create a single-instance named pipe
        m_Pipe = CreateNamedPipeA(
            pipeName,
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1, // max instances
            4096, // out buffer size
            4096, // in buffer size
            0, // default timeout
            nullptr
        );

        if (m_Pipe == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[NamedPipeServer] CreateNamedPipe failed. Error: " << GetLastError() << "\n";
            return false;
        }

        return true;
    }

    bool NamedPipeServer::WaitForClient(DWORD timeout_ms)
    {
        if (!m_Pipe || m_Pipe == INVALID_HANDLE_VALUE)
            return false;

        // Use ConnectNamedPipe with overlapped I/O for timeout support
        OVERLAPPED ovl = {0};
        ovl.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (!ovl.hEvent)
            return false;

        if (!ConnectNamedPipe(m_Pipe, &ovl))
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                CloseHandle(ovl.hEvent);
                return false;
            }
        }

        // Wait for connection or timeout
        DWORD result = WaitForSingleObject(ovl.hEvent, timeout_ms);
        CloseHandle(ovl.hEvent);

        if (result == WAIT_TIMEOUT)
        {
            CancelIo(m_Pipe);
            return false;
        }

        DWORD bytes;
        return GetOverlappedResult(m_Pipe, &ovl, &bytes, FALSE);
    }

    bool NamedPipeServer::IsConnected() const
    {
        return (m_Pipe != INVALID_HANDLE_VALUE);
    }

    void NamedPipeServer::Disconnect()
    {
        if (m_Disconnecting.exchange(true))
        {
            return;
        }

        if (m_Pipe != INVALID_HANDLE_VALUE)
        {
            // Cancel any pending I/O
            CancelIo(m_Pipe);

            // Flush data
            FlushFileBuffers(m_Pipe);

            // Disconnect server side
            DisconnectNamedPipe(m_Pipe);

            // Close handle
            CloseHandle(m_Pipe);
            m_Pipe = INVALID_HANDLE_VALUE;
        }

        m_Disconnecting = false;
    }

    bool NamedPipeServer::SendMessageToClient(MessageType type, const std::string& text)
    {
        if (!IsConnected()) return false;

        struct PacketHeader
        {
            uint8_t msgType;
            uint32_t length;
        } header;

        header.msgType = static_cast<uint8_t>(type);
        header.length = static_cast<uint32_t>(text.size());

        DWORD bytesWritten = 0;

        // Write header
        bool ok = WriteFile(m_Pipe, &header, sizeof(header), &bytesWritten, nullptr);
        if (!ok || bytesWritten != sizeof(header))
        {
            return false;
        }

        // Write payload
        if (!text.empty())
        {
            ok = WriteFile(m_Pipe, text.data(), header.length, &bytesWritten, nullptr);
            if (!ok || bytesWritten != header.length)
            {
                return false;
            }
        }

        FlushFileBuffers(m_Pipe);
        return true;
    }
}
