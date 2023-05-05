#pragma once

#include <string>
#include <map>

typedef struct _Position {
	int x = 0;
	int y = 0;
} Position;

class Player
{
public:
	Player() {};
	~Player() {};

	int id;

	float moveSpeed = 90.0f;

	Position position;

	std::map<int, std::string> input;
};

