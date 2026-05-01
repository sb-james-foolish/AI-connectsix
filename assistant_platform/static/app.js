const SIZE = 15;
const BLACK = 1;
const WHITE = -1;
const EMPTY = 0;

const state = {
  moves: [],
  board: makeBoard(),
  currentPlayer: BLACK,
  winner: EMPTY,
  selected: [],
  analysis: null,
  lastBotMove: null,
  showHeatmap: false,
};

const els = {
  board: document.getElementById("board"),
  gameStatus: document.getElementById("gameStatus"),
  selectionText: document.getElementById("selectionText"),
  submitMoveBtn: document.getElementById("submitMoveBtn"),
  heatmapToggle: document.getElementById("heatmapToggle"),
  newGameBtn: document.getElementById("newGameBtn"),
  undoBtn: document.getElementById("undoBtn"),
  exportBtn: document.getElementById("exportBtn"),
  reportBtn: document.getElementById("reportBtn"),
  importBtn: document.getElementById("importBtn"),
  importFile: document.getElementById("importFile"),
  phaseText: document.getElementById("phaseText"),
  advantageText: document.getElementById("advantageText"),
  advantageFill: document.getElementById("advantageFill"),
  turnText: document.getElementById("turnText"),
  riskList: document.getElementById("riskList"),
  recommendationBox: document.getElementById("recommendationBox"),
  candidateList: document.getElementById("candidateList"),
  llmBox: document.getElementById("llmBox"),
  ragList: document.getElementById("ragList"),
  graphBox: document.getElementById("graphBox"),
  profileSummary: document.getElementById("profileSummary"),
  mistakeList: document.getElementById("mistakeList"),
  trainingList: document.getElementById("trainingList"),
  replayChart: document.getElementById("replayChart"),
  turningList: document.getElementById("turningList"),
  timelineList: document.getElementById("timelineList"),
  reportBox: document.getElementById("reportBox"),
  jsonBox: document.getElementById("jsonBox"),
  toast: document.getElementById("toast"),
};

function makeBoard() {
  return Array.from({ length: SIZE }, () => Array(SIZE).fill(EMPTY));
}

function playerName(player) {
  if (player === BLACK) return "黑棋";
  if (player === WHITE) return "白棋";
  return "终局";
}

function pointKey(x, y) {
  return `${x},${y}`;
}

function samePoint(a, b) {
  return a.x === b.x && a.y === b.y;
}

function boardStoneCount() {
  return state.board.flat().filter(Boolean).length;
}

function expectedStones() {
  if (state.winner) return 0;
  if (state.analysis?.expected_stones) return state.analysis.expected_stones;
  return boardStoneCount() === 0 && state.currentPlayer === BLACK ? 1 : 2;
}

function rebuildBoard() {
  const board = makeBoard();
  for (const move of state.moves) {
    for (const stone of move.stones || []) {
      if (stone.x >= 0 && stone.x < SIZE && stone.y >= 0 && stone.y < SIZE) {
        board[stone.y][stone.x] = move.player;
      }
    }
  }
  state.board = board;
}

async function api(path, payload) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const data = await response.json();
  if (!response.ok) {
    throw new Error(data.message || data.error || "请求失败");
  }
  return data;
}

function showToast(message) {
  els.toast.textContent = message;
  els.toast.classList.add("show");
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => els.toast.classList.remove("show"), 2400);
}

function heatMap() {
  const map = new Map();
  for (const cell of state.analysis?.heatmap || []) {
    map.set(pointKey(cell.x, cell.y), cell);
  }
  return map;
}

function recommendationSet() {
  const set = new Set();
  for (const stone of state.analysis?.recommendation?.stones || []) {
    set.add(pointKey(stone.x, stone.y));
  }
  return set;
}

function lastBotSet() {
  const set = new Set();
  for (const stone of state.lastBotMove?.stones || []) {
    set.add(pointKey(stone.x, stone.y));
  }
  return set;
}

