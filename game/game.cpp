#include "game/game.hpp"

#include <cmath>

bool Game::init() {
    InitWindow(screenWidth_, screenHeight_, "Speed Racer");
    SetTargetFPS(60);

    playerCar_.reset(spawnPosition_, spawnAngle_);
    initialized_ = true;

    return true;
}

void Game::run() {
    if (!initialized_) {
        return;
    }

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        update(dt);
        render();
    }
}

void Game::update(float dt) {
    CarInput input{};

    if (IsKeyDown(KEY_UP)) {
        input.accel += 1.0f;
    }
    if (IsKeyDown(KEY_DOWN)) {
        input.accel -= 1.0f;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        input.steer += 1.0f;
    }
    if (IsKeyDown(KEY_LEFT)) {
        input.steer -= 1.0f;
    }

    if (IsKeyPressed(KEY_R)) {
        playerCar_.reset(spawnPosition_, spawnAngle_);
    }

    playerCar_.updateMotion(input, dt, surfaceFriction_);
}

void Game::render() {
    BeginDrawing();
    ClearBackground(DARKGREEN);

    const Vector2 carPosition = playerCar_.position();
    DrawCircleV(carPosition, 18.0f, RED);

    const Vector2 heading = {
        carPosition.x + std::cos(playerCar_.angle()) * 24.0f,
        carPosition.y + std::sin(playerCar_.angle()) * 24.0f,
    };
    DrawLineV(carPosition, heading, BLACK);

    DrawText(TextFormat("Speed: %.2f", playerCar_.speed()), 20, 20, 20, WHITE);
    DrawText(TextFormat("Angle: %.2f", playerCar_.angle()), 20, 50, 20, WHITE);
    DrawText("Arrows to drive, R to reset", 20, 80, 20, WHITE);

    EndDrawing();
}

void Game::shutdown() {
    if (initialized_) {
        CloseWindow();
        initialized_ = false;
    }
}