#include <iostream>
#include <chrono>
#include <algorithm>
#include <thread>

#include "Server.h"
#include "World.h"

typedef std::chrono::duration<float> seconds_type;

int calculateFPS(float frameTime)
{
	int fps;

	// Mjeri najvise avg od 10 frame timeova
	static const int NUM_SAMPLES = 10;
	static float frameTimes[NUM_SAMPLES];
	static int currentFrame = 0;

	// currentFrame ce se povecavat u nedogled, ali index ce uvijek biti unutar [0, 9]
	// zbog ovog %
	frameTimes[currentFrame % NUM_SAMPLES] = frameTime;

	if (!currentFrame) currentFrame++;		// Ovo radimo da sprijecimo overflow natrag na 0
	int count = currentFrame < NUM_SAMPLES ? currentFrame : NUM_SAMPLES;

	float frameTimeAvg = 0;
	for (size_t i = 0; i < count; i++)
	{
		frameTimeAvg += frameTimes[i];
	}
	frameTimeAvg /= count;

	if (frameTimeAvg > 0) {
		fps = 1.0f / frameTimeAvg;
	}
	else {
		// Fakeamo value da znamo da nes ne radi
		fps = -1;
	}

	currentFrame++;

	return fps;
}

int main()
{
    Server server;
	World world;

	world.server = &server;

	if (server.init(1234)) return 1;

	printf("Server started on port %u\n", server.port);

	// Ovo je process input
	std::thread t1(&Server::update, &server);

	const float DESIRED_FPS = 30.0f;
	const float DESIRED_FRAMETIME = 1.0f / DESIRED_FPS;
	const int MAX_FRAMESKIP = 5;

	auto lastUpdate = std::chrono::steady_clock::now();
	float lag = 0.0f;

	while (true)
	{
		auto now = std::chrono::steady_clock::now();
		float frameTime = (float)std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
		lastUpdate = now;

		lag += frameTime;

		int loops = 0;
		while (lag >= DESIRED_FRAMETIME && loops < MAX_FRAMESKIP)
		{
			// Ovo je "physics" update
			world.update(DESIRED_FRAMETIME, server.currentTick);
			server.currentTick++;
			lag -= DESIRED_FRAMETIME;
			loops++;
		}

		// Ovo printa FPS
		//printf("%d\n", calculateFPS(frameTime));

		now = std::chrono::steady_clock::now();
		frameTime = (float)std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;

		// Limitiraj FPS
		if (DESIRED_FRAMETIME > frameTime) {
			std::this_thread::sleep_for(seconds_type(DESIRED_FRAMETIME - frameTime));
		}
	}

	server.destroy();

	t1.join();

	return 0;
}
