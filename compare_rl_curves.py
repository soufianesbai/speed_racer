from __future__ import annotations

import argparse
import csv
from pathlib import Path
from typing import Iterable

import matplotlib.pyplot as plt


def load_metrics(path: Path) -> list[dict[str, float]]:
    if not path.exists():
        raise FileNotFoundError(f"metrics file not found: {path}")

    rows: list[dict[str, float]] = []
    with path.open("r", newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            parsed: dict[str, float] = {}
            for key, value in row.items():
                if value is None or value == "":
                    continue
                try:
                    parsed[key] = float(value)
                except ValueError:
                    continue
            rows.append(parsed)
    return rows


def moving_average(values: Iterable[float], window: int) -> list[float]:
    values_list = list(values)
    if window <= 1 or len(values_list) <= 2:
        return values_list

    smoothed: list[float] = []
    for index in range(len(values_list)):
        start = max(0, index - window + 1)
        chunk = values_list[start : index + 1]
        smoothed.append(sum(chunk) / len(chunk))
    return smoothed


def extract_series(rows: list[dict[str, float]], key: str) -> list[float]:
    series: list[float] = []
    for row in rows:
        if key in row:
            series.append(row[key])
    return series


def plot_metric(ax, x_a, series_a, label_a, x_b, series_b, label_b, title, ylabel, window):
    if series_a:
        ax.plot(x_a[: len(series_a)], moving_average(series_a, window), label=label_a, linewidth=2)
    if series_b:
        ax.plot(x_b[: len(series_b)], moving_average(series_b, window), label=label_b, linewidth=2)
    ax.set_title(title)
    ax.set_xlabel("Episode")
    ax.set_ylabel(ylabel)
    ax.grid(True, alpha=0.3)
    ax.legend()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot training curves for rl_dqn vs rl_double_dqn")
    parser.add_argument("--baseline", type=Path, default=Path("rl_dqn/runs/metrics.csv"), help="Baseline metrics.csv path")
    parser.add_argument("--double-dqn", type=Path, default=Path("rl_double_dqn/runs/metrics.csv"), help="Double DQN metrics.csv path")
    parser.add_argument("--output", type=Path, default=Path("rl_comparison.png"), help="Output plot path")
    parser.add_argument("--window", type=int, default=25, help="Moving average window")
    parser.add_argument("--show", action="store_true", help="Display the figure after saving")
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    baseline_rows = load_metrics(args.baseline)
    double_dqn_rows = load_metrics(args.double_dqn)

    baseline_episodes = extract_series(baseline_rows, "episode")
    double_dqn_episodes = extract_series(double_dqn_rows, "episode")

    fig, axes = plt.subplots(2, 2, figsize=(14, 9), constrained_layout=True)
    fig.suptitle("Speed Racer RL Comparison", fontsize=16)

    plot_metric(
        axes[0, 0],
        baseline_episodes,
        extract_series(baseline_rows, "reward"),
        "rl_dqn",
        double_dqn_episodes,
        extract_series(double_dqn_rows, "reward"),
        "rl_double_dqn",
        "Training Reward",
        "Reward",
        args.window,
    )
    plot_metric(
        axes[0, 1],
        baseline_episodes,
        extract_series(baseline_rows, "loss"),
        "rl_dqn",
        double_dqn_episodes,
        extract_series(double_dqn_rows, "loss"),
        "rl_double_dqn",
        "Training Loss",
        "Loss",
        args.window,
    )
    plot_metric(
        axes[1, 0],
        baseline_episodes,
        extract_series(baseline_rows, "checkpoints"),
        "rl_dqn",
        double_dqn_episodes,
        extract_series(double_dqn_rows, "checkpoints"),
        "rl_double_dqn",
        "Checkpoints Reached",
        "Count",
        args.window,
    )
    plot_metric(
        axes[1, 1],
        baseline_episodes,
        extract_series(baseline_rows, "laps"),
        "rl_dqn",
        double_dqn_episodes,
        extract_series(double_dqn_rows, "laps"),
        "rl_double_dqn",
        "Laps Completed",
        "Count",
        args.window,
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=160)
    print(f"saved comparison plot: {args.output}")

    if args.show:
        plt.show()


if __name__ == "__main__":
    main()