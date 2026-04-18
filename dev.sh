#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"

command -v cmake >/dev/null 2>&1 || {
    echo "Error: cmake is not installed or not in PATH."
    exit 1
}

detect_prefix() {
    if [[ -n "${CMAKE_PREFIX_PATH:-}" ]]; then
        printf '%s' "$CMAKE_PREFIX_PATH"
        return
    fi

    if command -v brew >/dev/null 2>&1; then
        brew --prefix
        return
    fi

    printf '%s' "/opt/homebrew"
}

configure() {
    local prefix
    prefix="$(detect_prefix)"

    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_PREFIX_PATH="$prefix"
}

build() {
    cmake --build "$BUILD_DIR"
}

run_game() {
    local executable="$BUILD_DIR/speed_racer"

    if [[ ! -x "$executable" ]]; then
        echo "No executable found at $executable"
        echo "Create main.cpp and rebuild before using the run command."
        exit 1
    fi

    exec "$executable"
}

clean() {
    rm -rf "$BUILD_DIR"
}

usage() {
    cat <<'EOF'
Usage: ./dev.sh [command]

Commands:
  configure  Configure the CMake project into ./build
  build      Build the project from ./build
  run        Run the speed_racer executable if it exists
  start      Configure, build, then run
  clean      Remove the ./build directory
  rebuild    Clean, configure, then build

Default behavior with no command: configure, then build
EOF
}

case "${1:-default}" in
    configure)
        configure
        ;;
    build)
        build
        ;;
    run)
        run_game
        ;;
    start)
        configure
        build
        run_game
        ;;
    clean)
        clean
        ;;
    rebuild)
        clean
        configure
        build
        ;;
    help|-h|--help|usage)
        usage
        ;;
    default)
        configure
        build
        ;;
    *)
        echo "Unknown command: $1"
        usage
        exit 1
        ;;
esac