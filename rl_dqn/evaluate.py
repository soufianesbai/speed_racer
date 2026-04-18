from __future__ import annotations

import argparse
import shutil
from pathlib import Path

import numpy as np

from dqn_agent import DQNAgent
from env_wrapper import SpeedRacerEnv


def evaluate_checkpoint(
    ckpt_path: Path,
    episodes: int,
    max_steps: int,
    render: bool,
    step_dt: float,
    real_time: bool,
) -> dict:
    env = SpeedRacerEnv(max_steps=max_steps, render=render, step_dt=step_dt)
    agent = DQNAgent(obs_dim=env.observation_dim, action_dim=env.action_dim)
    agent.load(str(ckpt_path))

    rewards = []
    checkpoints = []
    laps = []
    collisions = []
    finishes = 0

    try:
        for _ in range(episodes):
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
                checkpoints.append(last_result.episode_checkpoints)
                laps.append(last_result.episode_laps)
                collisions.append(last_result.episode_collisions)
                finishes += int(last_result.race_finished)
    finally:
        env.close()

    return {
        "checkpoint": str(ckpt_path),
        "avg_reward": float(np.mean(rewards)) if rewards else 0.0,
        "std_reward": float(np.std(rewards)) if rewards else 0.0,
        "avg_checkpoints": float(np.mean(checkpoints)) if checkpoints else 0.0,
        "avg_laps": float(np.mean(laps)) if laps else 0.0,
        "avg_collisions": float(np.mean(collisions)) if collisions else 0.0,
        "finish_rate": float(finishes / max(episodes, 1)),
    }


def ranking_key(result: dict) -> tuple:
    return (
        result["finish_rate"],
        result["avg_laps"],
        result["avg_checkpoints"],
        result["avg_reward"],
        -result["avg_collisions"],
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Evaluate DQN checkpoints for speed_racer (rl_dqn)")
    parser.add_argument("--runs-dir", type=Path, default=Path("rl_dqn/runs"))
    parser.add_argument("--episodes", type=int, default=5)
    parser.add_argument("--max-steps", type=int, default=2000)
    parser.add_argument("--checkpoint", type=Path, default=None)
    parser.add_argument("--promote-best", action="store_true", help="Copy best checkpoint to runs-dir/dqn_best.pt")
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--step-dt", type=float, default=1.0 / 60.0)
    parser.add_argument("--real-time", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if args.checkpoint is not None:
        checkpoints = [args.checkpoint]
    else:
        checkpoints = sorted(args.runs_dir.glob("*.pt"))

    if not checkpoints:
        raise FileNotFoundError("No checkpoints found. Use --runs-dir or --checkpoint.")

    results = []
    for ckpt in checkpoints:
        result = evaluate_checkpoint(
            ckpt_path=ckpt,
            episodes=args.episodes,
            max_steps=args.max_steps,
            render=args.render,
            step_dt=args.step_dt,
            real_time=args.real_time,
        )
        results.append(result)

        print(
            f"{ckpt.name}: "
            f"avg_reward={result['avg_reward']:.2f} "
            f"avg_checkpoints={result['avg_checkpoints']:.2f} "
            f"avg_laps={result['avg_laps']:.2f} "
            f"avg_collisions={result['avg_collisions']:.2f} "
            f"finish_rate={result['finish_rate']:.2f}"
        )

    best = max(results, key=ranking_key)
    print("\nBEST CHECKPOINT")
    print(
        f"path={best['checkpoint']} "
        f"avg_reward={best['avg_reward']:.2f} "
        f"avg_checkpoints={best['avg_checkpoints']:.2f} "
        f"avg_laps={best['avg_laps']:.2f} "
        f"avg_collisions={best['avg_collisions']:.2f} "
        f"finish_rate={best['finish_rate']:.2f}"
    )

    if args.promote_best:
        target = args.runs_dir / "dqn_best.pt"
        source = Path(best["checkpoint"])
        target.parent.mkdir(parents=True, exist_ok=True)
        if source != target:
            shutil.copy2(source, target)
            print(f"promoted best checkpoint to: {target}")
        else:
            print(f"best checkpoint already at: {target}")


if __name__ == "__main__":
    main()
