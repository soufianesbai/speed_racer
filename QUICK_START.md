# Quick Start Checklist ⚡

## Prépare ton environnement (5 min)

- [ ] Teste le build actuel: `cd build && make && ./speed_racer`
  - Note: Si ça crash, c'est normal, `Game::run()` n'est pas implémenté
- [ ] Ouvre `game/game.cpp` - il est vide (c'est là que tu mets ton code)
- [ ] Ouvre `gameplay/car.cpp` - regarde la structure
- [ ] Lis les `.hpp` pour comprendre les interfaces

---

## Task 1: Affiche Quelque Chose (30 min) 

**Objectif**: Voir la fenêtre du jeu s'ouvrir sans crash

**À faire**: Implémente dans `game/game.cpp`:

```cpp
bool Game::init() {
    InitWindow(screenWidth_, screenHeight_, "Speed Racer");
    SetTargetFPS(60);
    initialized_ = true;
    return true;
}

void Game::run() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        update(dt);
        render();
    }
}

void Game::update(float dt) {
    // TODO: input + physics + collisions
}

void Game::render() {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 20, WHITE);
    EndDrawing();
}

void Game::shutdown() {
    CloseWindow();
}
```

**Test**: `./speed_racer` doit ouvrir une fenêtre noire avec FPS en haut à gauche ✓

---

## Task 2: Dessine la Voiture (30 min)

**À faire dans `Game::render()`**:

```cpp
// Dessine voiture simple
Vector2 carPos = playerCar_.position();
DrawRectanglePro(
    {carPos.x, carPos.y, 40, 60},  // rect (x, y, width, height)
    {20, 30},                        // origin (center)
    playerCar_.angle() * RAD2DEG,   // rotation
    RED
);

// Dessine infos debug
DrawText(TextFormat("Pos: (%.0f, %.0f)", carPos.x, carPos.y), 10, 40, 20, WHITE);
DrawText(TextFormat("Speed: %.1f", playerCar_.speed()), 10, 70, 20, WHITE);
```

**Test**: Fenêtre noire avec un rectangle rouge (la voiture) ✓

---

## Task 3: Contrôles de Base (30 min)

**Crée**: `game/input.hpp`

```cpp
#pragma once
#include "gameplay/car.hpp"

class InputHandler {
public:
    CarInput getInput() const {
        CarInput input;
        
        if (IsKeyDown(KEY_UP))    input.accel = 1.0f;
        if (IsKeyDown(KEY_DOWN))  input.accel = -1.0f;
        if (IsKeyDown(KEY_LEFT))  input.steer = -1.0f;
        if (IsKeyDown(KEY_RIGHT)) input.steer = 1.0f;
        
        return input;
    }
};
```

**Dans `game/game.cpp`**:

```cpp
#include "game/input.hpp"

class Game {
private:
    InputHandler inputHandler_;  // ajoute ceci
};

void Game::update(float dt) {
    CarInput input = inputHandler_.getInput();
    playerCar_.updateMotion(input, dt, surfaceFriction_);
    
    // Reset
    if (IsKeyPressed(KEY_R)) {
        playerCar_.reset(spawnPosition_, spawnAngle_);
    }
}
```

**Test**: Les flèches ne font rien encore (c'est normal), R reset la voiture ✓

---

## Task 4: Physique Simple (1 hour)

**Implémente dans `gameplay/car.cpp`**:

```cpp
void Car::updateMotion(const CarInput& input, float dt, float surfaceFriction) {
    // Applique accélération/freinage
    if (input.accel != 0.0f) {
        speed_ += input.accel * config_.acceleration * dt;
    } else {
        // Applique friction naturelle
        speed_ *= (1.0f - config_.baseFriction * surfaceFriction * dt);
    }
    
    // Clamp speed
    if (speed_ > config_.max_speed) speed_ = config_.max_speed;
    if (speed_ < -config_.max_speed / 2) speed_ = -config_.max_speed / 2;
    
    // Applique direction
    if (input.steer != 0.0f) {
        float maxTurn = config_.turnBase + config_.turnFactor * std::abs(speed_);
        angle_ += input.steer * maxTurn * dt;
    }
    
    // Met à jour position
    Vector2 direction = {std::cos(angle_), std::sin(angle_)};
    position_.x += direction.x * speed_ * dt;
    position_.y += direction.y * speed_ * dt;
}
```

**Test**: Les flèches font bouger la voiture ✓

---

## Task 5: Piste Simple (1 hour)

**Crée**: `game/track.hpp`

```cpp
#pragma once
#include <raylib.h>

class Track {
public:
    Track() : trackImage_(nullptr) {}
    
    bool load(const char* imagePath) {
        trackImage_ = LoadImage(imagePath);
        return trackImage_ != nullptr;
    }
    
    void render() {
        if (trackImage_) {
            DrawTexture({trackImage_->id}, 0, 0, WHITE);
        } else {
            // Fallback: rectangle gris
            DrawRectangle(0, 0, 800, 600, DARKGRAY);
        }
    }
    
    bool isColliding(Vector2 pos) {
        // Pixel perfect: check pixel color at pos
        if (!trackImage_ || pos.x < 0 || pos.y < 0) return true;
        if (pos.x >= trackImage_->width || pos.y >= trackImage_->height) return true;
        
        Color pixel = GetImageColor(*trackImage_, (int)pos.x, (int)pos.y);
        // Assume: noir = wall, autre = track
        return pixel.r < 50 && pixel.g < 50 && pixel.b < 50;
    }
    
    ~Track() {
        if (trackImage_) UnloadImage(*trackImage_);
    }
    
private:
    Image* trackImage_;
};
```

**Test**: Charge `assets/testTrack.png` ou décommente le fallback gris ✓

---

## Task 6: Collisions Basiques (1 hour)

**Dans `Game::update()` après `updateMotion()`**:

```cpp
void Game::update(float dt) {
    Vector2 oldPos = playerCar_.position();
    
    CarInput input = inputHandler_.getInput();
    playerCar_.updateMotion(input, dt, surfaceFriction_);
    
    // Collision check
    if (track_.isColliding(playerCar_.position())) {
        playerCar_.rollbackTo(oldPos);
        playerCar_.bounceOnCollision();
    }
    
    if (IsKeyPressed(KEY_R)) {
        playerCar_.reset(spawnPosition_, spawnAngle_);
    }
}
```

**Implémente dans `car.cpp`**:

```cpp
void Car::rollbackTo(Vector2 prevPos) {
    position_ = prevPos;
}

void Car::bounceOnCollision() {
    speed_ *= config_.wallBounce;
}
```

**Test**: Voiture rebondit quand elle touche un mur ✓

---

## ✅ À ce stade tu as:

- ✅ Fenêtre qui s'ouvre
- ✅ Voiture visible
- ✅ Contrôles (en haut/bas/gauche/droite)
- ✅ Physique basique
- ✅ Piste chargeante
- ✅ Collisions simples

**Temps total**: 3-4 heures 🎉

---

## Prochaines tâches (après ce quick start):

1. Checkpoints + lap counting (1 hour)
2. HUD: vitesse, temps, meilleur temps (30 min)
3. Multiple pistes (2 hours)
4. Effets visuels: skid marks, particules collision (2 hours)
5. Menu principal + états jeu (2 hours)
6. Polish: sons, musique, camera follow (3 hours)

**Bonne chance! 🚗💨**