function selectedSet() {
  return new Set(state.selected.map((point) => pointKey(point.x, point.y)));
}

function isStarPoint(x, y) {
  return [3, 7, 11].includes(x) && [3, 7, 11].includes(y);
}

function renderBoard() {
  const heat = state.showHeatmap ? heatMap() : new Map();
  const recommended = state.showHeatmap ? recommendationSet() : new Set();
  const selected = selectedSet();
  const bot = lastBotSet();
  els.board.innerHTML = "";
  for (let y = 0; y < SIZE; y += 1) {
    for (let x = 0; x < SIZE; x += 1) {
      const cell = document.createElement("button");
      cell.type = "button";
      cell.className = "cell";
      cell.setAttribute("aria-label", `坐标 ${x}, ${y}`);
      if (isStarPoint(x, y)) cell.classList.add("star");
      const key = pointKey(x, y);
      const heatCell = heat.get(key);
      if (heatCell && state.board[y][x] === EMPTY) {
        const visibleHeat = heatCell.level === "critical"
          ? Math.max(0.34, heatCell.heat * 0.58)
          : Math.max(0, (heatCell.heat - 0.44) * 0.7);
        cell.style.setProperty("--heat", String(Math.min(0.52, visibleHeat)));
      }
      if (recommended.has(key) && state.board[y][x] === EMPTY) cell.classList.add("recommended");
      if (selected.has(key)) cell.classList.add("selected");
      if (bot.has(key)) cell.classList.add("last-bot");
      if (state.board[y][x] !== EMPTY) {
        const stone = document.createElement("span");
        stone.className = `stone ${state.board[y][x] === BLACK ? "black" : "white"}`;
        cell.appendChild(stone);
      }
      cell.addEventListener("click", () => selectCell(x, y));
      els.board.appendChild(cell);
    }
  }
}

function renderModeToggle() {
  els.heatmapToggle.classList.toggle("active", state.showHeatmap);
  els.heatmapToggle.setAttribute("aria-pressed", state.showHeatmap ? "true" : "false");
}

function selectCell(x, y) {
  if (state.winner) return;
  if (state.board[y][x] !== EMPTY) return;
  const existingIndex = state.selected.findIndex((point) => point.x === x && point.y === y);
  if (existingIndex >= 0) {
    state.selected.splice(existingIndex, 1);
  } else if (state.selected.length < expectedStones()) {
    state.selected.push({ x, y });
  }
  render();
}

function renderStatus() {
  if (state.winner) {
    els.gameStatus.textContent = `${playerName(state.winner)}获胜`;
  } else {
    els.gameStatus.textContent = `${playerName(state.currentPlayer)}回合`;
  }
  const need = expectedStones();
  const picked = state.selected.map((p) => `(${p.x},${p.y})`).join(" ");
  els.selectionText.textContent = picked || `选择 ${need} 个落点`;
  els.submitMoveBtn.disabled = Boolean(state.winner) || state.selected.length !== need;
  els.turnText.textContent = playerName(state.currentPlayer);
}

function phaseName(phase) {
  return { opening: "开局", middle: "中盘", late: "后盘" }[phase] || phase || "开局";
}

