#include "LocalPlayer.h"
#include "Client.h"

#include "raylib-cpp.hpp"
#include "nlohmann/json.hpp"

#define FRAME_TIME (1.0f / FPS)

using nlohmann::json;

void LocalPlayer::init(raylib::Camera2D* cam)
{
    camera = cam;

    position.x = 0;
    position.y = 0;

    camera->target = raylib::Vector2(position.x, position.y);;
    camera->offset = raylib::Vector2(1024 / 2.0f, 768 / 2.0f);
    camera->rotation = 0.0f;
    camera->zoom = 1.0f;
}

void LocalPlayer::update(int currentTick)
{
    std::string input = "0000";

    if (IsKeyDown(KEY_W)) {
        input[0] = '1';
    }
    else if (IsKeyDown(KEY_S)) {
        input[2] = '1';
    }
    if (IsKeyDown(KEY_A)) {
        input[1] = '1';
    }
    else if (IsKeyDown(KEY_D)) {
        input[3] = '1';
    }

    json inputs;
    inputs["type"] = "input";       // type je tu nebitan jer jedino saljemo inpute
    inputs["tick"] = currentTick;
    inputs["input"] = input;

    if (input != "0000") {
        movement(input, position);
        client->send(inputs.dump());
        client->inputHistory.emplace(currentTick, input);
    }
}

void LocalPlayer::draw()
{
    DrawCircle(position.x, position.y, 25.0f, RED);
}

void LocalPlayer::movement(std::string& input, Position& posRef)
{
    if (input[0] == '1') {
        posRef.y -= moveSpeed * FRAME_TIME;
    }
    else if (input[2] == '1') {
        posRef.y += moveSpeed * FRAME_TIME;
    }
    if (input[1] == '1') {
        posRef.x -= moveSpeed * FRAME_TIME;
    }
    else if (input[3] == '1') {
        posRef.x += moveSpeed * FRAME_TIME;
    }
}