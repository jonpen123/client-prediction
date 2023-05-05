#include "Player.h"

#include "raylib-cpp.hpp"

void Player::draw()
{
	DrawCircle(position.x, position.y, 25.0f, BLUE);
}