function renderOverview() {
  const analysis = state.analysis;
  if (!analysis) return;
  const snap = analysis.snapshot || {};
  const advantage = Number(snap.advantage || 0);
  els.phaseText.textContent = phaseName(analysis.phase);
  els.advantageText.textContent = advantage.toFixed(3);
  els.advantageFill.style.width = `${Math.max(0, Math.min(100, (advantage + 1) * 50))}%`;

  const threats = analysis.threats || [];
  els.riskList.innerHTML = "";
  if (!threats.length) {
    els.riskList.appendChild(listItem("暂无高危风险", "当前局面没有检测到直接成六或五连压迫。"));
  } else {
    for (const threat of threats) {
      const item = listItem(threat.message, threat.type);
      item.classList.add(`severity-${threat.severity}`);
      els.riskList.appendChild(item);
    }
  }

  const rec = analysis.recommendation || {};
  els.recommendationBox.innerHTML = "";
  const recTitle = document.createElement("strong");
  recTitle.textContent = (rec.stones || []).map((p) => `(${p.x},${p.y})`).join("  ");
  const recText = document.createElement("p");
  recText.textContent = rec.reason || "暂无推荐";
  els.recommendationBox.append(recTitle, recText);

  els.candidateList.innerHTML = "";
  for (const cell of (analysis.top_candidates || []).slice(0, 8)) {
    const item = document.createElement("div");
    item.className = "candidate";
    item.innerHTML = `<strong>(${cell.x},${cell.y})</strong><span>${(cell.labels || []).slice(0, 2).join(" / ")}</span><b>${cell.score}</b>`;
    els.candidateList.appendChild(item);
  }

  const report = analysis.llm_report || {};
  els.llmBox.innerHTML = "";
  if (report.content) {
    const text = document.createElement("p");
    text.textContent = report.content;
    els.llmBox.appendChild(text);
  } else {
    for (const key of ["risk", "recommendation", "principle", "review", "profile"]) {
      if (!report[key]) continue;
      const p = document.createElement("p");
      p.textContent = report[key];
      els.llmBox.appendChild(p);
    }
    if (report.message) {
      const p = document.createElement("p");
      p.textContent = report.message;
      els.llmBox.appendChild(p);
    }
  }
}

function listItem(title, text) {
  const item = document.createElement("div");
  item.className = "list-item";
  const strong = document.createElement("strong");
  strong.textContent = title;
  const p = document.createElement("p");
  p.textContent = text || "";
  item.append(strong, p);
  return item;
}

function renderKnowledge() {
  const analysis = state.analysis;
  if (!analysis) return;
  els.ragList.innerHTML = "";
  for (const doc of analysis.rag || []) {
    const item = listItem(doc.title, doc.summary);
    const tags = document.createElement("div");
    tags.className = "tags";
    for (const concept of doc.concepts || []) {
      const tag = document.createElement("span");
      tag.className = "tag";
      tag.textContent = concept;
      tags.appendChild(tag);
    }
    item.appendChild(tags);
    els.ragList.appendChild(item);
  }

  const graph = analysis.knowledge_graph || { nodes: [], edges: [] };
  els.graphBox.innerHTML = "";
  const nodes = document.createElement("div");
  nodes.className = "graph-nodes";
  for (const node of graph.nodes || []) {
    const chip = document.createElement("span");
    chip.className = `node-chip ${node.type || ""}`;
    chip.textContent = node.name;
    chip.title = node.summary || "";
    nodes.appendChild(chip);
  }
  const edges = document.createElement("div");
  edges.className = "graph-edges";
  edges.textContent = (graph.edges || [])
    .map((edge) => `${edge.source} ${edge.label} ${edge.target}`)
    .join("  ·  ");
  els.graphBox.append(nodes, edges);
}

function percent(value) {
  return `${(Number(value || 0) * 100).toFixed(1)}%`;
}

