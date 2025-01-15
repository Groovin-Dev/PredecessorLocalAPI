#pragma once
#include <string>
#include "NamedPipeClient.h"

class Loader
{
public:
    Loader();
    ~Loader();

    // Initialize loader (find process, inject, connect to pipe)
    bool Init();

    // Main loop - just listen for messages until user quits
    void RunLoop();

private:
    // Internal methods
    bool FindAndInject();
    bool ConnectToPayloadServer();

private:
    std::string m_TargetProcessName;
    std::string m_PayloadDllName;
    unsigned long m_TargetPID;
    bool m_Running;
    PredCommon::NamedPipeClient m_PipeClient;
};
