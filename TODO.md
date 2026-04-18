# Speed Racer - TODO List

## Phase 1: Fondations du jeu 🏗️

### Physique & Mouvement
- [ ] Implémenter `Car::updateMotion()` complète (accélération, freinage, friction)
- [ ] Implémenter `Car::rollbackTo()` pour la gestion des collisions
- [ ] Implémenter `Car::bounceOnCollision()` pour les rebonds mur
- [ ] Ajouter la barre d'énergie/santé après collision (health/damage system)
- [ ] Implémenter la friction de surface (track vs grass vs wall)

### Boucle de jeu
- [ ] Implémenter `Game::init()` - initialiser RayLib, écran, ressources
- [ ] Implémenter `Game::run()` - boucle principale (update/render)
- [ ] Implémenter `Game::update()` - traiter input, physique, collisions
- [ ] Implémenter `Game::render()` - dessiner voiture, monde, interface
- [ ] Implémenter `Game::shutdown()` - nettoyer ressources

### Input & Contrôles
- [ ] Mapper les contrôles (↑ accél, ↓ frein, ←/→ direction)
- [ ] Créer classe `InputHandler` pour gérer les événements clavier
- [ ] Ajouter reset voiture (touche R)

---

## Phase 2: Monde du jeu 🌍

### Track & Environnement
- [ ] Créer classe `Track` pour gérer la piste
- [ ] Charger une image de piste (PNG ou créer procedurale)
- [ ] Implémenter détection de collision avec pixels (raycast ou pixel-perfect)
- [ ] Implémenter système de surface (track=friction 1.0, grass=friction 0.7, wall=collision)
- [ ] Ajouter spawn points configurables

### Système de course
- [ ] Créer classe `Checkpoint` pour gérer les checkpoints
- [ ] Implémenter système de lap counting (tour complet = passage de 1→2→...→1)
- [ ] Ajouter horloge pour mesurer le temps au circuit
- [ ] Afficher le meilleur temps (best lap)

### Caméra & Vue
- [ ] Implémenter caméra qui suit la voiture
- [ ] Ajouter zoom/dézoom
- [ ] Afficher minimap si possible

---

## Phase 3: Interface & Gameplay 🎮

### HUD (Heads-Up Display)
- [ ] Afficher vitesse actuelle
- [ ] Afficher angle/direction
- [ ] Afficher temps actuel/meilleur temps
- [ ] Afficher numéro du checkpoint actuel
- [ ] Afficher santé/dégâts voiture

### Menu & États
- [ ] Créer classe `GameState` (enum: MENU, PLAYING, PAUSED, GAME_OVER)
- [ ] Implémenter menu principal (Play, Quit)
- [ ] Implémenter écran pause (Resume, Restart, Quit)
- [ ] Implémenter écran game over avec résultats

### Effets Visuels
- [ ] Ajouter traînées de friction/skid marks
- [ ] Ajouter effets de collision (particules, shake caméra)
- [ ] Ajouter animations voiture (rotation roues)
- [ ] Ajouter dégâts visuels sur la carrosserie

---

## Phase 4: Contenu & Polish 📦

### Assets
- [ ] Créer/télécharger sprite voiture haute résolution
- [ ] Créer 2-3 pistes différentes
- [ ] Ajouter sons (moteur, collision, music)
- [ ] Charger resources dans `assets/`

### Configuration
- [ ] Créer fichier config JSON/YAML pour CarConfig
- [ ] Créer système de difficulty (Easy, Normal, Hard)
- [ ] Ajouter sauvegarde high scores

### Tests
- [ ] Tester physique (accélération, virage, freinage)
- [ ] Tester collisions (précision, bugs edge-case)
- [ ] Tester performance (FPS stable)
- [ ] Tester sur OS macOS / Windows / Linux

---

## Phase 5: Préparation pour RL 🤖

### Infrastructure RL
- [ ] Créer classe `Environment` qui expose l'état du jeu
- [ ] Implémenter observation space (position, vélocité, angle, raycast lidar)
- [ ] Implémenter action space discret (7 actions ou continu)
- [ ] Ajouter reward shaping (vitesse + checkpoint + pénalité collision)
- [ ] Mode headless (pas de render RayLib) pour training rapide

### Monitoring & Debug
- [ ] Logger reward / episode progress
- [ ] Créer tableau de bord training (avec visualisation?)
- [ ] Ajouter replay mode pour tester modèles entraînés

---

## Priorité Rapide pour Démarrer 🚀

**Pour commencer tout de suite (ordre recommandé):**

1. ✅ Implémenter `Game::init()` ou `Game::run()`
2. ✅ Implémenter `InputHandler` + contrôles de base
3. ✅ Implémenter `Car::updateMotion()` physique simple
4. ✅ Charger une piste (image simple) et détection collision basique
5. ✅ Afficher voiture + piste + HUD minimal (vitesse)
6. ✅ Ajouter checkpoints et lap counting
7. ✅ Tester et polish jusqu'à jeu jouable
8. ✅ Ensuite: ajouter sons, effets, plusieurs pistes
9. ✅ Finalement: infrastructure RL + training

---

## Notes

- **Dépendances**: RayLib (déjà dans CMakeLists.txt) + optionnel: libtorch pour RL
- **Architecture**: POO bien structurée, facile d'ajouter `Environment` pour RL
- **Référence**: speed-racer (simple) et speed-racer-rl (complet) disponibles pour inspiration
- **Build**: `dev.sh` probablement lance le build, à vérifier

