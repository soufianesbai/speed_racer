from __future__ import annotations

import argparse
import csv
from pathlib import Path
import time

import numpy as np

from dqn_agent import DQNAgent
from env_wrapper import SpeedRacerEnv
from replay_buffer import ReplayBuffer


def train(
    episodes: int,
    batch_size: int,
    warmup_steps: int,
    train_every: int,
    save_every: int,
    out_dir: Path,
    render: bool,
    step_dt: float,
    real_time: bool,
    max_steps: int,
    resume: bool,
    resume_from: Path | None,
    lr: float | None,
    resume_lr: float | None,
    early_stop_patience: int,
    metrics_csv: Path | None,
) -> None:
    env = SpeedRacerEnv(render=render, step_dt=step_dt, max_steps=max_steps)
    agent = DQNAgent(obs_dim=env.observation_dim, action_dim=env.action_dim)
    buffer = ReplayBuffer(capacity=100_000)

    out_dir.mkdir(parents=True, exist_ok=True)

    checkpoint_to_load: Path | None = None
    if resume_from is not None:
        checkpoint_to_load = resume_from
    elif resume:
        default_ckpt = out_dir / "dqn_final.pt"
        if default_ckpt.exists():
            checkpoint_to_load = default_ckpt

    if checkpoint_to_load is not None:
        if not checkpoint_to_load.exists():
            raise FileNotFoundError(f"resume checkpoint not found: {checkpoint_to_load}")
        agent.load(str(checkpoint_to_load))
        # Continue in low-exploration regime when resuming.
        agent.steps = agent.cfg.eps_decay
        print(f"resumed from: {checkpoint_to_load}")
        if resume_lr is not None:
            agent.set_lr(resume_lr)
            print(f"resume lr override: {resume_lr}")
    elif lr is not None:
        agent.set_lr(lr)
        print(f"lr override: {lr}")

    best_ckpt = out_dir / "dqn_best.pt"
    best_score = float("-inf")
    no_improve_count = 0

    metrics_csv_path = metrics_csv if metrics_csv is not None else (out_dir / "metrics.csv")
    metrics_csv_path.parent.mkdir(parents=True, exist_ok=True)
    csv_fieldnames = [
        "episode",
        "reward",
        "sim_reward",
        "epsilon",
        "buffer",
        "loss",
        "steps",
        "collisions",
        "checkpoints",
        "laps",
        "truncated",
        "race_finished",
        "best_score",
        "global_step",
    ]

    csv_needs_header = not metrics_csv_path.exists() or metrics_csv_path.stat().st_size == 0
    metrics_file = metrics_csv_path.open("a", newline="", encoding="utf-8")
    metrics_writer = csv.DictWriter(metrics_file, fieldnames=csv_fieldnames)
    if csv_needs_header:
        metrics_writer.writeheader()

    global_step = 0
    try:
        for episode in range(1, episodes + 1):
            obs = env.reset()
            done = False
            ep_reward = 0.0
            losses = []
            last_result = None

            while not done:
                action = agent.select_action(obs)
                result = env.step(action)
                last_result = result
                buffer.push(obs, action, result.reward, result.obs, result.done)

                obs = result.obs
                done = result.done
                ep_reward += result.reward
                global_step += 1

                if render and real_time:
                    time.sleep(step_dt)

                can_train = len(buffer) >= max(batch_size, warmup_steps)
                if can_train and global_step % train_every == 0:
                    batch = buffer.sample(batch_size)
                    loss = agent.train_step(*batch)
                    losses.append(loss)

            mean_loss = float(np.mean(losses)) if losses else 0.0
            episode_steps = last_result.episode_steps if last_result is not None else 0
            episode_collisions = last_result.episode_collisions if last_result is not None else 0
            episode_checkpoints = last_result.episode_checkpoints if last_result is not None else 0
            episode_laps = last_result.episode_laps if last_result is not None else 0
            episode_reward_from_sim = last_result.episode_reward if last_result is not None else ep_reward
            truncated = last_result.truncated if last_result is not None else False
            race_finished = last_result.race_finished if last_result is not None else False

            print(
                f"episode={episode:04d} "
                f"reward={ep_reward:.2f} "
                f"sim_reward={episode_reward_from_sim:.2f} "
                f"epsilon={agent.epsilon():.3f} "
                f"buffer={len(buffer)} "
                f"loss={mean_loss:.4f} "
                f"steps={episode_steps} "
                f"collisions={episode_collisions} "
                f"checkpoints={episode_checkpoints} "
                f"laps={episode_laps} "
                f"truncated={int(truncated)} "
                f"race_finished={int(race_finished)}"
            )

            # Keep the best policy seen so far (not just the final checkpoint).
            if last_result is not None:
                score = (
                    1000.0 * int(race_finished)
                    + 50.0 * float(episode_laps)
                    + 5.0 * float(episode_checkpoints)
                    + float(episode_reward_from_sim)
                    - 2.0 * float(episode_collisions)
                )
                if score > best_score:
                    best_score = score
                    no_improve_count = 0
                    agent.save(str(best_ckpt))
                    print(f"new_best score={best_score:.2f} checkpoint={best_ckpt}")
                else:
                    no_improve_count += 1

            metrics_writer.writerow(
                {
                    "episode": episode,
                    "reward": f"{ep_reward:.6f}",
                    "sim_reward": f"{episode_reward_from_sim:.6f}",
                    "epsilon": f"{agent.epsilon():.6f}",
                    "buffer": len(buffer),
                    "loss": f"{mean_loss:.6f}",
                    "steps": episode_steps,
                    "collisions": episode_collisions,
                    "checkpoints": episode_checkpoints,
                    "laps": episode_laps,
                    "truncated": int(truncated),
                    "race_finished": int(race_finished),
                    "best_score": f"{best_score:.6f}" if best_score != float("-inf") else "",
                    "global_step": global_step,
                }
            )
            metrics_file.flush()

            if early_stop_patience > 0 and no_improve_count >= early_stop_patience:
                print(f"early-stop triggered: no improvement for {no_improve_count} episodes")
                break

            if episode % save_every == 0:
                ckpt = out_dir / f"dqn_ep_{episode}.pt"
                agent.save(str(ckpt))

        final_ckpt = out_dir / "dqn_final.pt"
        agent.save(str(final_ckpt))
        print(f"saved final model: {final_ckpt}")
    finally:
        metrics_file.close()
        env.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Train DQN for speed_racer (rl_dqn)")
    parser.add_argument("--episodes", type=int, default=300)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--warmup-steps", type=int, default=2000)
    parser.add_argument("--train-every", type=int, default=4)
    parser.add_argument("--save-every", type=int, default=50)
    parser.add_argument("--max-steps", type=int, default=2000)
    parser.add_argument("--out-dir", type=Path, default=Path("rl_dqn/runs"))
    parser.add_argument("--resume", action="store_true", help="Resume training from out-dir/dqn_final.pt")
    parser.add_argument("--resume-from", type=Path, default=None, help="Checkpoint path to resume from")
    parser.add_argument("--lr", type=float, default=None, help="Global learning rate override when training from scratch")
    parser.add_argument("--resume-lr", type=float, default=None, help="Learning rate override when resuming from checkpoint")
    parser.add_argument("--early-stop-patience", type=int, default=0, help="Stop if no best-score improvement for N episodes (0 disables)")
    parser.add_argument("--metrics-csv", type=Path, default=None, help="CSV path for per-episode metrics (default: out-dir/metrics.csv)")
    parser.add_argument("--render", action="store_true", help="Render C++ simulation window during training")
    parser.add_argument("--step-dt", type=float, default=1.0 / 60.0, help="Simulation dt used for each RL step")
    parser.add_argument("--real-time", action="store_true", help="Sleep step_dt between steps when rendering")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()
    train(
        episodes=args.episodes,
        batch_size=args.batch_size,
        warmup_steps=args.warmup_steps,
        train_every=args.train_every,
        save_every=args.save_every,
        out_dir=args.out_dir,
        render=args.render,
        step_dt=args.step_dt,
        real_time=args.real_time,
        max_steps=args.max_steps,
        resume=args.resume,
        resume_from=args.resume_from,
        lr=args.lr,
        resume_lr=args.resume_lr,
        early_stop_patience=args.early_stop_patience,
        metrics_csv=args.metrics_csv,
    )
