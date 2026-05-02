# 不依赖本地端口的打开方式

## 最省事做法（推荐）：前后端同域，一个链接直接分享

你不需要先知道后端域名。直接把整个项目部署成一个 Render Web Service，Render 会自动给你公网地址。

已准备好配置文件：`render.yaml`（仓库根目录）。

### 操作步骤

1. 把当前项目推到 GitHub（新建仓库并上传）。
2. 打开 Render，选择 New -> Blueprint。
3. 连接你的 GitHub 仓库，Render 会读取 `render.yaml` 自动创建服务。
4. 等待部署完成，得到地址：

```text
https://xxxx.onrender.com
```

5. 打开该地址即可使用，发这个链接给任何人都能打开。

### 验证

- 首页：`https://xxxx.onrender.com/`
- 健康检查：`https://xxxx.onrender.com/api/health`

如果健康检查返回 `{"ok": true, ...}`，说明可用。

你现在要的是：任何人用网址打开，而不是 `127.0.0.1:8765`。

当前项目有后端 API（`/api/*`），所以必须部署到公网。

## 方案概览

- 前端：部署 `assistant_platform/static` 到静态托管（Cloudflare Pages / Netlify / Vercel）
- 后端：部署 `assistant_platform/server.py` 到 Python 托管（Render / Railway）
- 前端配置远程 API：在 `assistant_platform/static/index.html` 里加入一行配置脚本

## 1) 部署后端（以 Render 为例）

1. 新建 Web Service，连接你的仓库
2. Root Directory 设为 `assistant_platform`
3. Build Command（可留空）
4. Start Command:

```bash
python server.py --host 0.0.0.0 --port $PORT
```

5. 部署完成后得到后端地址，例如：

```text
https://connectsix-api.onrender.com
```

## 2) 部署前端

把 `assistant_platform/static` 作为站点根目录部署，得到前端地址，例如：

```text
https://connectsix-web.pages.dev
```

## 3) 配置前端访问远程 API

在 `assistant_platform/static/index.html` 的 `</body>` 前，添加：

```html
<script>
  window.CONNECTSIX_API_BASE = "https://connectsix-api.onrender.com";
</script>
```

注意：替换成你的后端真实地址。

## 4) 完成

现在把前端网址发给任何人即可直接打开：

```text
https://connectsix-web.pages.dev
```

不再依赖你本地端口。

## 说明

- 本项目后端已设置 `Access-Control-Allow-Origin: *`，跨域请求可用。
- 若后端使用免费实例，首次访问可能冷启动慢几秒。
