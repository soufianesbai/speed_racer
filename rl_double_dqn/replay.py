from __future__ import annotations

import argparse
from pathlib import Path

from dqn_agent import DQNAgent
from env_wrapper import SpeedRacerEnv


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Replay a trained DQN checkpoint in render mode")
    parser.add_argument("--checkpoint", type=Path, required=True, help="Path to .pt checkpoint")
    parser.add_argument("--episodes", type=int, default=1)
    parser.add_argument("--max-steps", type=int, default=6000)
    parser.add_argument("--step-dt", type=float, default=1.0 / 60.0)
    parser.add_argument("--real-time", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if not args.checkpoint.exists():
        raise FileNotFoundError(f"Checkpoint not found: {args.checkpoint}")

    env = SpeedRacerEnv(max_steps=args.max_steps, render=True, step_dt=args.step_dt)
    agent = DQNAgent(obs_dim=env.observation_dim, action_dim=env.action_dim)
    agent.load(str(args.checkpoint))

    rewards = []
    finishes = 0

    try:
        for ep in range(1, args.episodes + 1):
            obs = env.reset()
            done = False
            ep_reward = 0.0
            last_result = None

            while not done:
                action = agent.select_action(obs, evaluate=True)
                result = env.step(action)
                obs = result.obs
                done = result.done
                ep_reward += result.reward
                last_result = result

            rewards.append(ep_reward)
            if last_result is not None:
                finishes += int(last_result.race_finished)
                print(
                    f"episode={ep:03d} "
                    f"reward={ep_reward:.2f} "
                    f"checkpoints={last_result.episode_checkpoints} "
                    f"laps={last_result.episode_laps} "
                    f"collisions={last_result.episode_collisions} "
                    f"finished={int(last_result.race_finished)}"
                )
    finally:
        env.close()

    avg_reward = sum(rewards) / max(len(rewards), 1)
    finish_rate = finishes / max(args.episodes, 1)
    print(f"summary avg_reward={avg_reward:.2f} finish_rate={finish_rate:.2f}")


if __name__ == "__main__":
    main()
