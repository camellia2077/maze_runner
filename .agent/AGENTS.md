# Build + Test 执行流程（单命令）

## 目标
- 用一条命令触发 Python 脚本，完成“编译 + 测试”。
- Agent 只需看命令退出码和汇总行即可判断是否通过。

## 统一命令
在仓库根目录执行：

```powershell
python "scripts/build_and_test.py"
```

如果需要转发构建参数（示例：Debug + 无优化）：

```powershell
python "scripts/build_and_test.py" --build-dir build_debug -- --build-type Debug --no-opt
```

## 判定规则
- 成功条件：
  - 进程退出码 `0`
  - 输出包含：`[SUMMARY] build=PASS test=PASS overall=PASS`
- 失败条件：
  - 退出码非 `0`，或
  - 汇总行为 `overall=FAIL`

## 失败时排查入口
- 编译失败：查看 `[STEP] build` 阶段日志。
- 测试失败：查看 `[STEP] test` 阶段输出（`ctest --output-on-failure` 已开启）。
