#pragma once

#include "gameplay/car.hpp"

#include <raylib.h>
#include <vector>

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
        struct RLStepResult {
            std::vector<float> observation;
            float reward = 0.0f;
            bool done = false;
            bool truncated = false;
            bool collided = false;
            bool checkpointReached = false;
            bool lapFinished = false;
            bool raceFinished = false;
            int episodeSteps = 0;
            int episodeCollisions = 0;
            int episodeCheckpoints = 0;
            int episodeLaps = 0;
            float episodeReward = 0.0f;
        };

        struct Checkpoint {
            Vector2 start{};
            Vector2 end{};
        };

        Game() : Game(900, 900) {}
        Game(int screenWidth, int screenHeight)
            : playerCar_(CarConfig{}),
              screenWidth_(screenWidth),
              screenHeight_(screenHeight) {}

        bool init(bool withWindow = true);
        void run();
        void shutdown();
        void setMaxEpisodeSteps(int maxSteps);
        std::vector<float> resetEpisode();
        RLStepResult stepDiscreteAction(int action, float dt = 1.0f / 60.0f);

    private:
        // float dt (delta time) represents the time delta between consecutive frames
        void update(float dt);
        void render();
        void initCheckpoints();
        void resetRaceState();
        void updateRaceState(Vector2 previousPosition, Vector2 currentPosition, float dt);
        void updateLidarReadings();
        float castRayToWall(Vector2 origin, float angle, float maxDistance, float step) const;
        bool isLidarObstacleAt(Vector2 position) const;
        CarInput inputFromDiscreteAction(int action) const;
        float distanceToCurrentCheckpoint(Vector2 position) const;
        std::vector<float> buildObservation() const;
        float computeReward(
            float dt,
            bool collided,
            float progressDelta,
            bool checkpointReached,
            bool lapFinished,
            bool raceFinished
        );

        float getSurfaceFrictionAt(Vector2 position) const;
        bool isWallAt(Vector2 position) const;

        Car playerCar_;
        Image trackImage_{};
        Texture2D trackTexture_{};
        Texture2D carTexture_{};
        bool hasTrackImage_ = false;
        bool hasTrackTexture_ = false;
        bool hasCarTexture_ = false;
        int screenWidth_ = 900;
        int screenHeight_ = 900;
        int hudPanelWidth_ = 360;
        bool initialized_ = false;
        Vector2 spawnPosition_ = {430.0f, 92.0f};
        float spawnAngle_ = 0.0f;
        float surfaceFriction_ = 1.0f;

        std::vector<Checkpoint> checkpoints_;
        int nextCheckpoint_ = 0;
        int currentLap_ = 0;
        int totalLaps_ = 3;
        float currentLapTime_ = 0.0f;
        float bestLapTime_ = -1.0f;
        float lastLapTime_ = -1.0f;
        bool raceStarted_ = false;
        bool raceFinished_ = false;

        std::vector<float> lidarReadings_;
        float lidarMaxDistance_ = 220.0f;
        bool drawLidar_ = true;

        std::vector<float> lastObservation_;
        float lastReward_ = 0.0f;
        float episodeReward_ = 0.0f;

        int maxEpisodeSteps_ = 1200;
        int episodeSteps_ = 0;
        int episodeCollisions_ = 0;
        int episodeCheckpoints_ = 0;
        int episodeLaps_ = 0;
        bool episodeTruncated_ = false;

        bool lastStepCollided_ = false;
        bool lastStepCheckpointReached_ = false;
        bool lastStepLapFinished_ = false;
        bool lastStepRaceFinished_ = false;

        bool withWindow_ = true;
        bool windowInitialized_ = false;
        bool useExternalInput_ = false;
        CarInput externalInput_{};
};