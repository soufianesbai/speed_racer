# Speed Racer RL Portfolio Project

A lightweight 2D racing simulation in C++ (Raylib) with a Python DQN agent trained through a C++ <-> Python bridge.

This project demonstrates:
- OOP game architecture in C++
- custom RL environment wrapping a native simulator
- reward shaping + lidar-based observations
- practical training, evaluation, and model selection workflow

## 1) Project Overview

Core idea:
- The game simulation runs in C++ for speed and deterministic control.
- A bridge executable exposes reset/step commands.
- Python trains a DQN agent against this environment.

Main folders:
- `game/`, `gameplay/`: simulation, race logic, car physics, rendering
- `rl_bridge/`: C++ bridge process for RL step/reset protocol
- `rl/`: Python DQN agent, replay buffer, env wrapper, train/evaluate/replay scripts
- `assets/`: track + car textures

## 2) Tech Stack

- C++17
- Raylib
- CMake
- Python 3
- PyTorch + NumPy

## 3) Features

Gameplay side:
- image-based track
- off-track penalties and collisions
- checkpoints, laps, race timer
- HUD panel and lidar debug

RL side:
- discrete action space (7 actions)
- lidar + vehicle-state observation vector
- reward shaping for progress, speed, penalties, and race objectives
- episode truncation via max-steps
- checkpoint ranking and best-model promotion

## 4) Quick Start (Demo)

Prerequisites:
- CMake and C++ toolchain
- Python venv at `/Users/soufianesbai/Projets/.venv`
- installed Python deps from `requirements-rl.txt`

Build bridge:

```bash
./rl.sh build
```

Run trained agent (visual demo):

```bash
./rl.sh play rl/runs/dqn_best.pt 1 6000
```

Evaluate checkpoints:

```bash
./rl.sh best 10 2000
```

Promote current best checkpoint to `rl/runs/dqn_best.pt`:

```bash
./rl.sh promote 10 2000
```

## 5) Training Commands

Train from scratch:

```bash
./rl.sh train 1000 50 2000
```

Resume from best model:

```bash
./rl.sh resume rl/runs/dqn_best.pt 1000 50 2000 0.0003
```

Generated artifacts in `rl/runs/`:
- `dqn_ep_*.pt`: periodic checkpoints
- `dqn_final.pt`: final model of the run
- `dqn_best.pt`: best promoted model
- `metrics.csv`: per-episode training metrics

## 6) RL Engineering Notes

Observation includes:
- normalized kinematic state (speed, heading/alignment, local position info)
- 13 lidar distances around the car

Action space (discrete):
- idle
- accelerate
- brake
- left
- right
- accelerate + left
- accelerate + right

Reward design combines:
- small time cost
- speed/progress incentives
- checkpoint/lap/race bonuses
- off-track and collision penalties

Model selection strategy:
- evaluate multiple checkpoints over N episodes
- rank by finish/lap/checkpoint/reward/collision criteria
- promote best to stable demo model (`dqn_best.pt`)