#!/usr/bin/env python3
import argparse
import shlex
import subprocess
import sys
from pathlib import Path


def run(cmd):
    return subprocess.run(cmd, check=False, text=True, encoding="utf-8", errors="replace", capture_output=True)


def _normalize_name(text: str) -> str:
    return "".join(ch for ch in text.strip().lstrip("\ufeff").lower() if ch.isalnum() or ch in "-_")


def _clean_text(text: str) -> str:
    return "".join(ch for ch in text if ch.isprintable()).strip().lstrip("\ufeff")


def require_wsl(distro: str) -> str:
    probe = run(["wsl", "-l", "-q"])
    if probe.returncode != 0:
        msg = (probe.stderr or probe.stdout or "").strip()
        raise RuntimeError(
            "WSL is not ready. Install and reboot first.\n"
            "Suggested: wsl --install -d Ubuntu\n"
            f"Details: {msg}"
        )

    distros = []
    for line in probe.stdout.splitlines():
        name = _clean_text(line)
        if name:
            distros.append(name)

    if not distros:
        raise RuntimeError("No WSL distro found. Install one: wsl --install -d Ubuntu")

    target = _normalize_name(distro)
    for name in distros:
        if _normalize_name(name) == target:
            return name

    # If WSL reports at least one distro, prefer the first one rather than failing on formatting quirks.
    return distros[0]


def to_linux_path(distro: str, windows_path: Path) -> str:
    raw = str(windows_path).replace("\\", "/")
    if len(raw) >= 2 and raw[1] == ":":
        drive = raw[0].lower()
        rest = raw[2:].lstrip("/")
        return f"/mnt/{drive}/{rest}"
    raise RuntimeError(f"Unsupported Windows path format: {windows_path}")


def to_bash_path(workspace: Path, path_text: str) -> str:
    raw = path_text.replace("\\", "/")
    if len(raw) >= 2 and raw[1] == ":":
        return to_linux_path("", Path(raw))
    candidate = Path(raw)
    if candidate.is_absolute():
        return to_linux_path("", candidate)
    return raw


def _is_katago_wrapper(path_text: str) -> bool:
    lowered = path_text.replace("\\", "/").lower()
    return lowered.endswith("katago_wrapper.py") or lowered.endswith("katago_wrapper")


def prompt_mode() -> str:
    print("Select mode:")
    print("  1) teacher (alice vs katago)")
    print("  2) normal  (alice vs james)")
    while True:
        try:
            raw = input("Mode (1/2): ").strip()
        except EOFError:
            print()
            raise
        if raw == "1":
            return "teacher"
        if raw == "2":
            return "normal"
        print("Please enter exactly 1 or 2.")


def main() -> int:
    parser = argparse.ArgumentParser(description="Run ConnectSix arena through WSL")
    parser.add_argument("--rounds", "-r", type=int, help="Number of games")
    parser.add_argument("--no-compile-bots", action="store_true", help="Pass --no-compile to arena")
    parser.add_argument("--alice-source", default=None, help="Alice bot source file")
    parser.add_argument("--james-source", default=None, help="James bot source file")
    parser.add_argument("--james-time", type=int, default=None, help="James per-move time limit ms (default: 1000, KataGo can use 30000)")
    parser.add_argument("--distro", default="Ubuntu", help="WSL distro name, default: Ubuntu")
    args = parser.parse_args()

    mode = prompt_mode()

    rounds = args.rounds
    if rounds is None:
        try:
            raw = input("Rounds: ").strip()
        except EOFError:
            raw = ""
        if not raw:
            print("rounds must be a positive integer", file=sys.stderr)
            return 2
        try:
            rounds = int(raw)
        except ValueError:
            print("rounds must be a positive integer", file=sys.stderr)
            return 2

    if rounds <= 0:
        print("rounds must be a positive integer", file=sys.stderr)
        return 2

    try:
        selected_distro = require_wsl(args.distro)
        workspace = Path(__file__).resolve().parent
        linux_workspace = to_linux_path(selected_distro, workspace)
    except RuntimeError as ex:
        print(str(ex), file=sys.stderr)
        return 1

    no_compile = ["--no-compile"] if args.no_compile_bots else []

    alice_source = args.alice_source or "alice.cpp"
    if args.james_source:
        james_source = args.james_source
    elif mode == "teacher":
        james_source = "katago/katago_wrapper.py"
    else:
        james_source = "james.cpp"

    james_time_value = args.james_time
    if james_time_value is None and mode == "teacher":
        james_time_value = 30000

    james_time = ["--james-time", str(james_time_value)] if james_time_value is not None else []
    arena_args = [str(rounds)] + no_compile + james_time + [
        "--alice-source", to_bash_path(workspace, alice_source),
        "--james-source", to_bash_path(workspace, james_source),
    ]
    arena_command = "./arena " + " ".join(shlex.quote(item) for item in arena_args)

    # KataGo wrapper需要的环境变量（james-source是katago_wrapper相关时自动带上）
    env_setup = ""
    if _is_katago_wrapper(james_source):
        katago_bin = to_linux_path(selected_distro, workspace / "katago" / "katago.exe")
        katago_model = to_linux_path(selected_distro, workspace / "katago" / "model.bin.gz")
        katago_cfg = to_linux_path(selected_distro, workspace / "katago" / "katago_local.cfg")
        env_setup = (
            f"export KATAGO_BIN=${{KATAGO_BIN:-{katago_bin}}}\n"
            f"export KATAGO_MODEL=${{KATAGO_MODEL:-{katago_model}}}\n"
            f"export KATAGO_CFG=${{KATAGO_CFG:-{katago_cfg}}}\n"
        )

    shell_script = (
        "set -e\n"
        f"cd '{linux_workspace}'\n"
        "if [ ! -x arena ] || [ platform.cpp -nt arena ]; then\n"
        "  echo '[WSL] Building platform.cpp -> arena'\n"
        "  g++ -std=c++17 -O2 platform.cpp -o arena\n"
        "else\n"
        "  echo '[WSL] Using existing arena'\n"
        "fi\n"
        + env_setup +
        f"printf '%s\\n' {shlex.quote('[WSL] Starting: ' + arena_command)}\n"
        f"{arena_command}\n"
    )

    print(f"WSL distro: {selected_distro}")
    print(f"Workspace (Linux): {linux_workspace}")
    print(f"Mode: {mode}")
    print(f"Alice source: {alice_source}")
    print(f"James source: {james_source}")
    if james_time_value is not None:
        print(f"James time limit: {james_time_value} ms")
    sys.stdout.flush()

    proc = subprocess.run(["wsl", "-d", args.distro, "--", "bash", "-lc", shell_script], check=False)
    return proc.returncode


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (KeyboardInterrupt, EOFError):
        print("\nCancelled.")
        raise SystemExit(130)
