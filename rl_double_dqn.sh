#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
VENV_PY="/Users/soufianesbai/Projets/.venv/bin/python"

if [[ ! -x "$VENV_PY" ]]; then
    echo "Python venv not found at: $VENV_PY"
    exit 1
fi

build_bridge() {
    cmake --build "$ROOT_DIR/build" --target speed_racer_rl_bridge
}

train_double_dqn() {
    local episodes="${1:-1000}"
    local save_every="${2:-50}"
    local max_steps="${3:-2000}"

    build_bridge
    "$VENV_PY" "$ROOT_DIR/rl_double_dqn/train.py" \
        --episodes "$episodes" \
        --save-every "$save_every" \
        --max-steps "$max_steps" \
        --out-dir "$ROOT_DIR/rl_double_dqn/runs"
}

resume_double_dqn() {
    local checkpoint="${1:-$ROOT_DIR/rl_double_dqn/runs/dqn_best.pt}"
    local episodes="${2:-1000}"
    local save_every="${3:-50}"
    local max_steps="${4:-2000}"
    local resume_lr="${5:-0.0003}"

    build_bridge
    "$VENV_PY" "$ROOT_DIR/rl_double_dqn/train.py" \
        --episodes "$episodes" \
        --save-every "$save_every" \
        --max-steps "$max_steps" \
        --resume-from "$checkpoint" \
        --resume-lr "$resume_lr" \
        --early-stop-patience 120 \
        --out-dir "$ROOT_DIR/rl_double_dqn/runs"
}

play_double_dqn() {
    local checkpoint="${1:-$ROOT_DIR/rl_double_dqn/runs/dqn_best.pt}"
    local episodes="${2:-1}"
    local max_steps="${3:-6000}"

    build_bridge
    "$VENV_PY" "$ROOT_DIR/rl_double_dqn/replay.py" \
        --checkpoint "$checkpoint" \
        --episodes "$episodes" \
        --max-steps "$max_steps" \
        --real-time
}

best_double_dqn() {
    local episodes="${1:-10}"
    local max_steps="${2:-2000}"

    build_bridge
    "$VENV_PY" "$ROOT_DIR/rl_double_dqn/evaluate.py" \
        --episodes "$episodes" \
        --max-steps "$max_steps" \
        --runs-dir "$ROOT_DIR/rl_double_dqn/runs"
}

    promote_double_dqn() {
    local episodes="${1:-10}"
    local max_steps="${2:-2000}"

    build_bridge
    "$VENV_PY" "$ROOT_DIR/rl_double_dqn/evaluate.py" \
        --episodes "$episodes" \
        --max-steps "$max_steps" \
        --runs-dir "$ROOT_DIR/rl_double_dqn/runs" \
        --promote-best
}

usage() {
    cat <<EOF
Usage: ./rl_double_dqn.sh <command> [args]

Commands:
  build
    Build C++ RL bridge

  train [episodes] [save_every] [max_steps]
    Train Double DQN from scratch (defaults: 1000 50 2000)

  resume [checkpoint] [episodes] [save_every] [max_steps] [resume_lr]
    Resume Double DQN (defaults: rl_double_dqn/runs/dqn_best.pt 1000 50 2000 0.0003)

  play [checkpoint] [episodes] [max_steps]
    Play a Double DQN model with rendering (defaults: rl_double_dqn/runs/dqn_best.pt 1 6000)

  best [episodes] [max_steps]
    Evaluate Double DQN checkpoints and print best (defaults: 10 2000)

  promote [episodes] [max_steps]
    Evaluate Double DQN checkpoints and copy best to rl_double_dqn/runs/dqn_best.pt (defaults: 10 2000)
EOF
}

main() {
    cd "$ROOT_DIR"

    local cmd="${1:-}"
    shift || true

    case "$cmd" in
        build)
            build_bridge
            ;;
        train)
            train_double_dqn "$@"
            ;;
        resume)
            resume_double_dqn "$@"
            ;;
        play)
            play_double_dqn "$@"
            ;;
        best)
            best_double_dqn "$@"
            ;;
        promote)
            promote_double_dqn "$@"
            ;;
        ""|help|-h|--help)
            usage
            ;;
        *)
            echo "Unknown command: $cmd"
            usage
            exit 1
            ;;
    esac
}

main "$@"
