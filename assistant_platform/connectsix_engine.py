from __future__ import annotations

import copy
import json
import math
import os
import re
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any


BOARD_SIZE = 15
EMPTY = 0
BLACK = 1
WHITE = -1
PLAYERS = {BLACK, WHITE}
DIRECTIONS = ((1, 0), (0, 1), (1, 1), (1, -1))
PLAYER_NAMES = {BLACK: "黑棋", WHITE: "白棋"}


@dataclass(frozen=True)
class Point:
    x: int
    y: int


@dataclass
class Move:
    player: int
    stones: list[Point]
    source: str = "human"
    note: str = ""
    ts: float = 0.0


class GameError(ValueError):
    pass


def empty_board(size: int = BOARD_SIZE) -> list[list[int]]:
    return [[EMPTY for _ in range(size)] for _ in range(size)]


def in_board(x: int, y: int, size: int = BOARD_SIZE) -> bool:
    return 0 <= x < size and 0 <= y < size


def point_to_dict(point: Point) -> dict[str, int]:
    return {"x": point.x, "y": point.y}


def move_to_dict(move: Move) -> dict[str, Any]:
    data: dict[str, Any] = {
        "player": move.player,
        "player_name": PLAYER_NAMES.get(move.player, "未知"),
        "stones": [point_to_dict(p) for p in move.stones],
        "source": move.source,
    }
    if move.note:
        data["note"] = move.note
    if move.ts:
        data["ts"] = move.ts
    return data


def board_to_dict(board: list[list[int]]) -> list[list[int]]:
    return [row[:] for row in board]


def coerce_point(raw: Any) -> Point:
    if isinstance(raw, Point):
        return raw
    if not isinstance(raw, dict):
        raise GameError("落点必须是对象，包含 x/y 坐标")
    try:
        return Point(int(raw["x"]), int(raw["y"]))
    except (KeyError, TypeError, ValueError) as exc:
        raise GameError("落点必须包含整数 x/y 坐标") from exc


def _stones_from_legacy_move(raw: dict[str, Any]) -> list[Point]:
    stones: list[Point] = []
    for idx in (0, 1):
        x_key = f"x{idx}"
        y_key = f"y{idx}"
        if x_key in raw and y_key in raw:
            x = int(raw[x_key])
            y = int(raw[y_key])
            if x >= 0 and y >= 0:
                stones.append(Point(x, y))
    return stones


def coerce_move(raw: Any, fallback_player: int | None = None) -> Move:
    if isinstance(raw, Move):
        return raw
    if not isinstance(raw, dict):
        raise GameError("棋步必须是对象")

    player = int(raw.get("player", fallback_player if fallback_player is not None else BLACK))
    if player not in PLAYERS:
        raise GameError("player 只能是 1(黑棋) 或 -1(白棋)")

    if "stones" in raw:
        stones = [coerce_point(item) for item in raw.get("stones", [])]
    else:
        stones = _stones_from_legacy_move(raw)

    source = str(raw.get("source", "human"))
    note = str(raw.get("note", ""))
    ts = float(raw.get("ts", 0.0) or 0.0)
    return Move(player=player, stones=stones, source=source, note=note, ts=ts)


def coerce_moves(raw_moves: Any) -> list[Move]:
    if raw_moves is None:
        return []
    if not isinstance(raw_moves, list):
        raise GameError("moves 必须是数组")
    moves: list[Move] = []
    next_player = BLACK
    for raw in raw_moves:
        move = coerce_move(raw, next_player)
        moves.append(move)
        next_player = -move.player
    return moves


def board_stone_count(board: list[list[int]]) -> int:
    return sum(1 for row in board for value in row if value != EMPTY)


def expected_stone_count(board: list[list[int]], player: int) -> int:
    if board_stone_count(board) == 0 and player == BLACK:
        return 1
    return 2


def apply_stones(board: list[list[int]], player: int, stones: list[Point]) -> None:
    for stone in stones:
        board[stone.y][stone.x] = player


def validate_move(board: list[list[int]], player: int, stones: list[Point]) -> None:
    if player not in PLAYERS:
        raise GameError("player 只能是 1(黑棋) 或 -1(白棋)")
    expected = expected_stone_count(board, player)
    if len(stones) != expected:
        raise GameError(f"{PLAYER_NAMES[player]}本手需要落 {expected} 子")

    seen: set[tuple[int, int]] = set()
    for stone in stones:
        if not in_board(stone.x, stone.y, len(board)):
            raise GameError(f"落点 ({stone.x}, {stone.y}) 超出棋盘")
        key = (stone.x, stone.y)
        if key in seen:
            raise GameError("同一手不能重复落在同一坐标")
        if board[stone.y][stone.x] != EMPTY:
            raise GameError(f"落点 ({stone.x}, {stone.y}) 已有棋子")
        seen.add(key)


