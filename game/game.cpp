#include "game/game.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

bool isWallColor(const Color& color) {
    const bool looksLikeTrack =
        std::abs(static_cast<int>(color.r) - 35) <= 12 &&
        std::abs(static_cast<int>(color.g) - 35) <= 12 &&
        std::abs(static_cast<int>(color.b) - 35) <= 12;

    const bool looksLikeGrass =
        std::abs(static_cast<int>(color.r) - 34) <= 18 &&
        std::abs(static_cast<int>(color.g) - 177) <= 25 &&
        std::abs(static_cast<int>(color.b) - 76) <= 18;

    // Treat any non-driveable color as obstacle so rays stop at the actual border.
    return !looksLikeTrack && !looksLikeGrass;
}

bool isGrassColor(const Color& color) {
    return std::abs(static_cast<int>(color.r) - 34) <= 18 &&
           std::abs(static_cast<int>(color.g) - 177) <= 25 &&
           std::abs(static_cast<int>(color.b) - 76) <= 18;
}

float frictionForColor(const Color& color) {
    if (isWallColor(color)) {
        return 999.0f;
    }
    if (isGrassColor(color)) {
        return 6.0f;
    }
    return 1.0f;
}

template <size_t N>
Texture2D loadTextureFromCandidates(const std::array<const char*, N>& paths, bool& loaded) {
    for (const char* path : paths) {
        if (FileExists(path)) {
            loaded = true;
            return LoadTexture(path);
        }
    }

    loaded = false;
    return Texture2D{};
}

template <size_t N>
Image loadImageFromCandidates(const std::array<const char*, N>& paths, bool& loaded) {
    for (const char* path : paths) {
        if (FileExists(path)) {
            loaded = true;
            return LoadImage(path);
        }
    }

    loaded = false;
    return Image{};
}

bool segmentIntersects(Vector2 a, Vector2 b, Vector2 c, Vector2 d) {
    const float x1 = a.x;
    const float y1 = a.y;
    const float x2 = b.x;
    const float y2 = b.y;
    const float x3 = c.x;
    const float y3 = c.y;
    const float x4 = d.x;
    const float y4 = d.y;

    const float denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (std::fabs(denominator) < 0.0001f) {
        return false;
    }

    const float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
    const float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;
    return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
}

} // namespace

bool Game::init(bool withWindow) {
    withWindow_ = withWindow;

    trackImage_ = loadImageFromCandidates(
        std::array<const char*, 2>{"assets/raceTrack.png", "../assets/raceTrack.png"},
        hasTrackImage_
    );

    if (hasTrackImage_) {
        screenWidth_ = trackImage_.width;
        screenHeight_ = trackImage_.height;
    }

    if (withWindow_) {
        InitWindow(screenWidth_ + hudPanelWidth_, screenHeight_, "Speed Racer");
        SetTargetFPS(60);
        windowInitialized_ = true;

        if (hasTrackImage_) {
            trackTexture_ = LoadTextureFromImage(trackImage_);
            hasTrackTexture_ = true;
        }

        carTexture_ = loadTextureFromCandidates(
            std::array<const char*, 2>{"assets/racecarTransparent.png", "../assets/racecarTransparent.png"},
            hasCarTexture_
        );
    }

    initCheckpoints();
    resetRaceState();
    lidarReadings_.assign(13, 1.0f);
    playerCar_.reset(spawnPosition_, spawnAngle_);
    updateLidarReadings();
    lastObservation_ = buildObservation();
    lastReward_ = 0.0f;
    episodeReward_ = 0.0f;
    initialized_ = true;

    return true;
}

void Game::run() {
    if (!initialized_ || !withWindow_) {
        return;
    }

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime();
        update(dt);
        render();
    }
}

