#pragma once

#include "gameplay/car.hpp"

#include <raylib.h>

/*
    * Game class represents the main game loop and manages the overall game state.
    * It provides methods for initializing the game, running the main loop, and shutting down the game.
    * The update method is responsible for updating the game state based on the time delta (dt) between frames,
    * while the render method handles drawing the game to the screen.
    *
    * This class serves as the central point of control for the game's execution flow.
*/
class Game {
    public:
        Game() : Game(800, 600) {}
        Game(int screenWidth, int screenHeight)
            : playerCar_(CarConfig{}),
              screenWidth_(screenWidth),
              screenHeight_(screenHeight) {}

        bool init();
        void run();
        void shutdown();

    private:
        // float dt (delta time) represents the time delta between consecutive frames
        void update(float dt);
        void render();

        Car playerCar_;
        int screenWidth_ = 800;
        int screenHeight_ = 600;
        bool initialized_ = false;
        Vector2 spawnPosition_ = {400.0f, 300.0f};
        float spawnAngle_ = 0.0f;
        float surfaceFriction_ = 1.0f;
};