def build_board(moves: list[Move], validate: bool = False) -> list[list[int]]:
    board = empty_board()
    for move in moves:
        if validate:
            validate_move(board, move.player, move.stones)
        apply_stones(board, move.player, move.stones)
    return board


def count_direction(board: list[list[int]], x: int, y: int, player: int, dx: int, dy: int) -> int:
    size = len(board)
    count = 0
    cx = x + dx
    cy = y + dy
    while in_board(cx, cy, size) and board[cy][cx] == player:
        count += 1
        cx += dx
        cy += dy
    return count


def line_profile(board: list[list[int]], x: int, y: int, player: int, dx: int, dy: int) -> dict[str, int]:
    size = len(board)
    left = count_direction(board, x, y, player, -dx, -dy)
    right = count_direction(board, x, y, player, dx, dy)
    count = 1 + left + right

    left_end_x = x - (left + 1) * dx
    left_end_y = y - (left + 1) * dy
    right_end_x = x + (right + 1) * dx
    right_end_y = y + (right + 1) * dy
    open_ends = 0
    if in_board(left_end_x, left_end_y, size) and board[left_end_y][left_end_x] == EMPTY:
        open_ends += 1
    if in_board(right_end_x, right_end_y, size) and board[right_end_y][right_end_x] == EMPTY:
        open_ends += 1
    return {"count": count, "open_ends": open_ends}


def line_value(count: int, open_ends: int) -> int:
    if count >= 6:
        return 120_000
    if count == 5:
        return 55_000 if open_ends else 34_000
    if count == 4:
        return 14_000 if open_ends == 2 else 5_200 if open_ends == 1 else 1_000
    if count == 3:
        return 2_800 if open_ends == 2 else 900 if open_ends == 1 else 120
    if count == 2:
        return 420 if open_ends == 2 else 130 if open_ends == 1 else 30
    return 18 if open_ends == 2 else 6


def pattern_labels(profiles: list[dict[str, int]], player_text: str) -> list[str]:
    labels: list[str] = []
    counts = [item["count"] for item in profiles]
    open_counts = [item["open_ends"] for item in profiles]
    if any(count >= 6 for count in counts):
        labels.append(f"{player_text}成六")
    if any(count == 5 and open_ends > 0 for count, open_ends in zip(counts, open_counts)):
        labels.append(f"{player_text}五连威胁")
    live_four = sum(1 for count, open_ends in zip(counts, open_counts) if count == 4 and open_ends == 2)
    strong_lines = sum(1 for count, open_ends in zip(counts, open_counts) if count >= 4 and open_ends > 0)
    if live_four:
        labels.append(f"{player_text}活四")
    if strong_lines >= 2:
        labels.append(f"{player_text}双线")
    return labels


def point_score(board: list[list[int]], x: int, y: int, player: int) -> dict[str, Any] | None:
    if not in_board(x, y, len(board)) or board[y][x] != EMPTY:
        return None

    attack_profiles = [line_profile(board, x, y, player, dx, dy) for dx, dy in DIRECTIONS]
    defense_profiles = [line_profile(board, x, y, -player, dx, dy) for dx, dy in DIRECTIONS]
    attack = sum(line_value(item["count"], item["open_ends"]) for item in attack_profiles)
    defense = sum(line_value(item["count"], item["open_ends"]) for item in defense_profiles)

    attack_labels = pattern_labels(attack_profiles, "进攻")
    defense_labels = pattern_labels(defense_profiles, "防守")
    if any(item["count"] >= 6 for item in defense_profiles):
        defense += 35_000
        defense_labels.append("必须防守")

    center = (len(board) - 1) / 2
    dist = abs(x - center) + abs(y - center)
    center_bonus = max(0, int(120 - dist * 12))

    neighbor_bonus = 0
    for yy in range(max(0, y - 2), min(len(board), y + 3)):
        for xx in range(max(0, x - 2), min(len(board), x + 3)):
            if xx == x and yy == y:
                continue
            if board[yy][xx] == player:
                neighbor_bonus += 24
            elif board[yy][xx] == -player:
                neighbor_bonus += 12

    total = int(attack + defense * 0.94 + center_bonus + neighbor_bonus)
    labels = attack_labels + defense_labels
    if center_bonus >= 80:
        labels.append("中腹效率")
    if neighbor_bonus >= 80:
        labels.append("局部连接")
    if not labels:
        labels.append("稳健扩张")

    return {
        "x": x,
        "y": y,
        "score": total,
        "attack": int(attack),
        "defense": int(defense),
        "center": center_bonus,
        "labels": labels[:5],
        "max_attack_line": max(item["count"] for item in attack_profiles),
        "max_defense_line": max(item["count"] for item in defense_profiles),
    }


