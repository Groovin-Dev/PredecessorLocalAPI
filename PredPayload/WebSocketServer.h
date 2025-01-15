#pragma once
#include <string>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();

    bool Start(uint16_t port = 9090);
    void Stop();
    void BroadcastEvent(const std::string& eventName, const json& data);

private:
    struct ServerImpl;
    std::unique_ptr<ServerImpl> m_impl;
    bool m_running;
}; 