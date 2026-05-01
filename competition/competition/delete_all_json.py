from pathlib import Path
import sys


def delete_json_files(directory: Path) -> int:
    deleted = 0
    for path in directory.rglob("*.json"):
        if path.is_file():
            path.unlink()
            deleted += 1
    return deleted


def main() -> int:
    base_dir = Path(__file__).resolve().parent
    target = base_dir / "result"

    if not target.exists():
        print(f"[INFO] Directory not found: {target}")
        return 0

    if not target.is_dir():
        print(f"[ERROR] Target exists but is not a directory: {target}")
        return 1

    try:
        deleted = delete_json_files(target)
        print(f"[OK] Deleted {deleted} JSON file(s) in: {target}")
        return 0
    except Exception as exc:
        print(f"[ERROR] Failed to delete JSON files in: {target}")
        print(f"Reason: {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