def heatmap_for(board: list[list[int]], player: int) -> list[dict[str, Any]]:
    cells: list[dict[str, Any]] = []
    for y in range(len(board)):
        for x in range(len(board)):
            scored = point_score(board, x, y, player)
            if scored:
                cells.append(scored)
    if not cells:
        return []

    min_score = min(cell["score"] for cell in cells)
    max_score = max(cell["score"] for cell in cells)
    span = max(1, max_score - min_score)
    for cell in cells:
        heat = (cell["score"] - min_score) / span
        cell["heat"] = round(heat, 4)
        if "必须防守" in cell["labels"] or "进攻成六" in cell["labels"]:
            cell["level"] = "critical"
        elif heat >= 0.72:
            cell["level"] = "strong"
        elif heat >= 0.42:
            cell["level"] = "useful"
        else:
            cell["level"] = "quiet"
    return cells


def makes_win_after(board: list[list[int]], player: int, stones: list[Point]) -> bool:
    trial = copy.deepcopy(board)
    apply_stones(trial, player, stones)
    return any(is_win_from(trial, stone.x, stone.y, player) for stone in stones)


def is_win_from(board: list[list[int]], x: int, y: int, player: int) -> bool:
    return any(1 + count_direction(board, x, y, player, dx, dy) +
               count_direction(board, x, y, player, -dx, -dy) >= 6
               for dx, dy in DIRECTIONS)


def winner(board: list[list[int]], last_move: Move | None = None) -> int:
    if last_move:
        for stone in last_move.stones:
            if is_win_from(board, stone.x, stone.y, last_move.player):
                return last_move.player
        return EMPTY

    for y, row in enumerate(board):
        for x, value in enumerate(row):
            if value != EMPTY and is_win_from(board, x, y, value):
                return value
    return EMPTY


def recommend_move(board: list[list[int]], player: int) -> dict[str, Any]:
    count = expected_stone_count(board, player)
    scored = sorted(heatmap_for(board, player), key=lambda item: item["score"], reverse=True)
    if not scored:
        return {"stones": [], "score": 0, "reason": "棋盘已满", "candidates": []}
    if count == 1:
        best = scored[0]
        return {
            "stones": [{"x": best["x"], "y": best["y"]}],
            "score": best["score"],
            "reason": explain_recommendation([best], player),
            "candidates": scored[:10],
        }

    first_pool = scored[: min(12, len(scored))]
    best_pair: tuple[int, list[dict[str, Any]]] | None = None
    for first in first_pool:
        trial = copy.deepcopy(board)
        p1 = Point(first["x"], first["y"])
        apply_stones(trial, player, [p1])
        second_pool = [
            item for item in sorted(heatmap_for(trial, player), key=lambda item: item["score"], reverse=True)
            if not (item["x"] == p1.x and item["y"] == p1.y)
        ][:8]
        for second in second_pool:
            p2 = Point(second["x"], second["y"])
            pair_score = int(first["score"] + second["score"] * 0.96)
            if p1.x == p2.x or p1.y == p2.y or abs(p1.x - p2.x) == abs(p1.y - p2.y):
                pair_score += 850
            if abs(p1.x - p2.x) + abs(p1.y - p2.y) <= 4:
                pair_score += 320
            if makes_win_after(board, player, [p1, p2]):
                pair_score += 100_000
            if best_pair is None or pair_score > best_pair[0]:
                best_pair = (pair_score, [first, second])

    assert best_pair is not None
    pair_score, pair = best_pair
    return {
        "stones": [{"x": pair[0]["x"], "y": pair[0]["y"]}, {"x": pair[1]["x"], "y": pair[1]["y"]}],
        "score": pair_score,
        "reason": explain_recommendation(pair, player),
        "candidates": scored[:10],
    }


