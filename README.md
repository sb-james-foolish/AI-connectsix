# AI-connectsix

六子棋人机对弈与智能辅助分析平台。

当前仓库包含两部分：

- `competition/competition`：原有六子棋对战平台、C++ bot、模型数据和 Botzone 风格日志生成工具。
- `assistant_platform`：新增的浏览器智能辅助平台，提供棋盘交互、机器人落子、规则分析、热力图、RAG 棋理知识库、知识图谱、用户画像和复盘报告。

启动智能辅助平台：

```powershell
python .\assistant_platform\server.py --port 8765
```

然后访问：

```text
http://127.0.0.1:8765
```

更多说明见 [assistant_platform/README.md](assistant_platform/README.md)。
