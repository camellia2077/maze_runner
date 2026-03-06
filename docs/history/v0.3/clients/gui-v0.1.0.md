## [v0.1.0] - 2026-03-06

### 新增功能 (Added)
* 新增 `apps/maze_win_gui/` Windows GUI 壳层目标 `maze_win_gui`。
* 新增 `apps/maze_win_gui/src/event/maze_event_parser.*`、`apps/maze_win_gui/src/state/gui_state_store.*`、`apps/maze_win_gui/src/render/maze_rasterizer.*` 与 `apps/maze_win_gui/src/runtime/session_runner.*`。
* 新增参数面板、算法下拉选择、异步运行模型与实时状态面板。
* 新增窗口标题版本信息，显示 `gui=<version>, kernel=<version>`。
* 新增 `apps/maze_win_gui/OPEN_SOURCE_LICENSES.md` 开源声明文档。

### 技术改进/重构 (Changed/Refactor)
* 重构 GUI 运行链路为 `MazeSession` 事件流驱动，由 `MazeEventCallback` 将拓扑、进度与路径事件映射为本地 BGRA 画布。
* 重构渲染模式为拓扑快照 + dirty-cell 增量更新，避免每个事件都整帧重栅格。
* 重构状态管理为 `GuiStateStore` 集中维护运行状态、摘要 JSON、取消标记与最新渲染缓冲。

### 修复 (Fixed)
* 修复 `Width/Height` 变化后的终点越界问题，自动调整 `End X/End Y`。
* 修复坐标输入失焦后的越界值，统一执行自动夹取。
* 修复取消、失败与完成事件在 GUI 上的状态反馈与摘要展示。

### 弃用/删除 (Deprecated/Removed)
* 删除对旧帧回放接口的依赖，不再通过历史帧数组驱动 GUI。
* 删除 `Write PNG` / `OutputDir` 控件及对应请求字段，GUI 运行主路径不再写磁盘。