void Game::update(float dt) {
    CarInput input{};
    const Vector2 previousPosition = playerCar_.position();
    const int previousCheckpoint = nextCheckpoint_;
    const int previousLap = currentLap_;
    const bool raceWasFinished = raceFinished_;
    const float previousDistanceToCheckpoint = distanceToCurrentCheckpoint(previousPosition);

    if (useExternalInput_) {
        input = externalInput_;
    } else if (withWindow_) {
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
    }

    if (withWindow_ && IsKeyPressed(KEY_R)) {
        playerCar_.reset(spawnPosition_, spawnAngle_);
        resetRaceState();
        episodeReward_ = 0.0f;
    }

    if (withWindow_ && IsKeyPressed(KEY_L)) {
        drawLidar_ = !drawLidar_;
    }

    surfaceFriction_ = getSurfaceFrictionAt(previousPosition);
    playerCar_.updateMotion(input, dt, surfaceFriction_);

    Vector2 currentPosition = playerCar_.position();
    bool collided = false;
    if (isWallAt(currentPosition)) {
        playerCar_.rollbackTo(previousPosition);
        playerCar_.bounceOnCollision();
        currentPosition = playerCar_.position();
        collided = true;
    }

    const float currentDistanceToCheckpoint = distanceToCurrentCheckpoint(currentPosition);
    const float progressDelta = previousDistanceToCheckpoint - currentDistanceToCheckpoint;

    updateLidarReadings();
    updateRaceState(previousPosition, currentPosition, dt);

    const bool checkpointReached = nextCheckpoint_ != previousCheckpoint;
    const bool lapFinished = currentLap_ > previousLap;
    const bool raceJustFinished = raceFinished_ && !raceWasFinished;

    episodeSteps_ += 1;
    if (collided) {
        episodeCollisions_ += 1;
    }
    if (checkpointReached) {
        episodeCheckpoints_ += 1;
    }
    if (lapFinished) {
        episodeLaps_ += 1;
    }
    if (!raceFinished_ && maxEpisodeSteps_ > 0 && episodeSteps_ >= maxEpisodeSteps_) {
        episodeTruncated_ = true;
    }

    lastStepCollided_ = collided;
    lastStepCheckpointReached_ = checkpointReached;
    lastStepLapFinished_ = lapFinished;
    lastStepRaceFinished_ = raceJustFinished;

    lastObservation_ = buildObservation();
    lastReward_ = computeReward(dt, collided, progressDelta, checkpointReached, lapFinished, raceJustFinished);
    episodeReward_ += lastReward_;
}

