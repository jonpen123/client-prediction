#include "World.h"
#include "Server.h"
#include "Player.h"
#include "nlohmann/json.hpp"

#include <iostream>

using nlohmann::json;

void World::update(float deltaTime, int currentTick)
{
    json newState;
    newState["type"] = "state";
    newState["tick"] = currentTick;

    for (auto player : server->players)
    {
        for (auto it = player->input.begin(); it != player->input.end();)
        {
            if (it->second == "0000" || it->first != currentTick) {
                if (it->first < currentTick) {
                    // Drop the input change request
                    it = player->input.erase(it);
                }
                else {
                    it++;
                }
                continue;
            }

            if (it->second[0] == '1') {
                player->position.y -= player->moveSpeed * deltaTime;
            }
            else if (it->second[2] == '1') {
                player->position.y += player->moveSpeed * deltaTime;
            }
            if (it->second[1] == '1') {
                player->position.x -= player->moveSpeed * deltaTime;
            }
            else if (it->second[3] == '1') {
                player->position.x += player->moveSpeed * deltaTime;
            }

            newState["lobby"]["player" + player->id]["id"] = player->id;
            newState["lobby"]["player" + player->id]["x"] = player->position.x;
            newState["lobby"]["player" + player->id]["y"] = player->position.y;

            it = player->input.erase(it);
        }
    }

    server->broadcast(newState.dump());
}