function renderProfile() {
  const profile = state.analysis?.user_profile || {};
  els.profileSummary.innerHTML = "";
  const cards = [
    ["风格", profile.style || "样本不足"],
    ["平均质量", String(profile.avg_quality || 0)],
    ["推荐命中", percent(profile.follow_recommendation_rate)],
    ["中腹率", percent(profile.center_rate)],
    ["进攻率", percent(profile.attack_rate)],
    ["防守率", percent(profile.defense_rate)],
  ];
  for (const [label, value] of cards) {
    const card = document.createElement("div");
    card.className = "profile-card";
    card.innerHTML = `<span class="eyebrow">${label}</span><strong>${value}</strong>`;
    els.profileSummary.appendChild(card);
  }

  els.mistakeList.innerHTML = "";
  const evaluations = profile.evaluations || [];
  const notable = evaluations
    .filter((item) => item.severity !== "good" || item.tags?.includes("贴近热区"))
    .slice(-8)
    .reverse();
  if (!notable.length) {
    els.mistakeList.appendChild(listItem("暂无明显失误", "继续观察必防点、成六点和双线机会。"));
  } else {
    for (const item of notable) {
      const detail = `${(item.tags || []).join(" / ")} · 质量 ${item.quality_score}`;
      const row = listItem(`#${item.ply} ${item.summary}`, detail);
      row.classList.add(`severity-${item.severity}`);
      els.mistakeList.appendChild(row);
    }
  }

  els.trainingList.innerHTML = "";
  for (const suggestion of profile.suggestions || []) {
    els.trainingList.appendChild(listItem(suggestion, ""));
  }
}

function renderReplay() {
  const timeline = state.analysis?.timeline || [];
  renderChart(timeline);
  els.turningList.innerHTML = "";
  const turning = state.analysis?.turning_points || [];
  if (!turning.length) {
    els.turningList.appendChild(listItem("暂无明显转折", "优势曲线比较平稳，可以重点复盘候选点选择。"));
  } else {
    for (const item of turning) {
      els.turningList.appendChild(listItem(item.summary, item.direction));
    }
  }
  els.timelineList.innerHTML = "";
  for (const point of timeline.slice().reverse().slice(0, 12)) {
    const item = document.createElement("div");
    item.className = "timeline-item";
    item.innerHTML = `<strong>#${point.ply} ${playerName(point.player)} ${point.coords}</strong><p>${point.narrative}</p>`;
    els.timelineList.appendChild(item);
  }
}

function renderChart(timeline) {
  const svg = els.replayChart;
  svg.innerHTML = "";
  const ns = "http://www.w3.org/2000/svg";
  const width = 420;
  const height = 180;
  const pad = 24;
  const axis = document.createElementNS(ns, "path");
  axis.setAttribute("d", `M${pad} ${height / 2}H${width - pad}`);
  axis.setAttribute("stroke", "#cbd5e1");
  axis.setAttribute("stroke-width", "2");
  svg.appendChild(axis);
  if (!timeline.length) return;
  const points = timeline.map((item, index) => {
    const x = timeline.length === 1 ? width / 2 : pad + (index / (timeline.length - 1)) * (width - pad * 2);
    const y = height / 2 - Number(item.advantage || 0) * 68;
    return [x, Math.max(pad, Math.min(height - pad, y))];
  });
  const line = document.createElementNS(ns, "polyline");
  line.setAttribute("points", points.map((p) => p.join(",")).join(" "));
  line.setAttribute("fill", "none");
  line.setAttribute("stroke", "#0f766e");
  line.setAttribute("stroke-width", "3");
  line.setAttribute("stroke-linecap", "round");
  line.setAttribute("stroke-linejoin", "round");
  svg.appendChild(line);
  for (const [x, y] of points) {
    const circle = document.createElementNS(ns, "circle");
    circle.setAttribute("cx", String(x));
    circle.setAttribute("cy", String(y));
    circle.setAttribute("r", "4");
    circle.setAttribute("fill", "#334155");
    svg.appendChild(circle);
  }
}

function renderJson() {
  const data = state.analysis?.game_json || {
    board_size: SIZE,
    current_player: state.currentPlayer,
    moves: state.moves,
    board: state.board,
  };
  els.jsonBox.value = JSON.stringify(data, null, 2);
  els.reportBox.value = state.analysis?.review_report || "";
}

function render() {
  renderModeToggle();
  renderBoard();
  renderStatus();
  renderOverview();
  renderKnowledge();
  renderProfile();
  renderReplay();
  renderJson();
}

async function refreshAnalysis(includeLlm = true) {
  state.analysis = await api("/api/analyze", {
    moves: state.moves,
    current_player: state.currentPlayer || BLACK,
    include_llm: includeLlm,
  });
  render();
}

