# 六子棋智能辅助平台原型

这个目录是在现有 `competition/competition` 对战平台旁边新增的可运行原型，重点先打通：

- 用户落子后生成棋局 JSON
- 后端规则推理、胜负判断、机器人落子
- 可解释热力图和候选点排序
- RAG 棋理知识库命中
- 轻量知识图谱子图
- 时间序列复盘曲线
- 大语言模型分析接口

## 启动

```powershell
python .\assistant_platform\server.py --port 8765
```

浏览器打开：

```text
http://127.0.0.1:8765
```

## LLM 配置

默认不需要密钥，后端会使用本地模板生成风险提醒、推荐落点、棋理解释和复盘摘要。

若要连接兼容 Chat Completions 的模型服务，设置：

```powershell
$env:LLM_API_KEY="你的密钥"
$env:LLM_MODEL="你的模型名"
$env:LLM_BASE_URL="https://api.openai.com/v1"
python .\assistant_platform\server.py --port 8765
```

`LLM_BASE_URL` 可替换成本地模型网关或其他兼容服务地址。

## API

- `POST /api/play`：提交当前用户落子，返回机器人落子和分析结果。
- `POST /api/analyze`：只分析已有棋局，不自动落子。
- `POST /api/import-botzone`：导入现有平台生成的 Botzone 风格日志。
- `GET /api/knowledge`：查看内置棋理知识库和知识图谱。

坐标采用 `x/y`，范围 `0..14`，棋盘数据为 `board[y][x]`。
