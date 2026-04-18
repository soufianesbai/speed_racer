from __future__ import annotations

from dataclasses import dataclass
import random

import numpy as np
import torch
from torch import nn
from torch.optim import Adam


class QNetwork(nn.Module):
    def __init__(self, obs_dim: int, action_dim: int) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(obs_dim, 256),
            nn.ReLU(),
            nn.Linear(256, 256),
            nn.ReLU(),
            nn.Linear(256, action_dim),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return self.net(x)


@dataclass
class DQNConfig:
    gamma: float = 0.99
    lr: float = 1e-3
    tau: float = 0.005
    eps_start: float = 1.0
    eps_end: float = 0.05
    eps_decay: int = 100_000


class DQNAgent:
    def __init__(
        self,
        obs_dim: int,
        action_dim: int,
        device: str = "cpu",
        config: DQNConfig | None = None,
    ) -> None:
        self.cfg = config or DQNConfig()
        self.action_dim = action_dim
        self.device = torch.device(device)

        self.q = QNetwork(obs_dim, action_dim).to(self.device)
        self.q_target = QNetwork(obs_dim, action_dim).to(self.device)
        self.q_target.load_state_dict(self.q.state_dict())
        self.optimizer = Adam(self.q.parameters(), lr=self.cfg.lr)

        self.steps = 0

    def epsilon(self) -> float:
        t = min(self.steps / float(self.cfg.eps_decay), 1.0)
        return self.cfg.eps_start + t * (self.cfg.eps_end - self.cfg.eps_start)

    @torch.no_grad()
    def select_action(self, obs: np.ndarray, evaluate: bool = False) -> int:
        self.steps += 1
        if (not evaluate) and random.random() < self.epsilon():
            return random.randrange(self.action_dim)

        obs_t = torch.as_tensor(obs, dtype=torch.float32, device=self.device).unsqueeze(0)
        q_values = self.q(obs_t)
        return int(torch.argmax(q_values, dim=1).item())

    def train_step(
        self,
        obs: np.ndarray,
        actions: np.ndarray,
        rewards: np.ndarray,
        next_obs: np.ndarray,
        dones: np.ndarray,
    ) -> float:
        obs_t = torch.as_tensor(obs, dtype=torch.float32, device=self.device)
        actions_t = torch.as_tensor(actions, dtype=torch.int64, device=self.device)
        rewards_t = torch.as_tensor(rewards, dtype=torch.float32, device=self.device)
        next_obs_t = torch.as_tensor(next_obs, dtype=torch.float32, device=self.device)
        dones_t = torch.as_tensor(dones, dtype=torch.float32, device=self.device)

        q_values = self.q(obs_t).gather(1, actions_t.unsqueeze(1)).squeeze(1)

        with torch.no_grad():
            next_q = self.q_target(next_obs_t).max(dim=1).values
            target = rewards_t + self.cfg.gamma * (1.0 - dones_t) * next_q

        loss = nn.functional.smooth_l1_loss(q_values, target)

        self.optimizer.zero_grad(set_to_none=True)
        loss.backward()
        nn.utils.clip_grad_norm_(self.q.parameters(), max_norm=10.0)
        self.optimizer.step()

        self.soft_update()
        return float(loss.item())

    def soft_update(self) -> None:
        tau = self.cfg.tau
        for p, p_tgt in zip(self.q.parameters(), self.q_target.parameters()):
            p_tgt.data.copy_(tau * p.data + (1.0 - tau) * p_tgt.data)

    def save(self, path: str) -> None:
        torch.save(self.q.state_dict(), path)

    def load(self, path: str) -> None:
        self.q.load_state_dict(torch.load(path, map_location=self.device))
        self.q_target.load_state_dict(self.q.state_dict())

    def set_lr(self, lr: float) -> None:
        self.cfg.lr = float(lr)
        for group in self.optimizer.param_groups:
            group["lr"] = self.cfg.lr
