# maze_win_gui

最小 Windows GUI 壳层（Phase 5 / Step 5.1）：

- 只依赖 `maze_api_c`（`api/maze_api.h`）。
- 使用 `MazeSession` 生命周期：create / configure / run / destroy。
- 订阅 `MazeEventCallback` 事件流并在本地渲染画布。
- 异步执行任务，增量刷新搜索过程与进度。
- 支持取消按钮（调用 `MazeSessionCancel(...)`）。
- 不再暴露 `Write PNG` / `OutputDir` 控件，运行路径不写磁盘。
- 展示状态、错误信息与摘要 JSON。
- 不依赖 `maze_core` 内部头文件。

运行方式（Windows）：

1. 构建后启动 `maze_win_gui.exe`
2. 点击 `Run JSON`
3. 在窗口中查看状态、帧信息和摘要 JSON
