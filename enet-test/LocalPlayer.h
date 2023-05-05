#pragma once

#include "Player.h"

#include <string>

class Client;

namespace raylib {
	class Camera2D;
}

class LocalPlayer : public Player
{
public:
	LocalPlayer() {};
	~LocalPlayer() {};

	void init(raylib::Camera2D*);
	void update(int currentTick);
	void draw() override;

	void movement(std::string&, Position&);

	raylib::Camera2D* camera = nullptr;

	Client* client = nullptr;
};

