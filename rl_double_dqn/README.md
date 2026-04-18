# rl_double_dqn

This folder is a parallel RL experiment track.

Purpose:
- keep `rl_dqn/` untouched as baseline
- test Double DQN safely in `rl_double_dqn/`
- compare results with identical environment settings

Key difference vs baseline:
- target computation uses Double DQN action selection/evaluation split

Suggested workflow:
1. Train Double DQN: `./rl_double_dqn.sh train 1000 50 2000`
2. Find best Double DQN checkpoint: `./rl_double_dqn.sh best 10 2000`
3. Promote best Double DQN model: `./rl_double_dqn.sh promote 10 2000`
4. Visual replay: `./rl_double_dqn.sh play rl_double_dqn/runs/dqn_best.pt 1 6000`

Comparison note:
- baseline checkpoints stay in `rl_dqn/runs/`
- Double DQN checkpoints stay in `rl_double_dqn/runs/`
