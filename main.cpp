#include "game/game.hpp"

int main() {
    Game game;
    if (game.init()) {
        game.run();
    }
    game.shutdown();
    return 0;
}