void Game::render() {
    if (!withWindow_) {
        return;
    }

    BeginDrawing();
    ClearBackground(BLACK);

    if (hasTrackTexture_) {
        DrawTexture(trackTexture_, 0, 0, WHITE);
    } else {
        DrawRectangle(0, 0, screenWidth_, screenHeight_, DARKGREEN);
    }

    const Vector2 carPosition = playerCar_.position();
    if (hasCarTexture_) {
        const Rectangle source = {0.0f, 0.0f, static_cast<float>(carTexture_.width), static_cast<float>(carTexture_.height)};
        const float scale = 0.1f;
        const Rectangle destination = {
            carPosition.x,
            carPosition.y,
            carTexture_.width * scale,
            carTexture_.height * scale,
        };
        const Vector2 origin = {destination.width * 0.5f, destination.height * 0.5f};
        DrawTexturePro(carTexture_, source, destination, origin, playerCar_.angle() * RAD2DEG, WHITE);
    } else {
        DrawCircleV(carPosition, 18.0f, RED);

        const Vector2 heading = {
            carPosition.x + std::cos(playerCar_.angle()) * 24.0f,
            carPosition.y + std::sin(playerCar_.angle()) * 24.0f,
        };
        DrawLineV(carPosition, heading, BLACK);
    }

    for (size_t i = 0; i < checkpoints_.size(); ++i) {
        const Color checkpointColor = (static_cast<int>(i) == nextCheckpoint_) ? ORANGE : SKYBLUE;
        DrawLineEx(checkpoints_[i].start, checkpoints_[i].end, 3.0f, checkpointColor);
    }

    if (drawLidar_ && !lidarReadings_.empty()) {
        const float minRelativeAngle = -90.0f * DEG2RAD;
        const float maxRelativeAngle = 90.0f * DEG2RAD;
        const int sensorCount = static_cast<int>(lidarReadings_.size());
        const float step = (sensorCount > 1) ? (maxRelativeAngle - minRelativeAngle) / static_cast<float>(sensorCount - 1) : 0.0f;

        for (int i = 0; i < sensorCount; ++i) {
            const float rayAngle = playerCar_.angle() + minRelativeAngle + step * static_cast<float>(i);
            const float distance = lidarReadings_[static_cast<size_t>(i)] * lidarMaxDistance_;
            const Vector2 rayEnd = {
                carPosition.x + std::cos(rayAngle) * distance,
                carPosition.y + std::sin(rayAngle) * distance,
            };
            const Color rayColor = (i == sensorCount / 2) ? RED : Fade(WHITE, 0.45f);
            DrawLineV(carPosition, rayEnd, rayColor);
            DrawCircleV(rayEnd, 2.0f, rayColor);
        }
    }

    const int hudX = screenWidth_;
    DrawRectangle(hudX, 0, hudPanelWidth_, screenHeight_, Fade(BLACK, 0.88f));
    DrawLine(hudX, 0, hudX, screenHeight_, GRAY);

    const int textX = hudX + 20;
    int textY = 20;
    const int lineStep = 32;

    DrawText("RACE HUD", textX, textY, 26, ORANGE);
    textY += 46;

    DrawText(TextFormat("Speed: %.2f", playerCar_.speed()), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Angle: %.2f", playerCar_.angle()), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Surface friction: %.2f", surfaceFriction_), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Lap: %d/%d", currentLap_, totalLaps_), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Next CP: %d/%d", nextCheckpoint_ + 1, static_cast<int>(checkpoints_.size())), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Current lap: %.2fs", currentLapTime_), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Reward: %.3f", lastReward_), textX, textY, 22, WHITE);
    textY += lineStep;
    DrawText(TextFormat("Episode reward: %.1f", episodeReward_), textX, textY, 22, WHITE);
    textY += lineStep;

    if (bestLapTime_ > 0.0f) {
        DrawText(TextFormat("Best lap: %.2fs", bestLapTime_), textX, textY, 22, WHITE);
        textY += lineStep;
    }
    if (lastLapTime_ > 0.0f) {
        DrawText(TextFormat("Last lap: %.2fs", lastLapTime_), textX, textY, 22, WHITE);
        textY += lineStep;
    }

    textY += 12;
    if (!raceStarted_) {
        DrawText("Cross finish line", textX, textY, 20, LIGHTGRAY);
        textY += 24;
        DrawText("to start timer", textX, textY, 20, LIGHTGRAY);
        textY += 34;
    }
    if (raceFinished_) {
        DrawText("Race finished!", textX, textY, 24, YELLOW);
        textY += 28;
        DrawText("Press R to restart.", textX, textY, 20, YELLOW);
        textY += 34;
    }

    DrawText("Arrows to drive", textX, textY, 20, WHITE);
    textY += 24;
    DrawText("R to reset", textX, textY, 20, WHITE);
    textY += 24;
    DrawText(TextFormat("L to toggle lidar: %s", drawLidar_ ? "ON" : "OFF"), textX, textY, 20, WHITE);

    textY += 20;
    DrawText("LIDAR", textX, textY, 22, ORANGE);
    textY += 30;
    for (size_t i = 0; i < lidarReadings_.size(); ++i) {
        const int barWidth = static_cast<int>(200.0f * lidarReadings_[i]);
        DrawRectangle(textX, textY, barWidth, 10, SKYBLUE);
        DrawRectangleLines(textX, textY, 200, 10, DARKGRAY);
        textY += 14;
        if (textY > screenHeight_ - 120) {
            break;
        }
    }

    if (!hasTrackTexture_ || !hasCarTexture_) {
        DrawText("Missing assets:", textX, screenHeight_ - 90, 20, YELLOW);
        DrawText("assets/raceTrack.png", textX, screenHeight_ - 65, 20, YELLOW);
        DrawText("assets/racecarTransparent.png", textX, screenHeight_ - 40, 20, YELLOW);
    }

    EndDrawing();
}

