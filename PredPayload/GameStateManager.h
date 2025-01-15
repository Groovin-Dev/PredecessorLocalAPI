#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

#include "SDK/Predecessor_classes.hpp"

using json = nlohmann::json;

class GameStateManager {
public:
    GameStateManager();
    ~GameStateManager() = default;

    bool Update();

private:
    json GetCurrentGameState();
    json GetPlayerState(SDK::ABasePlayerState* playerState);
    json GetPlayerItems(SDK::UPredInventoryComponent* inventoryComponent);
    std::string GetItemSlotTypeString(SDK::EPredItemSlotType slotType);
    bool IsAbilityOnCooldown();
};