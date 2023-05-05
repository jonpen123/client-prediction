#include "Client.h"
#include "enet/enet.h"
#include "nlohmann/json.hpp"
#include "Player.h"
#include "LocalPlayer.h"

#include <iostream>

using nlohmann::json;

typedef std::chrono::duration<int, std::milli> millisecconds_type;

int Client::init()
{
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    client = enet_host_create(NULL, 1, 2, 0, 0);
    if (client == NULL)
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet client host.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int Client::connect(const char* ip, const unsigned int port)
{
    ENetAddress address;
    ENetEvent event;

    enet_address_set_host(&address, ip);
    address.port = port;
    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL)
    {
        fprintf(stderr,
            "No available peers for initiating an ENet connection.\n");
        return EXIT_FAILURE;
    }

    pingSent = std::chrono::steady_clock::now();

    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT)
    {
        // Calculate estimated ping
        auto pingReply = std::chrono::steady_clock::now();
        ping = (float)std::chrono::duration_cast<std::chrono::milliseconds>(pingReply - pingSent).count();

        printf("Connection to %s:%u succeeded.\n", ip, port);
        enet_host_flush(client);
    }
    else
    {
        enet_peer_reset(peer);
        printf("Connection to %s:%u failed.\n", ip, port);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Client::update()
{
    ENetEvent event;

    while (enet_host_service(client, &event, 0) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
        {
            json data = json::parse((char*)event.packet->data);

            //puts(data.c_str());

            if (data["type"] == "welcome") {
                // Get id
                localPlayer->id = data["id"];
                // Get players in lobby
                for (auto player : data["lobby"])
                {
                    players.emplace_back(new Player());
                    players.back()->id = player["id"];
                    players.back()->position.x = player["x"];
                    players.back()->position.y = player["y"];
                }
                // Set client's tick ahead of the server
                currentTick = data["tick"] + (ping / 1000.0f) * FPS + ticksAhead;
            }
            else if (data["type"] == "joined") {
                players.emplace_back(new Player());
                players.back()->id = data["id"];
                players.back()->position.x = data["x"];
                players.back()->position.y = data["y"];
            }
            else if (data["type"] == "state") {
                for (auto player : data["lobby"])
                {
                    Player* p = getPlayer(player["id"]);
                    // Save predicted state
                    if (p == localPlayer) {
                        Position tempPos;
                        Position predictedPos;
                        predictedPos = p->position;

                        // Apply authoritative data
                        tempPos.x = player["x"];
                        tempPos.y = player["y"];
                        // Replay any inputs after authoritative data
                        for (auto it = inputHistory.begin(); it != inputHistory.end();)
                        {
                            // Erase every input up to this tick
                            if (it->first <= (int)data["tick"]) {
                                it = inputHistory.erase(it);
                            }
                            else {
                                // Simulate predicted inputs
                                ((LocalPlayer*)localPlayer)->movement(it->second, tempPos);
                                it++;
                            }
                        }
                        // Compare predicted inputs
                        if (tempPos.x == predictedPos.x && tempPos.y == predictedPos.y) {
                            p->position.x = predictedPos.x;
                            p->position.y = predictedPos.y;
                        }
                        else {
                            // If mis-predicted just apply the authoritative state
                            p->position.x = player["x"];
                            p->position.y = player["y"];
                            puts("corrected");
                        }
                    }
                    else {
                        // For other players just set position
                        p->position.x = player["x"];
                        p->position.y = player["y"];
                    }
                }
                tickDiff = currentTick - (int)data["tick"];
            }
            else if (data["type"] == "ping") {
                auto pingReply = std::chrono::steady_clock::now();
                float pingTime = (float)std::chrono::duration_cast<std::chrono::milliseconds>(pingReply - pingSent).count();

                ping = calculatePing(pingTime);
                
                printf("%.f ms\n", ping);

                json ping;
                ping["type"] = "ping";
                send(ping.dump());
                pingSent = std::chrono::steady_clock::now();
            }

            enet_packet_destroy(event.packet);
            break;
        }

        case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconnected.\n", (char*)event.peer->data);
        }
    }
}

void Client::disconnect()
{
    ENetEvent event;
    enet_peer_disconnect(peer, 0);

    while (enet_host_service(client, &event, 3000) > 0)
    {
        switch (event.type)
        {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            puts("Disconnection succeeded.");
            return;
        }
    }

    enet_peer_reset(peer);
}

void Client::destroy()
{
    enet_host_destroy(client);

    for (auto player : players)
    {
        delete player;
    }
}

void Client::send(std::string msg)
{
    ENetPacket* packet = enet_packet_create(msg.c_str(), strlen(msg.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}

Player* Client::getPlayer(int id)
{
    if (id == localPlayer->id)
        return localPlayer;

    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i]->id == id) {
            return players[i];
        }
    }

    return nullptr;
}

int Client::calculatePing(float pingTime)
{
    static const int NUM_SAMPLES = 10;
    static float pingTimes[NUM_SAMPLES];
    static int currentPingIdx = 0;

    pingTimes[currentPingIdx % NUM_SAMPLES] = pingTime;

    if (!currentPingIdx) currentPingIdx++;		// Ovo radimo da sprijecimo overflow natrag na 0
    int count = currentPingIdx < NUM_SAMPLES ? currentPingIdx : NUM_SAMPLES;

    float pingAvg = 0;
    for (size_t i = 0; i < count; i++)
    {
        pingAvg += pingTimes[i];
    }
    pingAvg /= count;

    currentPingIdx++;

    return pingAvg;
}