void Game::shutdown() {
    if (initialized_) {
        if (hasTrackTexture_) {
            UnloadTexture(trackTexture_);
            hasTrackTexture_ = false;
        }
        if (hasCarTexture_) {
            UnloadTexture(carTexture_);
            hasCarTexture_ = false;
        }
        if (hasTrackImage_) {
            UnloadImage(trackImage_);
            hasTrackImage_ = false;
        }

        if (windowInitialized_) {
            CloseWindow();
            windowInitialized_ = false;
        }
        initialized_ = false;
    }
}

std::vector<float> Game::resetEpisode() {
    playerCar_.reset(spawnPosition_, spawnAngle_);
    resetRaceState();
    episodeSteps_ = 0;
    episodeCollisions_ = 0;
    episodeCheckpoints_ = 0;
    episodeLaps_ = 0;
    episodeTruncated_ = false;

    lastStepCollided_ = false;
    lastStepCheckpointReached_ = false;
    lastStepLapFinished_ = false;
    lastStepRaceFinished_ = false;

    episodeReward_ = 0.0f;
    lastReward_ = 0.0f;
    updateLidarReadings();
    lastObservation_ = buildObservation();
    return lastObservation_;
}

void Game::setMaxEpisodeSteps(int maxSteps) {
    maxEpisodeSteps_ = std::max(1, maxSteps);
}

Game::RLStepResult Game::stepDiscreteAction(int action, float dt) {
    RLStepResult result{};
    if (!initialized_) {
        return result;
    }

    useExternalInput_ = true;
    externalInput_ = inputFromDiscreteAction(action);
    update(dt);
    useExternalInput_ = false;

    if (withWindow_) {
        render();
        if (WindowShouldClose()) {
            episodeTruncated_ = true;
        }
    }

    result.observation = lastObservation_;
    result.reward = lastReward_;
    result.truncated = episodeTruncated_;
    result.done = raceFinished_ || episodeTruncated_;
    result.collided = lastStepCollided_;
    result.checkpointReached = lastStepCheckpointReached_;
    result.lapFinished = lastStepLapFinished_;
    result.raceFinished = raceFinished_;
    result.episodeSteps = episodeSteps_;
    result.episodeCollisions = episodeCollisions_;
    result.episodeCheckpoints = episodeCheckpoints_;
    result.episodeLaps = episodeLaps_;
    result.episodeReward = episodeReward_;
    return result;
}

float Game::getSurfaceFrictionAt(Vector2 position) const {
    if (!hasTrackImage_) {
        return 1.0f;
    }

    const int pixelX = static_cast<int>(position.x);
    const int pixelY = static_cast<int>(position.y);
    if (pixelX < 0 || pixelY < 0 || pixelX >= trackImage_.width || pixelY >= trackImage_.height) {
        return 999.0f;
    }

    const Color color = GetImageColor(trackImage_, pixelX, pixelY);
    return frictionForColor(color);
}

bool Game::isWallAt(Vector2 position) const {
    if (!hasTrackImage_) {
        return false;
    }

    const int pixelX = static_cast<int>(position.x);
    const int pixelY = static_cast<int>(position.y);
    if (pixelX < 0 || pixelY < 0 || pixelX >= trackImage_.width || pixelY >= trackImage_.height) {
        return true;
    }

    const Color color = GetImageColor(trackImage_, pixelX, pixelY);
    return isWallColor(color);
}

void Game::initCheckpoints() {
    checkpoints_.clear();

    checkpoints_.push_back({{450.0f, 35.0f}, {450.0f, 150.0f}}); // Finish line
    checkpoints_.push_back({{719.0f, 260.0f}, {850.0f, 260.0f}});
    checkpoints_.push_back({{850.0f, 665.0f}, {723.0f, 665.0f}});
    checkpoints_.push_back({{523.0f, 482.0f}, {625.0f, 517.0f}});
    checkpoints_.push_back({{409.0f, 438.0f}, {295.0f, 413.0f}});
    checkpoints_.push_back({{139.0f, 672.0f}, {49.0f, 672.0f}});
    checkpoints_.push_back({{138.0f, 205.0f}, {49.0f, 205.0f}});
}

