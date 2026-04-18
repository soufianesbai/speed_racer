#pragma once

#include <raylib.h>

// Car configuration parameters that define its behavior and physics properties.
struct CarConfig {
    float max_speed = 300.0f; // Maximum speed of the car in units per second
    float acceleration = 150.0f; // Acceleration rate of the car
    float brake = 200.0f; // Braking rate of the car
    float baseFriction = 50.0f; // Base friction coefficient for the car
    float turnBase = 3.0f; // Base turning ability of the car
    float turnFactor = 0.3f; // Factor that affects how much the car can turn based on its speed
    float wallBounce = -0.5f; // Bounce factor when the car collides with a wall
};

// Input structure for controlling the car's acceleration and steering.
struct CarInput {
    float accel = 0.0f; // [-1, 1]
    float steer = 0.0f; // [-1, 1]
};

class Car {
    public:
        Car(const CarConfig& config);
        
        // Resets the car to a specific position and angle, typically used at the start of a race or after a collision.
        void reset(Vector2 spawnPos, float spawnAngle);

        // Updates the car motion for one frame using input, frame delta time, and surface friction.
        void updateMotion(const CarInput& input, float dt, float surfaceFriction);

        // Rolls back the car's position to a previous state, used for collision response.
        void rollbackTo(Vector2 prevPos);
        
        // Applies a bounce effect after collision by reversing/damping scalar speed.
        void bounceOnCollision();

        Vector2 position() const;
        Vector2 velocity() const;
        float speed() const;
        float angle() const;

    private:
        CarConfig config_;
        float speed_ = 0.0f; // Current speed of the car
        float angle_ = 0.0f; // Current angle of the car in radians
        Vector2 position_ = {0.0f, 0.0f}; // Current position of the car
        Vector2 velocity_ = {0.0f, 0.0f}; // Current velocity of the car
};