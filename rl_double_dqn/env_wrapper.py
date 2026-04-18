from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import subprocess
from typing import List

import numpy as np


ACTION_SPACE = 7


@dataclass
class StepResult:
    obs: np.ndarray
    reward: float
    done: bool
    truncated: bool
    collided: bool
    checkpoint_reached: bool
    lap_finished: bool
    race_finished: bool
    episode_steps: int
    episode_collisions: int
    episode_checkpoints: int
    episode_laps: int
    episode_reward: float


class SpeedRacerEnv:
    """Python wrapper over the C++ headless simulator bridge process."""

    def __init__(
        self,
        max_steps: int = 2_000,
        bridge_path: str | None = None,
        render: bool = False,
        step_dt: float = 1.0 / 60.0,
    ) -> None:
        self.max_steps = max_steps
        self.step_count = 0
        self.step_dt = float(step_dt)
        self.render = render

        root = Path(__file__).resolve().parents[1]
        default_bridge = root / "build" / "speed_racer_rl_bridge"
        self.bridge_path = Path(bridge_path) if bridge_path else default_bridge

        if not self.bridge_path.exists():
            raise FileNotFoundError(
                f"Bridge executable not found: {self.bridge_path}\n"
                f"Build it with: cmake -S {root} -B {root / 'build'} && cmake --build {root / 'build'} --target speed_racer_rl_bridge"
            )

        command = [str(self.bridge_path)]
        if self.render:
            command.append("--render")

        self._proc = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

        ready = self._read_protocol_line(("READY ",))
        if not ready.startswith("READY "):
            raise RuntimeError(f"Unexpected bridge handshake: {ready}")

        self._send_line(f"SET_MAX_STEPS {self.max_steps}")
        ack = self._read_protocol_line(("OK",))
        if ack != "OK":
            raise RuntimeError(f"Unexpected SET_MAX_STEPS response: {ack}")

        self.obs = self._parse_obs_payload(ready.split(maxsplit=1)[1])
        self._obs_dim = int(self.obs.shape[0])

    @property
    def observation_dim(self) -> int:
        return self._obs_dim

    @property
    def action_dim(self) -> int:
        return ACTION_SPACE

    def reset(self) -> np.ndarray:
        self.step_count = 0
        self._send_line("RESET")
        line = self._read_protocol_line(("OBS ",))
        if not line.startswith("OBS "):
            raise RuntimeError(f"Unexpected RESET response: {line}")

        self.obs = self._parse_obs_payload(line.split(maxsplit=1)[1])
        return self.obs.copy()

    def step(self, action: int) -> StepResult:
        self.step_count += 1
        action = int(np.clip(action, 0, ACTION_SPACE - 1))

        self._send_line(f"STEP {action} {self.step_dt:.6f}")
        line = self._read_protocol_line(("STEP ",))
        if not line.startswith("STEP "):
            raise RuntimeError(f"Unexpected STEP response: {line}")

        tokens = line.strip().split()
        # STEP reward done truncated collided cp lap race steps collisions checkpoints laps episode_reward obs_dim obs...
        if len(tokens) < 15:
            raise RuntimeError(f"Malformed STEP response: {line}")

        reward = float(tokens[1])
        done_from_sim = tokens[2] == "1"
        truncated = tokens[3] == "1"
        collided = tokens[4] == "1"
        checkpoint_reached = tokens[5] == "1"
        lap_finished = tokens[6] == "1"
        race_finished = tokens[7] == "1"
        episode_steps = int(tokens[8])
        episode_collisions = int(tokens[9])
        episode_checkpoints = int(tokens[10])
        episode_laps = int(tokens[11])
        episode_reward = float(tokens[12])
        obs_dim = int(tokens[13])
        if len(tokens) != 14 + obs_dim:
            raise RuntimeError(f"Malformed STEP obs size: expected {obs_dim}, got {len(tokens) - 14}")

        obs_values = np.asarray([float(v) for v in tokens[14:]], dtype=np.float32)
        self.obs = obs_values

        return StepResult(
            obs=self.obs.copy(),
            reward=reward,
            done=done_from_sim,
            truncated=truncated,
            collided=collided,
            checkpoint_reached=checkpoint_reached,
            lap_finished=lap_finished,
            race_finished=race_finished,
            episode_steps=episode_steps,
            episode_collisions=episode_collisions,
            episode_checkpoints=episode_checkpoints,
            episode_laps=episode_laps,
            episode_reward=episode_reward,
        )

    def close(self) -> None:
        if self._proc is None:
            return

        if self._proc.poll() is None:
            try:
                self._send_line("CLOSE")
            except Exception:
                pass

            try:
                self._proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                self._proc.kill()

        self._proc = None

    def _send_line(self, line: str) -> None:
        if self._proc is None or self._proc.stdin is None:
            raise RuntimeError("Bridge process is not available")
        self._proc.stdin.write(line + "\n")
        self._proc.stdin.flush()

    def _read_line(self) -> str:
        if self._proc is None or self._proc.stdout is None:
            raise RuntimeError("Bridge process is not available")

        line = self._proc.stdout.readline()
        if line:
            return line.strip()

        stderr_text = ""
        if self._proc.stderr is not None:
            stderr_text = self._proc.stderr.read().strip()
        raise RuntimeError(f"Bridge process ended unexpectedly. stderr={stderr_text}")

    def _read_protocol_line(self, expected_prefixes: tuple[str, ...]) -> str:
        while True:
            line = self._read_line()
            if line.startswith(expected_prefixes):
                return line
            # Ignore non-protocol logs that may leak from native libraries.
            if line.startswith("INFO:") or line.startswith("WARNING:"):
                continue
            return line

    @staticmethod
    def _parse_obs_payload(payload: str) -> np.ndarray:
        tokens: List[str] = payload.strip().split()
        if not tokens:
            raise RuntimeError("Empty observation payload")

        obs_dim = int(tokens[0])
        if len(tokens) != 1 + obs_dim:
            raise RuntimeError(f"Malformed observation payload: expected {obs_dim} values, got {len(tokens) - 1}")

        return np.asarray([float(v) for v in tokens[1:]], dtype=np.float32)

    def __del__(self) -> None:
        try:
            self.close()
        except Exception:
            pass