async function submitMove() {
  try {
    const data = await api("/api/play", {
      moves: state.moves,
      player: state.currentPlayer,
      stones: state.selected,
      include_llm: true,
    });
    state.moves = data.moves;
    state.board = data.board;
    state.currentPlayer = data.current_player || state.currentPlayer;
    state.winner = data.winner || EMPTY;
    state.analysis = data.analysis;
    state.lastBotMove = data.last_bot_move;
    state.selected = [];
    render();
    if (state.winner) showToast(`${playerName(state.winner)}获胜`);
  } catch (error) {
    showToast(error.message);
  }
}

async function newGame() {
  state.moves = [];
  state.board = makeBoard();
  state.currentPlayer = BLACK;
  state.winner = EMPTY;
  state.selected = [];
  state.lastBotMove = null;
  await refreshAnalysis(true);
}

async function undo() {
  if (!state.moves.length) return;
  const last = state.moves[state.moves.length - 1];
  const removeCount = last.source === "robot" && state.moves.length >= 2 ? 2 : 1;
  state.moves.splice(state.moves.length - removeCount, removeCount);
  rebuildBoard();
  state.currentPlayer = state.moves.length ? -state.moves[state.moves.length - 1].player : BLACK;
  state.winner = EMPTY;
  state.selected = [];
  state.lastBotMove = [...state.moves].reverse().find((move) => move.source === "robot") || null;
  await refreshAnalysis(false);
}

function exportJson() {
  const payload = els.jsonBox.value || "{}";
  const blob = new Blob([payload], { type: "application/json;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `connectsix-game-${Date.now()}.json`;
  link.click();
  URL.revokeObjectURL(url);
}

function exportReport() {
  const payload = state.analysis?.review_report || "# 六子棋复盘报告\n\n暂无复盘内容。";
  const blob = new Blob([payload], { type: "text/markdown;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const link = document.createElement("a");
  link.href = url;
  link.download = `connectsix-review-${Date.now()}.md`;
  link.click();
  URL.revokeObjectURL(url);
}

async function importLog(file) {
  try {
    const text = await file.text();
    const json = JSON.parse(text);
    const data = await api("/api/import-botzone", Array.isArray(json) ? json : { entries: json.entries || json.log || json });
    state.moves = data.moves;
    state.board = data.board;
    state.currentPlayer = data.current_player || BLACK;
    state.winner = data.analysis?.winner || EMPTY;
    state.analysis = data.analysis;
    state.selected = [];
    state.lastBotMove = [...state.moves].reverse().find((move) => move.source === "robot") || null;
    render();
    showToast("日志已导入");
  } catch (error) {
    showToast(error.message);
  } finally {
    els.importFile.value = "";
  }
}

function bindTabs() {
  for (const tab of document.querySelectorAll(".tab")) {
    tab.addEventListener("click", () => {
      for (const item of document.querySelectorAll(".tab")) item.classList.remove("active");
      for (const panel of document.querySelectorAll(".tab-panel")) panel.classList.remove("active");
      tab.classList.add("active");
      document.getElementById(`${tab.dataset.tab}Panel`).classList.add("active");
    });
  }
}

els.submitMoveBtn.addEventListener("click", submitMove);
els.heatmapToggle.addEventListener("click", () => {
  state.showHeatmap = !state.showHeatmap;
  render();
});
els.newGameBtn.addEventListener("click", newGame);
els.undoBtn.addEventListener("click", undo);
els.exportBtn.addEventListener("click", exportJson);
els.reportBtn.addEventListener("click", exportReport);
els.importBtn.addEventListener("click", () => els.importFile.click());
els.importFile.addEventListener("change", () => {
  const file = els.importFile.files?.[0];
  if (file) importLog(file);
});

bindTabs();
newGame().catch((error) => showToast(error.message));