def explain_recommendation(points: list[dict[str, Any]], player: int) -> str:
    names = "、".join(f"({p['x']},{p['y']})" for p in points)
    labels: list[str] = []
    for point in points:
        labels.extend(point.get("labels", []))
    unique_labels = list(dict.fromkeys(labels))[:4]
    label_text = "、".join(unique_labels) if unique_labels else "提升局面效率"
    return f"建议 {PLAYER_NAMES[player]} 落在 {names}：{label_text}。"


def detect_threats(board: list[list[int]], player: int) -> list[dict[str, Any]]:
    threats: list[dict[str, Any]] = []
    scored_self = heatmap_for(board, player)
    scored_opp = heatmap_for(board, -player)

    for cell in sorted(scored_opp, key=lambda item: item["defense"] + item["attack"], reverse=True)[:18]:
        if cell["max_attack_line"] >= 6:
            threats.append({
                "severity": "critical",
                "type": "opponent_win_next",
                "point": {"x": cell["x"], "y": cell["y"]},
                "message": f"对手在 ({cell['x']},{cell['y']}) 有直接成六点，当前回合必须处理。",
            })
        elif cell["max_attack_line"] == 5:
            threats.append({
                "severity": "high",
                "type": "opponent_five",
                "point": {"x": cell["x"], "y": cell["y"]},
                "message": f"对手在 ({cell['x']},{cell['y']}) 能形成五连压力，需优先压制。",
            })

    for cell in sorted(scored_self, key=lambda item: item["attack"], reverse=True)[:8]:
        if cell["max_attack_line"] >= 6:
            threats.append({
                "severity": "opportunity",
                "type": "self_win",
                "point": {"x": cell["x"], "y": cell["y"]},
                "message": f"我方在 ({cell['x']},{cell['y']}) 有直接成六机会。",
            })
        elif cell["max_attack_line"] == 5:
            threats.append({
                "severity": "medium",
                "type": "self_five",
                "point": {"x": cell["x"], "y": cell["y"]},
                "message": f"我方在 ({cell['x']},{cell['y']}) 可制造五连威胁。",
            })

    deduped: list[dict[str, Any]] = []
    seen: set[tuple[str, int, int]] = set()
    priority = {"critical": 0, "opportunity": 1, "high": 2, "medium": 3}
    for threat in sorted(threats, key=lambda item: priority.get(item["severity"], 9)):
        point = threat["point"]
        key = (threat["type"], point["x"], point["y"])
        if key not in seen:
            deduped.append(threat)
            seen.add(key)
    return deduped[:8]


def phase_for_moves(moves: list[Move]) -> str:
    stone_count = sum(len(move.stones) for move in moves)
    if stone_count <= 8:
        return "opening"
    if stone_count <= 34:
        return "middle"
    return "late"


def score_snapshot(board: list[list[int]]) -> dict[str, Any]:
    black_heat = heatmap_for(board, BLACK)
    white_heat = heatmap_for(board, WHITE)
    black_best = max((cell["score"] for cell in black_heat), default=0)
    white_best = max((cell["score"] for cell in white_heat), default=0)
    black_center = 0
    white_center = 0
    center = (len(board) - 1) / 2
    for y, row in enumerate(board):
        for x, value in enumerate(row):
            if value == EMPTY:
                continue
            gain = int(max(0, 8 - (abs(x - center) + abs(y - center))))
            if value == BLACK:
                black_center += gain
            else:
                white_center += gain
    raw = (black_best - white_best) / 70_000 + (black_center - white_center) / 60
    advantage = round(math.tanh(raw), 4)
    return {
        "black_best": black_best,
        "white_best": white_best,
        "black_center": black_center,
        "white_center": white_center,
        "advantage": advantage,
        "favored": "black" if advantage > 0.08 else "white" if advantage < -0.08 else "balanced",
    }


def timeline_for_moves(moves: list[Move]) -> list[dict[str, Any]]:
    board = empty_board()
    points: list[dict[str, Any]] = []
    for idx, move in enumerate(moves, start=1):
        apply_stones(board, move.player, move.stones)
        snap = score_snapshot(board)
        win = winner(board, move)
        coords = " ".join(f"({stone.x},{stone.y})" for stone in move.stones)
        if win:
            narrative = f"{PLAYER_NAMES[win]}在第 {idx} 手形成成六。"
        elif snap["favored"] == "balanced":
            narrative = f"第 {idx} 手后局面接近平衡。"
        else:
            narrative = f"第 {idx} 手后{PLAYER_NAMES[BLACK if snap['favored'] == 'black' else WHITE]}主动权更明显。"
        points.append({
            "ply": idx,
            "player": move.player,
            "coords": coords,
            "source": move.source,
            "advantage": snap["advantage"],
            "black_best": snap["black_best"],
            "white_best": snap["white_best"],
            "winner": win,
            "narrative": narrative,
        })
    return points


