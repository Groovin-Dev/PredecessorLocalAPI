#include "NamedPipeClient.h"
#include <iostream>

namespace PredCommon
{
    NamedPipeClient::NamedPipeClient()
        : m_Pipe(INVALID_HANDLE_VALUE),
          m_Disconnecting(false)
    {
    }

    NamedPipeClient::~NamedPipeClient()
    {
        Disconnect();
    }

    bool NamedPipeClient::Connect(const char* pipeName)
    {
        // Attempt to open the named pipe
        m_Pipe = CreateFileA(
            pipeName,
            GENERIC_READ | GENERIC_WRITE,
            0, // no sharing
            nullptr,
            OPEN_EXISTING, // must exist
            0, // default attrs
            nullptr
        );

        if (m_Pipe == INVALID_HANDLE_VALUE)
        {
            std::cerr << "[NamedPipeClient] CreateFile failed. Error: " << GetLastError() << "\n";
            return false;
        }

        return true;
    }

    bool NamedPipeClient::IsConnected() const
    {
        return (m_Pipe != INVALID_HANDLE_VALUE);
    }

    void NamedPipeClient::Disconnect()
    {
        if (m_Disconnecting.exchange(true))
            return;

        if (m_Pipe != INVALID_HANDLE_VALUE)
        {
            CancelIo(m_Pipe);
            FlushFileBuffers(m_Pipe);
            CloseHandle(m_Pipe);
            m_Pipe = INVALID_HANDLE_VALUE;
        }

        m_Disconnecting = false;
    }

    bool NamedPipeClient::ReadMessage(MessageType& outType, std::string& outText)
    {
        if (!IsConnected())
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return false;
        }

        struct PacketHeader
        {
            uint8_t msgType;
            uint32_t length;
        } header;

        DWORD bytesRead = 0;

        BOOL ok = ReadFile(m_Pipe, &header, sizeof(header), &bytesRead, nullptr);
        if (!ok || bytesRead != sizeof(header))
        {
            return false;
        }

        outType = static_cast<MessageType>(header.msgType);

        if (header.length > 0)
        {
            outText.resize(header.length);
            bytesRead = 0;
            ok = ReadFile(m_Pipe, &outText[0], header.length, &bytesRead, nullptr);
            if (!ok || bytesRead != header.length)
            {
                outText.clear();
                return false;
            }
        }
        else
        {
            outText.clear();
        }

        return true;
    }
}
