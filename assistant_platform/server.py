from __future__ import annotations

import argparse
import json
import mimetypes
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlparse

from connectsix_engine import (
    BLACK,
    EMPTY,
    GameError,
    Move,
    Point,
    analyze_game,
    apply_stones,
    board_to_dict,
    build_board,
    coerce_moves,
    coerce_point,
    empty_board,
    expected_stone_count,
    load_knowledge,
    move_to_dict,
    parse_botzone_log,
    recommend_move,
    validate_move,
    winner,
)


ROOT = Path(__file__).resolve().parent
STATIC_ROOT = ROOT / "static"


class AssistantHandler(BaseHTTPRequestHandler):
    server_version = "ConnectSixAssistant/0.1"

    def log_message(self, fmt: str, *args: object) -> None:
        sys.stdout.write("[%s] %s\n" % (self.log_date_time_string(), fmt % args))

    def _send_bytes(self, payload: bytes, status: int = 200, content_type: str = "application/octet-stream") -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Access-Control-Allow-Methods", "GET,POST,OPTIONS")
        self.end_headers()
        self.wfile.write(payload)

    def _send_json(self, data: object, status: int = 200) -> None:
        payload = json.dumps(data, ensure_ascii=False, indent=2).encode("utf-8")
        self._send_bytes(payload, status=status, content_type="application/json; charset=utf-8")

    def _read_json(self) -> object:
        length = int(self.headers.get("Content-Length", "0") or "0")
        raw = self.rfile.read(length) if length else b"{}"
        if not raw:
            return {}
        return json.loads(raw.decode("utf-8"))

    def do_OPTIONS(self) -> None:
        self._send_bytes(b"", status=204)

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path
        if path == "/api/health":
            self._send_json({"ok": True, "service": "connectsix-assistant", "time": int(time.time())})
            return
        if path == "/api/knowledge":
            self._send_json(load_knowledge())
            return
        if path == "/":
            self._serve_static("index.html")
            return
        if path in {"/app.js", "/styles.css"}:
            self._serve_static(path.removeprefix("/"))
            return
        if path.startswith("/static/"):
            self._serve_static(path.removeprefix("/static/"))
            return
        self._send_json({"error": "not_found"}, status=404)

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        try:
            payload = self._read_json()
            if parsed.path == "/api/analyze":
                self._send_json(self._handle_analyze(payload))
            elif parsed.path == "/api/play":
                self._send_json(self._handle_play(payload))
            elif parsed.path == "/api/import-botzone":
                self._send_json(self._handle_import_botzone(payload))
            else:
                self._send_json({"error": "not_found"}, status=404)
        except GameError as exc:
            self._send_json({"error": "game_error", "message": str(exc)}, status=400)
        except json.JSONDecodeError as exc:
            self._send_json({"error": "bad_json", "message": str(exc)}, status=400)
        except Exception as exc:  # Keep the browser useful during prototype work.
            self._send_json({"error": "server_error", "message": str(exc)}, status=500)

    def _serve_static(self, rel_path: str) -> None:
        rel_path = unquote(rel_path).replace("\\", "/")
        target = (STATIC_ROOT / rel_path).resolve()
        try:
            target.relative_to(STATIC_ROOT.resolve())
        except ValueError:
            self._send_json({"error": "bad_path"}, status=400)
            return
        if not target.is_file():
            self._send_json({"error": "not_found"}, status=404)
            return
        content_type = mimetypes.guess_type(str(target))[0] or "application/octet-stream"
        if target.suffix == ".js":
            content_type = "text/javascript; charset=utf-8"
        elif target.suffix in {".html", ".css"}:
            content_type = f"text/{target.suffix[1:]}; charset=utf-8"
        self._send_bytes(target.read_bytes(), content_type=content_type)

    def _handle_analyze(self, payload: object) -> dict[str, object]:
        if not isinstance(payload, dict):
            raise GameError("请求体必须是对象")
        moves = coerce_moves(payload.get("moves", []))
        build_board(moves, validate=True)
        current_player = payload.get("current_player")
        current = int(current_player) if current_player is not None else None
        include_llm = bool(payload.get("include_llm", False))
        return analyze_game(moves, current_player=current, include_llm=include_llm)

    def _handle_play(self, payload: object) -> dict[str, object]:
        if not isinstance(payload, dict):
            raise GameError("请求体必须是对象")
        moves = coerce_moves(payload.get("moves", []))
        board = build_board(moves, validate=True)
        player = int(payload.get("player", BLACK if not moves else -moves[-1].player))
        stones_raw = payload.get("stones", [])
        if not isinstance(stones_raw, list):
            raise GameError("stones 必须是数组")
        stones = [coerce_point(item) for item in stones_raw]
        validate_move(board, player, stones)

        human_move = Move(player=player, stones=stones, source="human", ts=time.time())
        apply_stones(board, player, stones)
        next_moves = moves + [human_move]
        win = winner(board, human_move)
        human_analysis = analyze_game(next_moves, current_player=-player, include_llm=False)
        include_llm = bool(payload.get("include_llm", False))

        if win != EMPTY:
            return {
                "moves": [move_to_dict(move) for move in next_moves],
                "board": board_to_dict(board),
                "winner": win,
                "current_player": EMPTY,
                "last_bot_move": None,
                "human_analysis": human_analysis,
                "analysis": analyze_game(next_moves, current_player=-player, include_llm=include_llm),
            }

        bot_player = -player
        bot_plan = recommend_move(board, bot_player)
        bot_stones = [Point(int(item["x"]), int(item["y"])) for item in bot_plan.get("stones", [])]
        validate_move(board, bot_player, bot_stones)
        bot_move = Move(player=bot_player, stones=bot_stones, source="robot", note=bot_plan["reason"], ts=time.time())
        apply_stones(board, bot_player, bot_stones)
        final_moves = next_moves + [bot_move]
        win = winner(board, bot_move)
        current_player = EMPTY if win != EMPTY else player
        analysis = analyze_game(final_moves, current_player=player, include_llm=include_llm)
        return {
            "moves": [move_to_dict(move) for move in final_moves],
            "board": board_to_dict(board),
            "winner": win,
            "current_player": current_player,
            "last_bot_move": move_to_dict(bot_move),
            "human_analysis": human_analysis,
            "analysis": analysis,
        }

    def _handle_import_botzone(self, payload: object) -> dict[str, object]:
        entries: object
        if isinstance(payload, list):
            entries = payload
        elif isinstance(payload, dict):
            entries = payload.get("entries", payload.get("log", []))
        else:
            raise GameError("Botzone 日志必须是数组或包含 entries/log 的对象")
        if not isinstance(entries, list):
            raise GameError("Botzone entries/log 必须是数组")
        moves = parse_botzone_log(entries)
        board = build_board(moves, validate=False) if moves else empty_board()
        current_player = BLACK if not moves else -moves[-1].player
        return {
            "moves": [move_to_dict(move) for move in moves],
            "board": board_to_dict(board),
            "current_player": current_player,
            "analysis": analyze_game(moves, current_player=current_player, include_llm=False),
        }


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the ConnectSix assistant platform")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()

    server = ThreadingHTTPServer((args.host, args.port), AssistantHandler)
    print(f"ConnectSix assistant running at http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping.")
    finally:
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
