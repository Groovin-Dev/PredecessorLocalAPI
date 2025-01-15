#include "Hooks.h"

#include "KPE_HookLib/KEI_PE_HOOK.hpp"

#include "Logging.h"

#include "SDK/Predecessor_classes.hpp"
#include "SDK/Predecessor_parameters.hpp"

using namespace SDK;
using namespace SDK::Params;

void InstallHooks()
{
    // Enable hooking system
    KPE::Enable();

    // 1) Hook: Server_TryBuyItem
    KPE_AddHook("Function Predecessor.PredInventoryComponent.Server_TryBuyItem", {
        auto InventoryComponent = static_cast<SDK::UPredInventoryComponent*>(Object);
        if (InventoryComponent && InventoryComponent->GetOwner())
        {
            if (auto PlayerState = static_cast<SDK::ABasePlayerState*>(InventoryComponent->GetOwner()))
            {
                std::string playerName = PlayerState->GetPlayerName().ToString();
                auto params = static_cast<SDK::Params::PredInventoryComponent_Server_TryBuyItem*>(Parms);
                std::string itemId = std::to_string(reinterpret_cast<uintptr_t>(params->Item));

                // Just log it
                LogInfo("PURCHASE_ATTEMPT:%s:%s", playerName.c_str(), itemId.c_str());
            }
        }
        return true; // proceed
        });

    // 2) Hook: Server_TryBuyItemPartsRecursive
    KPE_AddHook("Function Predecessor.PredInventoryComponent.Server_TryBuyItemPartsRecursive", {
        auto InventoryComponent = static_cast<SDK::UPredInventoryComponent*>(Object);
        if (InventoryComponent && InventoryComponent->GetOwner())
        {
            if (auto PlayerState = static_cast<SDK::ABasePlayerState*>(InventoryComponent->GetOwner()))
            {
                std::string playerName = PlayerState->GetPlayerName().ToString();
                auto params = static_cast<SDK::Params::PredInventoryComponent_Server_TryBuyItemPartsRecursive*>(Parms);
                std::string itemId = std::to_string(reinterpret_cast<uintptr_t>(params->Item));

                LogInfo("RECURSIVE_PURCHASE_ATTEMPT:%s:%s", playerName.c_str(), itemId.c_str());
            }
        }
        return true; // proceed
        });

    LogInfo("Hooks installed.");
}

void RemoveHooks()
{
    // If you want to explicitly remove hooks or restore them, do so here
    KPE::Disable();
    LogInfo("Hooks removed.");
}
