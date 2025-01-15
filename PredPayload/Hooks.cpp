#include "Hooks.h"

#include "KPE_HookLib/KEI_PE_HOOK.hpp"

#include "Logging.h"

#include "SDK/Predecessor_classes.hpp"
#include "SDK/Predecessor_parameters.hpp"

#include "WebSocketServer.h"
extern std::unique_ptr<WebSocketServer> g_WebSocketServer;

#include <nlohmann/json.hpp>
using json = nlohmann::json;

using namespace SDK;
using namespace SDK::Params;

bool OnTryBuyItem(UObject* Object, UFunction* Function, void* Parms)
{
    auto InventoryComponent = static_cast<SDK::UPredInventoryComponent*>(Object);
    if (InventoryComponent && InventoryComponent->GetOwner())
    {
        if (auto PlayerState = static_cast<SDK::ABasePlayerState*>(InventoryComponent->GetOwner()))
        {
            std::string playerName = PlayerState->GetPlayerName().ToString();
            auto params = static_cast<SDK::Params::PredInventoryComponent_Server_TryBuyItem*>(Parms);
            std::string itemId = std::to_string(reinterpret_cast<uintptr_t>(params->Item));

            // Log to file
            LogInfo("PURCHASE_ATTEMPT:%s:%s", playerName.c_str(), itemId.c_str());

            // Send to websocket
            json data = {
                {"player_name", playerName},
                {"item_id", itemId}
            };
            g_WebSocketServer->BroadcastEvent("purchase_attempt", data);
        }
    }
    return true; // proceed
}

void InstallHooks()
{
    KPE::Enable();
    KPE_AddHook("Function Predecessor.PredInventoryComponent.Server_TryBuyItem", { return OnTryBuyItem(Object, Function, Parms); });
    LogInfo("Hooks installed.");
}

void RemoveHooks()
{
    // If you want to explicitly remove hooks or restore them, do so here
    KPE::Disable();
    LogInfo("Hooks removed.");
}
