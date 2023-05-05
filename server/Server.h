#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <map>

typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

class Player;

class ClientPacket
{
public:
	ClientPacket() {};
	~ClientPacket() {};

	ENetPeer* peer = nullptr;
	std::chrono::time_point<std::chrono::steady_clock> timeReceived;
	std::string packet;
};

class Server
{
public:
	Server() {};
	~Server() {};

	int init(unsigned int _port);
	void update();
	void destroy();

	void broadcast(std::string);
	void send(std::string, ENetPeer*);

	unsigned int port = 1234;

	std::vector<Player*> players;

	int currentTick = 0;

private:
	ENetHost* server = nullptr;

	int lastFreeId = 0;

	bool running = false;

	// Ovo sve za fakeat ping
	int latency = 150;
	std::vector<ClientPacket*> receivedPackets;
};
