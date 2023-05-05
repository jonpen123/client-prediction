#pragma once

class Server;

class World
{
public:
	World() {};
	~World() {};

	void update(float deltaTime, int currentTick);

	Server* server = nullptr;
};

