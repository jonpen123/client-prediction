#include <cmath>

#include "raylib-cpp.hpp"

#include "Client.h"
#include "LocalPlayer.h"

int main() {
    int screenWidth = 1024;
    int screenHeight = 768;
    raylib::Window window(screenWidth, screenHeight, "raylib [core] example - basic window");

    raylib::Camera2D camera;

    Client client;
    LocalPlayer player;

    player.client = &client;
    client.localPlayer = &player;

    if (client.init()) return 1;

    client.connect("localhost", 1234);

    player.init(&camera);

    //SetTargetFPS(60);

    const float DESIRED_FPS = FPS;
    const float DESIRED_FRAMETIME = 1.0f / DESIRED_FPS;
    const int MAX_FRAMESKIP = 5;

    auto lastUpdate = std::chrono::steady_clock::now();
    float lag = 0.0f;

    while (!window.ShouldClose()) {   // Detect window close button or ESC key

        auto now = std::chrono::steady_clock::now();
        float frameTime = (float)std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
        lastUpdate = now;

        lag += frameTime;

        client.update();

        int loops = 0;
        while (lag >= DESIRED_FRAMETIME && loops < MAX_FRAMESKIP)
        {
            if (client.tickDiff > client.ticksAhead) {
                // Ako clientova simulacija ide puno brze od servera
                // nemoj updateat igru dok se tickDiff ne smanji (pricekaj server)
                lag -= DESIRED_FRAMETIME;
                break;
            }
            
            int catchUp = 1;
            if (client.tickDiff < client.ticksAhead) {
                // Ako je clientova simulacija u proslosti
                // updateaj igru vise puta dok simulacija ne bude ponovo u buducnosti
                catchUp = client.ticksAhead - client.tickDiff;
            }

            while (catchUp)
            {
                player.update(client.currentTick);
                client.currentTick++;
                lag -= DESIRED_FRAMETIME;
                loops++;
                catchUp--;
            }
        }

        BeginDrawing();
        {
            window.ClearBackground(BLACK);

            BeginMode2D(*player.camera);

            for (auto p : client.players)
            {
                p->draw();
            }
            player.draw();

            EndMode2D();

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    client.disconnect();
    client.destroy();

    return 0;
}