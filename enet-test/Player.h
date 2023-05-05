#pragma once

#include "Types.h"

class Player
{
public:
	Player() {};
	~Player() {};

	virtual void draw();

	int id;

	Position position;

protected:
	float moveSpeed = 90.0f;
};