void Game::resetRaceState() {
    nextCheckpoint_ = 0;
    currentLap_ = 0;
    currentLapTime_ = 0.0f;
    bestLapTime_ = -1.0f;
    lastLapTime_ = -1.0f;
    raceStarted_ = false;
    raceFinished_ = false;
}

void Game::updateRaceState(Vector2 previousPosition, Vector2 currentPosition, float dt) {
    if (checkpoints_.empty() || raceFinished_) {
        return;
    }

    if (raceStarted_) {
        currentLapTime_ += dt;
    }

    const Checkpoint& expected = checkpoints_[nextCheckpoint_];
    if (!segmentIntersects(previousPosition, currentPosition, expected.start, expected.end)) {
        return;
    }

    if (!raceStarted_ && nextCheckpoint_ == 0) {
        raceStarted_ = true;
        currentLap_ = 1;
        currentLapTime_ = 0.0f;
        nextCheckpoint_ = 1;
        return;
    }

    if (nextCheckpoint_ > 0) {
        nextCheckpoint_ += 1;
        if (nextCheckpoint_ >= static_cast<int>(checkpoints_.size())) {
            nextCheckpoint_ = 0;
        }
        return;
    }

    lastLapTime_ = currentLapTime_;
    if (bestLapTime_ < 0.0f || currentLapTime_ < bestLapTime_) {
        bestLapTime_ = currentLapTime_;
    }

    if (currentLap_ >= totalLaps_) {
        raceFinished_ = true;
        currentLapTime_ = 0.0f;
        return;
    }

    currentLap_ += 1;
    currentLapTime_ = 0.0f;
    nextCheckpoint_ = 1;
}

float Game::distanceToCurrentCheckpoint(Vector2 position) const {
    if (checkpoints_.empty()) {
        return 0.0f;
    }

    const Checkpoint& cp = checkpoints_[nextCheckpoint_];
    const Vector2 midpoint = {
        (cp.start.x + cp.end.x) * 0.5f,
        (cp.start.y + cp.end.y) * 0.5f,
    };
    return std::hypot(position.x - midpoint.x, position.y - midpoint.y);
}

std::vector<float> Game::buildObservation() const {
    std::vector<float> obs;
    obs.reserve(7 + lidarReadings_.size());

    const Vector2 pos = playerCar_.position();
    const float maxSpeedRef = 300.0f;
    const float normalizedSpeed = std::clamp(playerCar_.speed() / maxSpeedRef, -1.0f, 1.0f);
    const float normalizedX = (screenWidth_ > 0) ? std::clamp(pos.x / static_cast<float>(screenWidth_), 0.0f, 1.0f) : 0.0f;
    const float normalizedY = (screenHeight_ > 0) ? std::clamp(pos.y / static_cast<float>(screenHeight_), 0.0f, 1.0f) : 0.0f;
    const float normalizedSurface = std::clamp(surfaceFriction_ / 6.0f, 0.0f, 1.0f);
    const float normalizedCheckpoint = checkpoints_.empty()
        ? 0.0f
        : static_cast<float>(nextCheckpoint_) / static_cast<float>(checkpoints_.size());
    const float normalizedLap = (totalLaps_ > 0)
        ? std::clamp(static_cast<float>(currentLap_) / static_cast<float>(totalLaps_), 0.0f, 1.0f)
        : 0.0f;

    obs.push_back(normalizedSpeed);
    obs.push_back(std::sin(playerCar_.angle()));
    obs.push_back(std::cos(playerCar_.angle()));
    obs.push_back(normalizedX);
    obs.push_back(normalizedY);
    obs.push_back(normalizedSurface);
    obs.push_back(normalizedCheckpoint);
    obs.push_back(normalizedLap);

    for (float reading : lidarReadings_) {
        obs.push_back(std::clamp(reading, 0.0f, 1.0f));
    }
    return obs;
}

