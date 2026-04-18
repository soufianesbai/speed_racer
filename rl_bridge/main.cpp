#include "game/game.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void printObservation(const std::vector<float>& observation) {
    std::cout << observation.size();
    for (float v : observation) {
        std::cout << ' ' << std::fixed << std::setprecision(6) << v;
    }
}

} // namespace

int main(int argc, char** argv) {
    bool render = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--render") {
            render = true;
        }
    }

    SetTraceLogLevel(LOG_NONE);

    Game game;
    if (!game.init(render)) {
        std::cerr << "ERROR init failed" << std::endl;
        return 1;
    }

    const std::vector<float> initialObs = game.resetEpisode();
    std::cout << "READY ";
    printObservation(initialObs);
    std::cout << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "RESET") {
            const std::vector<float> obs = game.resetEpisode();
            std::cout << "OBS ";
            printObservation(obs);
            std::cout << std::endl;
            continue;
        }

        if (cmd == "STEP") {
            int action = 0;
            float dt = 1.0f / 60.0f;
            iss >> action;
            if (iss.good()) {
                iss >> dt;
            }

            const Game::RLStepResult step = game.stepDiscreteAction(action, dt);
            std::cout << "STEP "
                      << std::fixed << std::setprecision(6) << step.reward << ' '
                      << (step.done ? 1 : 0) << ' '
                      << (step.truncated ? 1 : 0) << ' '
                      << (step.collided ? 1 : 0) << ' '
                      << (step.checkpointReached ? 1 : 0) << ' '
                      << (step.lapFinished ? 1 : 0) << ' '
                      << (step.raceFinished ? 1 : 0) << ' '
                      << step.episodeSteps << ' '
                      << step.episodeCollisions << ' '
                      << step.episodeCheckpoints << ' '
                      << step.episodeLaps << ' '
                      << std::fixed << std::setprecision(6) << step.episodeReward << ' ';
            printObservation(step.observation);
            std::cout << std::endl;
            continue;
        }

        if (cmd == "SET_MAX_STEPS") {
            int maxSteps = 1200;
            iss >> maxSteps;
            game.setMaxEpisodeSteps(maxSteps);
            std::cout << "OK" << std::endl;
            continue;
        }

        if (cmd == "CLOSE" || cmd == "QUIT") {
            break;
        }

        std::cout << "ERROR unknown_command" << std::endl;
    }

    game.shutdown();
    return 0;
}