def load_knowledge(base_dir: Path | None = None) -> dict[str, Any]:
    root = base_dir or Path(__file__).resolve().parent
    path = root / "knowledge" / "connectsix_knowledge.json"
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def _query_terms(analysis_seed: dict[str, Any]) -> set[str]:
    terms: set[str] = set()
    phase = analysis_seed.get("phase")
    if phase:
        terms.add(str(phase))
    for threat in analysis_seed.get("threats", []):
        terms.add(str(threat.get("type", "")))
        terms.add(str(threat.get("severity", "")))
    for point in analysis_seed.get("recommendation", {}).get("candidates", [])[:3]:
        for label in point.get("labels", []):
            terms.add(str(label))
    return {term for term in terms if term}


def retrieve_knowledge(knowledge: dict[str, Any], terms: set[str], limit: int = 3) -> list[dict[str, Any]]:
    docs = knowledge.get("documents", [])
    scored: list[tuple[int, dict[str, Any]]] = []
    normalized_terms = {term.lower() for term in terms}
    for doc in docs:
        haystack = " ".join([
            str(doc.get("title", "")),
            str(doc.get("summary", "")),
            " ".join(doc.get("tags", [])),
            " ".join(doc.get("concepts", [])),
        ]).lower()
        score = 0
        for term in normalized_terms:
            if not term:
                continue
            if term in haystack:
                score += 4
        for tag in doc.get("tags", []):
            tag_l = tag.lower()
            if any(tag_l in term or term in tag_l for term in normalized_terms):
                score += 3
        if doc.get("phase") in terms:
            score += 2
        if score:
            scored.append((score, doc))

    if not scored:
        scored = [(1, doc) for doc in docs[:limit]]
    scored.sort(key=lambda item: item[0], reverse=True)
    return [
        {
            "id": doc.get("id"),
            "title": doc.get("title"),
            "summary": doc.get("summary"),
            "concepts": doc.get("concepts", []),
            "tips": doc.get("tips", []),
            "score": score,
        }
        for score, doc in scored[:limit]
    ]


def knowledge_subgraph(knowledge: dict[str, Any], docs: list[dict[str, Any]], threats: list[dict[str, Any]]) -> dict[str, Any]:
    concept_ids = {concept for doc in docs for concept in doc.get("concepts", [])}
    for threat in threats:
        threat_type = threat.get("type")
        if threat_type in {"opponent_win_next", "self_win"}:
            concept_ids.add("six")
        if threat_type in {"opponent_five", "self_five"}:
            concept_ids.add("five")

    nodes_by_id = {node["id"]: node for node in knowledge.get("graph", {}).get("nodes", [])}
    edges = knowledge.get("graph", {}).get("edges", [])
    selected = set(concept_ids)
    for edge in edges:
        if edge["source"] in concept_ids or edge["target"] in concept_ids:
            selected.add(edge["source"])
            selected.add(edge["target"])

    return {
        "nodes": [nodes_by_id[node_id] for node_id in selected if node_id in nodes_by_id],
        "edges": [edge for edge in edges if edge["source"] in selected and edge["target"] in selected],
    }


def serialize_game_json(moves: list[Move], board: list[list[int]], current_player: int) -> dict[str, Any]:
    return {
        "board_size": len(board),
        "current_player": current_player,
        "moves": [move_to_dict(move) for move in moves],
        "board": board_to_dict(board),
        "generated_at": int(time.time()),
    }


def template_llm_report(seed: dict[str, Any], docs: list[dict[str, Any]]) -> dict[str, Any]:
    recommendation = seed["recommendation"]
    threats = seed["threats"]
    snap = seed["snapshot"]
    if threats:
        risk = threats[0]["message"]
    else:
        risk = "当前没有检测到必须立即处理的成六威胁。"
    doc_title = docs[0]["title"] if docs else "基础棋理"
    doc_tip = docs[0]["tips"][0] if docs and docs[0].get("tips") else "优先处理直接胜负点，再考虑形势效率。"
    if snap["favored"] == "balanced":
        replay = "复盘曲线显示双方主动权接近，下一手的双线制造或防守质量很关键。"
    else:
        favored = "黑棋" if snap["favored"] == "black" else "白棋"
        replay = f"复盘曲线显示{favored}当前更有主动权，但仍要先排除对手的直接威胁。"
    return {
        "provider": "template",
        "risk": risk,
        "recommendation": recommendation["reason"],
        "principle": f"关联棋理：{doc_title}。{doc_tip}",
        "review": replay,
    }


