#include "Server.h"
#include "enet/enet.h"
#include "nlohmann/json.hpp"
#include "Player.h"

#include <iostream>

using nlohmann::json;

int Server::init(unsigned int _port)
{
    if (enet_initialize() != 0)
    {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }
    atexit(enet_deinitialize);

    ENetAddress address;

    port = _port;

    address.host = ENET_HOST_ANY;
    address.port = port;
    server = enet_host_create(&address, 20, 2, 0, 0);
    if (!server)
    {
        fprintf(stderr,
            "An error occurred while trying to create an ENet server host.\n");
        return EXIT_FAILURE;
    }

    running = true;

    return EXIT_SUCCESS;
}

void Server::update()
{
    ENetEvent event;

    while (running)
    {
        // Ovo sve za fake lag
        auto now = std::chrono::steady_clock::now();
        for (auto it = receivedPackets.begin(); it != receivedPackets.end();)
        {
            ClientPacket* packet = *it;
            float timeDiff = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - packet->timeReceived).count();
            if (timeDiff > latency) {
                
                Player* player = static_cast<Player*>(packet->peer->data);

                json data = json::parse(packet->packet);

                if (data["type"] == "input") {
                    player->input.emplace((int)data["tick"], (std::string)data["input"]);
                }
                else if (data["type"] == "ping") {
                    json reply;
                    reply["type"] = "ping";
                    send(reply.dump(), packet->peer);
                }

                delete packet;

                it = receivedPackets.erase(it);
            }
            else {
                it++;
            }
        }

        while (enet_host_service(server, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
            {
                printf("A new client connected from %x:%u.\n",
                    (unsigned int)event.peer->address.host,
                    (unsigned int)event.peer->address.port);
                players.emplace_back(new Player());
                event.peer->data = players.back();
                players.back()->id = lastFreeId;

                // Send id to client
                json welcome;
                welcome["type"] = "welcome";
                welcome["id"] = lastFreeId;
                // Send already joined clients to new client
                for (auto player : players)
                {
                    if (player->id == players.back()->id) continue;

                    welcome["lobby"].array().push_back("player" + player->id);
                    welcome["lobby"]["player" + player->id]["id"] = player->id;
                    welcome["lobby"]["player" + player->id]["x"] = player->position.x;
                    welcome["lobby"]["player" + player->id]["y"] = player->position.y;
                }
                welcome["tick"] = currentTick;

                send(welcome.dump(), event.peer);

                // Send a ping packet to new slient
                json ping;
                ping["type"] = "ping";
                send(ping.dump(), event.peer);

                json msg;
                msg["type"] = "joined";
                msg["id"] = lastFreeId;
                msg["x"] = players.back()->position.x;
                msg["y"] = players.back()->position.y;

                // Broadcast joined client (except to the client that joined)
                for (size_t i = 0; i < server->connectedPeers; i++)
                {
                    if (server->peers + i == event.peer) continue;

                    send(msg.dump(), server->peers + i);
                }

                lastFreeId++;
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                /*
                printf("A packet of length %u containing %s was received from %s on channel %u.\n",
                    (unsigned int)event.packet->dataLength,
                    (char*)event.packet->data,
                    (char*)event.peer->data,
                    (unsigned int)event.channelID);
                */
                /*
                Player* player = static_cast<Player*>(event.peer->data);

                json data = json::parse((char*)event.packet->data);

                if (data["type"] == "input") {
                    player->input.emplace((int)data["tick"], (std::string)data["input"]);
                }
                else if (data["type"] == "ping") {
                    json reply;
                    reply["type"] = "ping";
                    send(reply.dump(), event.peer);
                }
                */

                ClientPacket* p = new ClientPacket();
                p->peer = event.peer;
                p->timeReceived = std::chrono::steady_clock::now();
                p->packet = (char*)event.packet->data;
                receivedPackets.push_back(p);

                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                printf("%s disconnected.\n", (char*)event.peer->data);
                event.peer->data = nullptr;
            }
            }
        }
    }
}

void Server::destroy()
{
    running = false;

    enet_host_destroy(server);

    for (auto player : players)
    {
        delete player;
    }
}

void Server::broadcast(std::string msg)
{
    ENetPacket* packet = enet_packet_create(msg.c_str(), strlen(msg.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

void Server::send(std::string msg, ENetPeer* peer)
{
    ENetPacket* packet = enet_packet_create(msg.c_str(), strlen(msg.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 0, packet);
}