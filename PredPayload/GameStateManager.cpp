#include <format>

#include "GameStateManager.h"
#include "Logging.h"
#include "WebSocketServer.h"

extern std::shared_ptr<WebSocketServer> g_WebSocketServer;

GameStateManager::GameStateManager()
{
}

bool GameStateManager::Update()
{
    if (!g_WebSocketServer)
    {
        LogWarning("[GameState] WebSocket server not available");
        return false;
    }

    try
    {
        const json gameState = GetCurrentGameState();
        g_WebSocketServer->BroadcastEvent("game_state", gameState);
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("[GameState] Error updating game state: %s", e.what());
        return false;
    }
}

json GameStateManager::GetCurrentGameState()
{
    json gameState = {
        {"players", json::array()}
    };

    try
    {
        const SDK::UWorld* world = SDK::UWorld::GetWorld();
        if (!world)
        {
            LogWarning("[GameState] Failed to get World");
            return gameState;
        }

        const auto gameMode = world->AuthorityGameMode;
        if (!gameMode)
        {
            LogWarning("[GameState] Failed to get gameMode");
            return gameState;
        }

        LogDebug("[GameState] gameMode full name: %s", gameMode->GetFullName().c_str());

        const auto predGameState = static_cast<SDK::APredGameState*>(world->GameState);
        if (!predGameState)
        {
            LogWarning("[GameState] Failed to get PredGameState");
            return gameState;
        }

        const auto predGameComponent = static_cast<SDK::UPredGameStateGameComponent*>(predGameState->
            PredGameStateComponent);
        if (!predGameComponent)
        {
            LogWarning("[GameState] Failed to get PredGameStateComponent");
            return gameState;
        }

        if (!predGameComponent->GameTime)
        {
            LogWarning("[GameState] Failed to get GameTime. Probably not in game");
            return gameState;
        }

        int minutes = predGameComponent->GameTime / 60;
        int seconds = predGameComponent->GameTime % 60;
        const std::string gameTime = std::format("{:02d}:{:02d}", minutes, seconds);
        LogDebug("[GameState] Game Time: %s", gameTime.c_str());
        LogDebug("[GameState] Size of all heroes: %d", predGameComponent->AllHeroes.Num());

        for (int32_t i = 0; i < predGameComponent->AllHeroes.Num(); i++)
        {
            try
            {
                const auto character = static_cast<SDK::APredCharacter*>(predGameComponent->AllHeroes[i]);
                if (!character)
                {
                    LogWarning("[GameState] Invalid hero at index %d", i);
                    continue;
                }

                json playerData = GetPlayerState(character);
                gameState["players"].push_back(playerData);
            }
            catch (const std::exception& e)
            {
                LogError("[GameState] Error processing hero at index %d: %s", i, e.what());
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("[GameState] Error retrieving current game state: %s", e.what());
    }

    return gameState;
}

json GameStateManager::GetPlayerState(SDK::APredCharacter* character)
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
        {
            "spells", {
                {"flash_ready", false},
                {
                    "ultimate", {
                        {"ultimate_unlocked", false},
                        {"ultimate_ready", false}
                    }
                }
            }
        }
    };

    if (!character)
    {
        LogWarning("[GameState] Null hero passed to GetPlayerState");
        return playerData;
    }

    try
    {
        const auto playerState = character->PredPlayerState;
        if (!playerState)
        {
            LogWarning("[GameState] No player state found for hero");
            return playerData;
        }

        SDK::UPredAttributeSet_Progression* attributeSet = character->AttributeSet_Progression;
        if (!attributeSet)
        {
            LogWarning("[GameState] No progression attribute set found for player %s",
                       playerState->PlayerNamePrivate.ToString().c_str());
            return playerData;
        }

        playerData["name"] = playerState->PlayerNamePrivate.ToString();
        playerData["team"] = playerState->GetTeamId();
        playerData["level"] = playerState->GetNumericalLevel();
        playerData["gold"] = attributeSet->Gold.CurrentValue;
        playerData["kills"] = playerState->HeroKills;
        playerData["deaths"] = playerState->Deaths;
        playerData["assists"] = playerState->Assists;
        playerData["minion_kills"] = playerState->MinionKills;
        playerData["bounty"] = playerState->CurrentBounty;

        if (playerState->InventoryComponent)
        {
            playerData["inventory"] = GetPlayerItems(playerState->InventoryComponent);
        }
        else
        {
            LogWarning("[GameState] Player %s has no inventory component",
                       playerState->PlayerNamePrivate.ToString().c_str());
        }

        if (playerState->bIsUltimateUnlocked)
        {
            playerData["spells"]["ultimate"]["ultimate_unlocked"] = true;
        }
    }
    catch (const std::exception& e)
    {
        LogError("[GameState] Error getting player state: %s", e.what());
    }

    return playerData;
}

json GameStateManager::GetPlayerItems(SDK::UPredInventoryComponent* inventoryComponent)
{
    json items = json::array();

    if (!inventoryComponent)
    {
        LogWarning("[GameState] Null inventory component passed to GetPlayerItems");
        return items;
    }

    try
    {
        for (const auto& inventorySlot : inventoryComponent->Inventory)
        {
            if (!inventorySlot.SlottedItem.Item)
            {
                continue;
            }

            items.push_back({
                {"slot_id", inventorySlot.SlotID},
                {"slot_type", GetItemSlotTypeString(inventorySlot.SlotType)},
                {
                    "item", {
                        {"item_id", inventorySlot.SlottedItem.Item->ItemId},
                        {"item_name", inventorySlot.SlottedItem.Item->ItemName.ToString()},
                    }
                }
            });
        }
    }
    catch (const std::exception& e)
    {
        LogError("[GameState] Error processing inventory items: %s", e.what());
    }

    return items;
}

std::string GameStateManager::GetItemSlotTypeString(SDK::EPredItemSlotType slotType)
{
    try
    {
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
    catch (...)
    {
        LogError("[GameState] Error converting slot type to string");
        return "Error";
    }
}

bool GameStateManager::IsAbilityOnCooldown(/* todo: figure this out */)
{
    return true;
}
