# Speed Racer - Stratégie d'Accélération 🏃‍♂️

## 1. Ordre de Priorité Recommandé

Fais ces tâches dans cet ordre pour avoir un jeu **jouable en 2-3 jours**:

```
Jour 1:
├── Implement Game::init() + Game::run() (boucle principale)
├── InputHandler basique (4 flèches + R pour reset)
└── Render écran noir avec FPS

Jour 2:
├── Car::updateMotion() (physique simple: accel/brake/turn)
├── Charger image piste + détection collision pixel-perfect
├── Render voiture + piste
└── Debug physique

Jour 3:
├── Checkpoints + lap counting
├── HUD minimal (vitesse, temps, lap)
└── Test et polish
```

---

## 2. Réutilise le Code des Projets Existants

### De `speed-racer/src/main.cpp`:
- Structure de boucle RayLib (UpdateDrawFrame)
- Gestion simple voiture top-down
- Détection pixel-parfaite avec la piste

### De `speed-racer-rl/`:
- Physics constants (max_speed, friction, etc.)
- Raycast LIDAR pour observation (utile later pour RL)
- Environment class template

**Copie-colle** ce qui marche, ne réinvente pas la roue! ⚡

---

## 3. Astuces de Performance

### Physique
- Utilise **vector math simple** : `v = v * (1 - friction) + accel * direction`
- Pas besoin de forces complexes pour commencer
- Update vitesse d'abord, position ensuite

### Collision
- Pixel-perfect: charge piste comme texture, sample pixel sous voiture
- Ou: utilise segments + raycast (moins précis mais plus rapide)
- Cache les résultats collision si possible

### Render
- Utilise RayLib `DrawTexture()` pour piste (très rapide)
- Dessine voiture avec `DrawRectanglePro()` + rotation simples
- Ne régenère pas les textures chaque frame!

---

## 4. Structure Code à Respecter

Tes fichiers existent déjà, respecte cette architecture:

```cpp
// game/game.hpp - orchestration
class Game {
    void update(float dt);  // logique
    void render();           // affichage
    // État: car, track, checkpoints, ui
};

// gameplay/car.hpp - physique
class Car {
    void updateMotion(input, dt, friction);
    Vector2 position(), velocity();
    float speed(), angle();
};

// À créer:
// game/track.hpp - gestion piste
class Track {
    bool isColliding(Vector2 pos);
    float getFriction(Vector2 pos);
};

// game/checkpoint.hpp - checkpoints
class Checkpoint {
    bool isTriggered(Vector2 carPos);
    Vector2 getNext();
};

// game/input.hpp - contrôles
class InputHandler {
    CarInput getInput();
};
```

---

## 5. Déploiement CMake Rapide

Ton CMakeLists.txt est déjà bon! Utilise:

```bash
cd speed_racer
mkdir -p build && cd build
cmake .. && make
./speed_racer
```

Ou utilise `dev.sh` si configuré. ⚙️

---

## 6. Debugging & Testing

### Affiche le debug:
```cpp
DrawText(TextFormat("Speed: %.1f", car.speed()), 10, 10, 20, WHITE);
DrawText(TextFormat("Pos: (%.0f, %.0f)", car.position().x, car.position().y), 10, 35, 20, WHITE);
DrawText(TextFormat("FPS: %d", GetFPS()), 10, 60, 20, WHITE);
```

### Test cases importantes:
- Voiture accélère à max_speed? ✓
- Voiture freine correctement? ✓
- Virage fluide? ✓
- Collision stop bien? ✓
- Checkpoints trigger? ✓

---

## 7. Git Workflow

```bash
git add TODO.md STRATEGY.md
git commit -m "docs: add dev roadmap"
git checkout -b feature/game-loop
# trav...
git commit -m "feat: implement Game::run() and basic physics"
git push origin feature/game-loop
# PR review
```

---

## 8. KPI pour Mesurer Progrès

- **Jour 1**: FPS affichage 60 + input registering
- **Jour 2**: Voiture bouge sans crash + physique debug
- **Jour 3**: Course jouable avec checkpoints
- **Semaine 1**: Jeu complet avec multiple tracks
- **Semaine 2**: RL environment prêt

---

## 9. Ressources Rapides

- **RayLib Docs**: https://www.raylib.com/
- **Math basics**: `Vector2` add, multiply (RayLib inclut)
- **Pixelart assets**: itch.io, opengameart.org
- **Sample track**: Speed-racer repo a déjà assets/

---

## 10. Red Flags à Éviter

❌ Ne commence pas par les sons / musique / animations fancy
❌ Ne fais pas d'UI polished avant d'avoir gameplay basique
❌ Ne réinvente pas la physique, utilise les formules simples
❌ Ne fais pas plusieurs pistes en même temps
❌ Ne pense pas à RL avant que le jeu soit jouable

**Focus**: Minimalisme → Jouable → Polish → Contenu ✅

