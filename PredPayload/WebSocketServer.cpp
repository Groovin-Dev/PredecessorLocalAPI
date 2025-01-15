#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN

#include "WebSocketServer.h"
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <thread>
#include <mutex>
#include "Logging.h"

using websocketpp::connection_hdl;
using Server = websocketpp::server<websocketpp::config::asio>;

struct WebSocketServer::ServerImpl
{
    Server server;
    std::thread serverThread;
    std::set<connection_hdl, std::owner_less<connection_hdl>> connections;
    std::mutex connectionsMutex;
    bool running;
};

WebSocketServer::WebSocketServer()
    : m_impl(std::make_unique<ServerImpl>())
      , m_running(false)
{
    // Set logging settings
    m_impl->server.set_access_channels(websocketpp::log::alevel::none);
    m_impl->server.set_error_channels(websocketpp::log::elevel::none);

    // Initialize ASIO
    m_impl->server.init_asio();

    // Register handlers
    m_impl->server.set_open_handler([this](connection_hdl hdl)
    {
        std::lock_guard<std::mutex> lock(m_impl->connectionsMutex);
        m_impl->connections.insert(hdl);
        LogInfo("[WebSocket] Client connected. Total clients: %zu", m_impl->connections.size());
    });

    m_impl->server.set_close_handler([this](connection_hdl hdl)
    {
        std::lock_guard<std::mutex> lock(m_impl->connectionsMutex);
        m_impl->connections.erase(hdl);
        LogInfo("[WebSocket] Client disconnected. Total clients: %zu", m_impl->connections.size());
    });
}

WebSocketServer::~WebSocketServer()
{
    Stop();
}

bool WebSocketServer::Start(uint16_t port)
{
    if (m_running) return false;

    try
    {
        m_impl->server.listen(port);
        m_impl->server.start_accept();

        // Run the ASIO io_service loop in a separate thread
        m_impl->serverThread = std::thread([this]()
        {
            try
            {
                m_impl->server.run();
            }
            catch (const std::exception& e)
            {
                LogError("[WebSocket] Server error: %s", e.what());
            }
        });

        m_running = true;
        LogInfo("[WebSocket] Server started on port %d", port);
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("[WebSocket] Failed to start server: %s", e.what());
        return false;
    }
}

void WebSocketServer::Stop()
{
    if (!m_running) return;

    try
    {
        // Stop the server
        m_impl->server.stop();

        // Close all connections
        {
            std::lock_guard<std::mutex> lock(m_impl->connectionsMutex);
            for (auto& hdl : m_impl->connections)
            {
                m_impl->server.close(hdl, websocketpp::close::status::going_away, "Server shutdown");
            }
            m_impl->connections.clear();
        }

        // Join the server thread
        if (m_impl->serverThread.joinable())
        {
            m_impl->serverThread.join();
        }

        m_running = false;
        LogInfo("[WebSocket] Server stopped");
    }
    catch (const std::exception& e)
    {
        LogError("[WebSocket] Error during shutdown: %s", e.what());
    }
}

void WebSocketServer::BroadcastEvent(const std::string& eventName, const json& data)
{
    if (!m_running) return;

    try
    {
        json message = {
            {"event", eventName},
            {"data", data}
        };

        std::string messageStr = message.dump();

        std::lock_guard<std::mutex> lock(m_impl->connectionsMutex);
        for (auto& hdl : m_impl->connections)
        {
            try
            {
                m_impl->server.send(hdl, messageStr, websocketpp::frame::opcode::text);
            }
            catch (const std::exception& e)
            {
                LogError("[WebSocket] Error sending to client: %s", e.what());
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("[WebSocket] Error broadcasting event: %s", e.what());
    }
}
