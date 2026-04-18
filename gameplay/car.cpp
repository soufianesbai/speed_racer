#include "car.hpp"

#include <algorithm>
#include <cmath>

namespace {

float safeMaxSpeed(const CarConfig& config) {
	// Prevent invalid division if max_speed is misconfigured.
	return std::max(config.max_speed, 1.0f);
}

float surfaceTractionMultiplier(float surfaceFriction) {
	if (surfaceFriction <= 1.0f) {
		return 1.0f;
	}
	// Keep enough traction to move from rest, but still penalize off-track acceleration.
	return std::clamp(1.0f / std::sqrt(surfaceFriction), 0.35f, 1.0f);
}

float surfaceSpeedMultiplier(float surfaceFriction) {
	if (surfaceFriction <= 1.0f) {
		return 1.0f;
	}
	// Cap top speed on rough surfaces while preserving controllability.
	return std::clamp(1.0f / std::sqrt(surfaceFriction), 0.40f, 1.0f);
}

void applyThrottleAndBrake(float& speed, const CarConfig& config, float accelInput, float dt, float surfaceFriction) {
	const float tractionMultiplier = surfaceTractionMultiplier(surfaceFriction);
	if (accelInput > 0.0f) {
		speed += config.acceleration * accelInput * tractionMultiplier * dt;
	} else if (accelInput < 0.0f) {
		if (speed > 0.0f) {
			// If moving forward, negative input acts as brake.
			speed -= config.brake * (-accelInput) * tractionMultiplier * dt;
			if (speed < 0.0f) speed = 0.0f;
		} else {
			// Reverse is weaker than forward for finer low-speed control.
			speed += config.acceleration * 0.4f * accelInput * tractionMultiplier * dt;
		}
	}
}

void applyFriction(float& speed, const CarConfig& config, float accelInput, float dt, float surfaceFriction) {
	// Use a smoother drag model to avoid getting "stuck" from rest on grass.
	const float absSpeed = std::fabs(speed);
	float rollingDecel = config.baseFriction * surfaceFriction;
	if (std::fabs(accelInput) >= 0.001f) {
		// Under throttle, keep resistance but allow launch from rest on rough surfaces.
		rollingDecel *= (absSpeed < 5.0f) ? 0.08f : 0.18f;
	}

	// Add mild speed-proportional drag so high speed decays faster off-track.
	const float linearDrag = absSpeed * 0.15f * std::max(surfaceFriction, 1.0f);
	const float totalDecel = rollingDecel + linearDrag;

	if (speed > 0.0f) {
		speed -= totalDecel * dt;
		if (speed < 0.0f) speed = 0.0f;
	} else if (speed < 0.0f) {
		speed += totalDecel * dt;
		if (speed > 0.0f) speed = 0.0f;
	}
}

void snapTinySpeedToZero(float& speed) {
	// Avoid tiny sign oscillations around zero.
    const float kEpsilonSpeed = 0.01f;
	if (std::fabs(speed) < kEpsilonSpeed) {
		speed = 0.0f;
	}
}

void clampSpeed(float& speed, const CarConfig& config, float surfaceFriction) {
	// Clamp forward and reverse top speeds.
	const float maxForward = safeMaxSpeed(config) * surfaceSpeedMultiplier(surfaceFriction);
	const float maxReverse = -maxForward * 0.5f;
	speed = std::clamp(speed, maxReverse, maxForward);
}

float computeTurnRate(float speed, const CarConfig& config) {
	// Steering gets weaker at higher speed for stability.
	// The turn rate is calculated based on the current speed of the car.
	// As the speed increases, the turn rate decreases,
	// making it harder to steer sharply at high speeds.
	const float maxSpeed = safeMaxSpeed(config);
	const float speedFactor = 1.0f / (1.0f + (std::fabs(speed) / maxSpeed) * config.turnFactor);
	return config.turnBase * speedFactor;
}

void applySteering(float& angle, float speed, float steerInput, float turnRate, float dt) {
	if (std::fabs(speed) > 1.0f) {
		// When reversing, steering direction is inverted.
		float dir = speed > 0.0f ? 1.0f : -1.0f;
		angle += steerInput * turnRate * dt * dir;
	}
}

void updateVelocity(Vector2& velocity, float angle, float speed) {
	// Convert scalar speed + heading into velocity vector.
	velocity.x = std::cos(angle) * speed;
	velocity.y = std::sin(angle) * speed;
}

void integratePosition(Vector2& position, const Vector2& velocity, float dt) {
	// Integrate position using velocity and frame delta time.
	position.x += velocity.x * dt;
	position.y += velocity.y * dt;
}

} // namespace

Car::Car(const CarConfig& config) : config_(config) {}

void Car::reset(Vector2 spawnPos, float spawnAngle) {
	position_ = spawnPos;
	angle_ = spawnAngle;
	speed_ = 0.0f;
	velocity_ = {0.0f, 0.0f};
}

void Car::updateMotion(const CarInput& input, float dt, float surfaceFriction) {
	// Clamp raw input so the car logic always receives a valid range.
	float accelInput = std::clamp(input.accel, -1.0f, 1.0f);
	float steerInput = std::clamp(input.steer, -1.0f, 1.0f);

	// Apply throttle / reverse acceleration to scalar speed.
	applyThrottleAndBrake(speed_, config_, accelInput, dt, surfaceFriction);

	// Apply drag/friction. Off-throttle friction depends on surface type.
	applyFriction(speed_, config_, accelInput, dt, surfaceFriction);

	// Avoid tiny sign oscillations around zero.
	snapTinySpeedToZero(speed_);

	// Clamp forward and reverse top speeds.
	clampSpeed(speed_, config_, surfaceFriction);

	// Steering gets weaker at higher speed for stability.
	const float turnRate = computeTurnRate(speed_, config_);
	applySteering(angle_, speed_, steerInput, turnRate, dt);

	// Convert scalar speed + heading into velocity vector.
	updateVelocity(velocity_, angle_, speed_);

	// Integrate position using velocity and frame delta time.
	integratePosition(position_, velocity_, dt);
}

void Car::rollbackTo(Vector2 prevPos) {
	position_ = prevPos;
}

void Car::bounceOnCollision() {
	speed_ *= config_.wallBounce;
	// Keep velocity coherent in the same frame after speed sign/magnitude change.
	updateVelocity(velocity_, angle_, speed_);
}

Vector2 Car::position() const { return position_; }
Vector2 Car::velocity() const { return velocity_; }
float Car::speed() const { return speed_; }
float Car::angle() const { return angle_; }
