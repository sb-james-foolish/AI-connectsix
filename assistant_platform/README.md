# 六子棋智能辅助平台原型

这个目录是在现有 `competition/competition` 对战平台旁边新增的可运行原型，核心链路是：

用户落子 -> 生成棋局 JSON -> 后端规则分析 -> 机器人回应 -> 热力图解释 -> RAG/LLM 生成教学反馈 -> 时间序列复盘。

## 已实现功能

- 15x15 前端棋盘、黑白棋展示、新局、悔棋
- 用户落子后机器人自动落子
- 每手自动维护棋局 JSON
- 后端 API：合法性、胜负、威胁点、推荐点
- 可解释 AI 热力图和候选点排序
- RAG 棋理知识库命中
- 轻量知识图谱展示
- 时间序列复盘曲线与关键转折识别
- 用户画像：进攻/防守倾向、中腹率、推荐命中率、平均质量
- 失误识别：漏防成六、错过成六、留出反击、偏离推荐
- Markdown 复盘报告导出
- LLM 分析接口；未配置密钥时使用本地模板兜底

## 启动

```powershell
python .\assistant_platform\server.py --port 8765
```

浏览器打开：

```text
http://127.0.0.1:8765
```

## LLM 配置

默认不需要密钥，后端会使用本地模板生成风险提醒、推荐落点、棋理解释、用户画像摘要和复盘总结。

若要连接兼容 Chat Completions 的模型服务，设置：

```powershell
$env:LLM_API_KEY="你的密钥"
$env:LLM_MODEL="你的模型名"
$env:LLM_BASE_URL="https://api.openai.com/v1"
python .\assistant_platform\server.py --port 8765
```

`LLM_BASE_URL` 可替换成本地模型网关或其他兼容服务地址。

## API

- `POST /api/play`：提交当前用户落子，返回机器人落子和完整分析结果。
- `POST /api/analyze`：只分析已有棋局，不自动落子。
- `POST /api/import-botzone`：导入现有平台生成的 Botzone 风格日志。
- `GET /api/knowledge`：查看内置棋理知识库和知识图谱。

坐标采用 `x/y`，范围 `0..14`，棋盘数据为 `board[y][x]`。

`/api/analyze` 和 `/api/play` 的分析结果包含：

- `threats`：当前风险与机会
- `recommendation`：推荐落点和解释
- `heatmap`：每个空点的热力分数
- `rag`：命中的棋理知识
- `knowledge_graph`：相关概念子图
- `timeline`：每手优势变化
- `turning_points`：关键转折点
- `user_profile`：用户画像和失误统计
- `review_report`：可导出的 Markdown 复盘报告
