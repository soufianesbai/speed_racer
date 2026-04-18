from __future__ import annotations

from collections import deque
from dataclasses import dataclass
import random
from typing import Deque, Tuple

import numpy as np


@dataclass
class Transition:
    obs: np.ndarray
    action: int
    reward: float
    next_obs: np.ndarray
    done: bool


class ReplayBuffer:
    def __init__(self, capacity: int = 100_000) -> None:
        self._buffer: Deque[Transition] = deque(maxlen=capacity)

    def __len__(self) -> int:
        return len(self._buffer)

    def push(
        self,
        obs: np.ndarray,
        action: int,
        reward: float,
        next_obs: np.ndarray,
        done: bool,
    ) -> None:
        self._buffer.append(
            Transition(
                obs=np.asarray(obs, dtype=np.float32),
                action=int(action),
                reward=float(reward),
                next_obs=np.asarray(next_obs, dtype=np.float32),
                done=bool(done),
            )
        )

    def sample(self, batch_size: int) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        batch = random.sample(self._buffer, batch_size)
        obs = np.stack([t.obs for t in batch], axis=0)
        actions = np.array([t.action for t in batch], dtype=np.int64)
        rewards = np.array([t.reward for t in batch], dtype=np.float32)
        next_obs = np.stack([t.next_obs for t in batch], axis=0)
        dones = np.array([t.done for t in batch], dtype=np.float32)
        return obs, actions, rewards, next_obs, dones
