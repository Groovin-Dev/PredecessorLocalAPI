#include "GameStateManager.h"
#include "Logging.h"
#include "WebSocketServer.h"

extern std::shared_ptr<WebSocketServer> g_WebSocketServer;

GameStateManager::GameStateManager()
{
}

bool GameStateManager::Update()
{
    if (!g_WebSocketServer) {
        LogWarning("[GameState] WebSocket server not available");
        return false;
    }

    try {
        json gameState = GetCurrentGameState();
        g_WebSocketServer->BroadcastEvent("game_state", gameState);
        return true;
    }
    catch (const std::exception& e) {
        LogError("[GameState] Error updating game state: %s", e.what());
        return false;
    }
}

json GameStateManager::GetCurrentGameState()
{
    json gameState = {
        {"players", json::array()}
    };

    SDK::UWorld* world = SDK::UWorld::GetWorld();
    if (!world) {
        LogWarning("[GameState] Failed to get World");
        return gameState;
    }

    SDK::APredGameState* predGameState = static_cast<SDK::APredGameState*>(world->GameState);
    if (!predGameState) {
        LogWarning("[GameState] Failed to get PredGameState");
        return gameState;
    }

    SDK::UPredGameStateGameComponent* predGameComponent = static_cast<SDK::UPredGameStateGameComponent*>(predGameState->PredGameStateComponent);
    if (!predGameComponent) {
        LogWarning("[GameState] Failed to get PredGameStateComponent");
        return gameState;
    }

    // Iterate through all player states
    SDK::TArray<SDK::APlayerState*>& playerArray = predGameState->PlayerArray;

    // If there is only one player, we are probably in the lobby
    if (playerArray.Num() <= 1) {
        LogDebug("[GameState] Only one or no players found, likely in lobby");
        return gameState;
    }

    for (int32_t i = 0; i < playerArray.Num(); i++)
    {
        try {
            auto playerState = static_cast<SDK::ABasePlayerState*>(playerArray[i]);
            if (!playerState) {
                LogWarning("[GameState] Invalid player state at index %d", i);
                continue;
            }

            json playerData = GetPlayerState(playerState);
            gameState["players"].push_back(playerData);
        }
        catch (const std::exception& e) {
            LogError("[GameState] Error processing player at index %d: %s", i, e.what());
            continue;
        }
    }

    return gameState;
}

json GameStateManager::GetPlayerState(SDK::ABasePlayerState* playerState)
{
    json playerData = {
        {"name", "Unknown"},
        {"team", -1},
        {"level", 0},
        {"gold", 0},
        {"kills", 0},
        {"deaths", 0},
        {"assists", 0},
        {"minion_kills", 0},
        {"bounty", 0},
        {"inventory", json::array()},
        {"spells", {
            {"flash_ready", false},
            {"ultimate", {
                {"ultimate_unlocked", false},
                {"ultimate_ready", false}
            }}
        }}
    };

    if (!playerState) {
        LogWarning("[GameState] Null player state passed to GetPlayerState");
        return playerData;
    }

    try {
        // Basic player info
        playerData["name"] = playerState->PlayerNamePrivate.ToString();
        playerData["team"] = playerState->GetTeamId();
        playerData["level"] = playerState->GetNumericalLevel();
        playerData["gold"] = playerState->TotalGold;
        playerData["kills"] = playerState->HeroKills;
        playerData["deaths"] = playerState->Deaths;
        playerData["assists"] = playerState->Assists;
        playerData["minion_kills"] = playerState->MinionKills;
        playerData["bounty"] = playerState->CurrentBounty;

        // Inventory
        if (playerState->InventoryComponent) {
            playerData["inventory"] = GetPlayerItems(playerState->InventoryComponent);
        }
        else {
            LogWarning("[GameState] Player %s has no inventory component", 
                playerState->PlayerNamePrivate.ToString().c_str());
        }

        // Spells
        if (playerState->bIsUltimateUnlocked) {
            playerData["spells"]["ultimate"]["ultimate_unlocked"] = true;
        }

    }
    catch (const std::exception& e) {
        LogError("[GameState] Error getting player state for %s: %s", 
            playerState->PlayerNamePrivate.ToString().c_str(), e.what());
    }

    return playerData;
}

json GameStateManager::GetPlayerItems(SDK::UPredInventoryComponent* inventoryComponent)
{
    json items = json::array();

    if (!inventoryComponent) {
        LogWarning("[GameState] Null inventory component passed to GetPlayerItems");
        return items;
    }

    try {
        for (const auto& inventorySlot : inventoryComponent->Inventory)
        {
            if (!inventorySlot.SlottedItem.Item) {
                continue;  // Skip empty slots
            }

            items.push_back({
                {"slot_id", inventorySlot.SlotID},
                {"slot_type", GetItemSlotTypeString(inventorySlot.SlotType)},
                {"item", {
                    {"item_id", inventorySlot.SlottedItem.Item->ItemId},
                    {"item_name", inventorySlot.SlottedItem.Item->ItemName.ToString()},
                }}
            });
        }
    }
    catch (const std::exception& e) {
        LogError("[GameState] Error processing inventory items: %s", e.what());
    }

    return items;
}

std::string GameStateManager::GetItemSlotTypeString(SDK::EPredItemSlotType slotType)
{
    try {
        switch (slotType)
        {
            case SDK::EPredItemSlotType::Passive: return "Passive";
            case SDK::EPredItemSlotType::Active: return "Active";
            case SDK::EPredItemSlotType::Crest: return "Crest";
            case SDK::EPredItemSlotType::Trinket: return "Trinket";
            case SDK::EPredItemSlotType::EPredItemSlotType_MAX: return "Max";
            default: return "Unknown";
        }
    }
    catch (...) {
        LogError("[GameState] Error converting slot type to string");
        return "Error";
    }
}


bool GameStateManager::IsAbilityOnCooldown(/* todo: figure this out */)
{
    return true;
}