def maybe_call_llm(seed: dict[str, Any], docs: list[dict[str, Any]]) -> dict[str, Any]:
    api_key = os.getenv("LLM_API_KEY") or os.getenv("OPENAI_API_KEY")
    model = os.getenv("LLM_MODEL")
    base_url = os.getenv("LLM_BASE_URL", "https://api.openai.com/v1").rstrip("/")
    fallback = template_llm_report(seed, docs)
    if not api_key or not model:
        fallback["status"] = "offline"
        fallback["message"] = "未配置 LLM_API_KEY/OPENAI_API_KEY 与 LLM_MODEL，当前使用本地模板分析。"
        return fallback

    compact = {
        "phase": seed["phase"],
        "current_player": PLAYER_NAMES.get(seed["current_player"], "未知"),
        "threats": seed["threats"][:4],
        "recommendation": seed["recommendation"],
        "snapshot": seed["snapshot"],
        "knowledge": docs,
    }
    payload = {
        "model": model,
        "temperature": 0.2,
        "messages": [
            {
                "role": "system",
                "content": "你是六子棋教学分析助手，回答必须简洁、可解释，并按风险提醒、推荐落点、棋理解释、复盘摘要四项给出。",
            },
            {"role": "user", "content": json.dumps(compact, ensure_ascii=False)},
        ],
    }
    request = urllib.request.Request(
        f"{base_url}/chat/completions",
        data=json.dumps(payload).encode("utf-8"),
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(request, timeout=18) as response:
            data = json.loads(response.read().decode("utf-8"))
        content = data["choices"][0]["message"]["content"]
        return {
            "provider": "llm",
            "model": model,
            "content": content,
            "status": "ok",
        }
    except (urllib.error.URLError, KeyError, json.JSONDecodeError, TimeoutError) as exc:
        fallback["status"] = "fallback"
        fallback["message"] = f"LLM 调用失败，已回退本地模板：{exc}"
        return fallback


def analyze_game(moves: list[Move], current_player: int | None = None, include_llm: bool = True) -> dict[str, Any]:
    board = build_board(moves)
    if current_player is None:
        current_player = BLACK if not moves else -moves[-1].player
    if current_player not in PLAYERS:
        current_player = BLACK

    phase = phase_for_moves(moves)
    heat = heatmap_for(board, current_player)
    heat_sorted = sorted(heat, key=lambda item: item["score"], reverse=True)
    recommendation = recommend_move(board, current_player)
    threats = detect_threats(board, current_player)
    snapshot = score_snapshot(board)
    seed = {
        "phase": phase,
        "current_player": current_player,
        "recommendation": recommendation,
        "threats": threats,
        "snapshot": snapshot,
    }
    knowledge = load_knowledge()
    terms = _query_terms(seed)
    docs = retrieve_knowledge(knowledge, terms)
    graph = knowledge_subgraph(knowledge, docs, threats)
    report = maybe_call_llm(seed, docs) if include_llm else template_llm_report(seed, docs)
    last_winner = winner(board, moves[-1] if moves else None)

    return {
        "board_size": len(board),
        "current_player": current_player,
        "current_player_name": PLAYER_NAMES[current_player],
        "expected_stones": expected_stone_count(board, current_player),
        "phase": phase,
        "winner": last_winner,
        "winner_name": PLAYER_NAMES.get(last_winner, ""),
        "snapshot": snapshot,
        "threats": threats,
        "recommendation": recommendation,
        "top_candidates": heat_sorted[:12],
        "heatmap": heat,
        "rag": docs,
        "knowledge_graph": graph,
        "timeline": timeline_for_moves(moves),
        "llm_report": report,
        "game_json": serialize_game_json(moves, board, current_player),
    }


def parse_botzone_log(entries: list[Any]) -> list[Move]:
    moves: list[Move] = []
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        for key, player in (("0", BLACK), ("1", WHITE)):
            node = entry.get(key)
            if not isinstance(node, dict):
                continue
            response = node.get("response")
            if not isinstance(response, dict):
                continue
            stones = _stones_from_legacy_move(response)
            if stones:
                moves.append(Move(player=player, stones=stones, source="botzone"))
    return moves