float Game::computeReward(
    float dt,
    bool collided,
    float progressDelta,
    bool checkpointReached,
    bool lapFinished,
    bool raceFinished
) {
    float reward = 0.0f;

    // Small time penalty encourages the agent to keep moving and finish quickly.
    reward -= 0.03f * dt;

    // Dense progress reward toward the currently expected checkpoint.
    reward += std::clamp(progressDelta * 0.02f, -0.40f, 0.40f);

    // Encourage useful speed while mostly staying on track.
    const float speedReward = std::max(playerCar_.speed(), 0.0f) / 300.0f;
    reward += 0.015f * speedReward;

    // Penalize grass/off-track behavior.
    if (surfaceFriction_ > 1.0f) {
        reward -= 0.015f * (surfaceFriction_ - 1.0f);
    }

    // Penalize stagnant behavior (little/no progress while barely moving).
    if (progressDelta < 0.01f && std::fabs(playerCar_.speed()) < 20.0f) {
        reward -= 0.02f;
    }

    if (collided) {
        reward -= 1.2f;
    }
    if (checkpointReached) {
        reward += 0.8f;
    }
    if (lapFinished) {
        reward += 3.0f;
    }
    if (raceFinished) {
        reward += 8.0f;
    }

    return reward;
}

void Game::updateLidarReadings() {
    if (lidarReadings_.empty()) {
        return;
    }

    const float minRelativeAngle = -90.0f * DEG2RAD;
    const float maxRelativeAngle = 90.0f * DEG2RAD;
    const int sensorCount = static_cast<int>(lidarReadings_.size());
    const float angleStep = (sensorCount > 1) ? (maxRelativeAngle - minRelativeAngle) / static_cast<float>(sensorCount - 1) : 0.0f;

    const Vector2 origin = playerCar_.position();
    for (int i = 0; i < sensorCount; ++i) {
        const float rayAngle = playerCar_.angle() + minRelativeAngle + angleStep * static_cast<float>(i);
        const float hitDistance = castRayToWall(origin, rayAngle, lidarMaxDistance_, 3.0f);
        lidarReadings_[static_cast<size_t>(i)] = hitDistance / lidarMaxDistance_;
    }
}

float Game::castRayToWall(Vector2 origin, float angle, float maxDistance, float step) const {
    for (float distance = step; distance <= maxDistance; distance += step) {
        const Vector2 point = {
            origin.x + std::cos(angle) * distance,
            origin.y + std::sin(angle) * distance,
        };
        if (isLidarObstacleAt(point)) {
            return distance;
        }
    }
    return maxDistance;
}

bool Game::isLidarObstacleAt(Vector2 position) const {
    if (!hasTrackImage_) {
        return false;
    }

    const int pixelX = static_cast<int>(position.x);
    const int pixelY = static_cast<int>(position.y);
    if (pixelX < 0 || pixelY < 0 || pixelX >= trackImage_.width || pixelY >= trackImage_.height) {
        return true;
    }

    const Color color = GetImageColor(trackImage_, pixelX, pixelY);
    return isWallColor(color) || isGrassColor(color);
}

CarInput Game::inputFromDiscreteAction(int action) const {
    CarInput input{};
    switch (action) {
        case 0: // no-op
            break;
        case 1: // throttle
            input.accel = 1.0f;
            break;
        case 2: // brake/reverse
            input.accel = -1.0f;
            break;
        case 3: // throttle + left
            input.accel = 1.0f;
            input.steer = -1.0f;
            break;
        case 4: // throttle + right
            input.accel = 1.0f;
            input.steer = 1.0f;
            break;
        case 5: // left only
            input.steer = -1.0f;
            break;
        case 6: // right only
            input.steer = 1.0f;
            break;
        default:
            break;
    }
    return input;
}
