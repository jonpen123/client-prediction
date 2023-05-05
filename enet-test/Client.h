#pragma once

#include "Types.h"

#include <string>
#include <vector>
#include <map>
#include <chrono>

// FPS "physics" simulacije
#define FPS 30.0f

typedef struct _ENetHost ENetHost;
typedef struct _ENetPeer ENetPeer;

class Player;

class Client
{
public:
	Client() {};
	~Client() {};

	int init();
	void update();
	void destroy();

	int connect(const char* ip, const unsigned int port);
	void disconnect();

	void send(std::string);

	Player* localPlayer = nullptr;

	std::vector<Player*> players;

	int currentTick = 0;

	int tickDiff = 0;
	int ticksAhead = 13;

	std::map<int, std::string> inputHistory;

	float ping;		// in ms

private:
	int calculatePing(float pingTime);

	ENetHost* client = nullptr;
	ENetPeer* peer = nullptr;

	Player* getPlayer(int id);

	std::chrono::time_point<std::chrono::steady_clock> pingSent;
};

