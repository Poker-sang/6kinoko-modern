# 6kinoko-modern

6kinoko-rebuild 的 SDL3 现代化分支。完整游戏可构建 Windows x86/x64、Linux x86_64、macOS Intel/Apple Silicon 版本。Windows 与 Apple Silicon 渲染已有用户验证；用户也确认 Linux/WSL `067fc414` 的顶出星星、气球星星、踩怪飘分正常。完整游戏及存档验收与自动测试分开记录。

## Build / 构建

Windows：先提交源码，每次使用新名称：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_x64_probe.ps1 -Name <unique-name> -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

Linux/macOS 需要 CMake 3.24+、C++17 编译器、SDL 开发依赖和 `glslangValidator`；macOS 还需要 `spirv-cross`：

```sh
cmake -S . -B build-runs/native-01 -DCMAKE_BUILD_TYPE=Release -DKINOKO_RETDEC_DISABLE_TRACE=ON -DKINOKO_RUNTIME_DIR="<absolute-new-runtime-directory>"
cmake --build build-runs/native-01 --target kinoko_modern_gpu --parallel 4
```

macOS 双架构增加 `'-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'`。CI 列出 Linux 依赖与 macOS 着色器工具。`KINOKO_PLATFORM_ONLY=ON` 只构建可移植模块；`tools/build_staged.ps1` 保留 Windows x86 构建。始终使用新的构建/运行目录，保留产物。

## Run / 运行

将完整包解压到可写目录。将自己的原版 `6kinoko_a.dat`、`6kinoko_b.dat`、`6kinoko_c.dat` 放在程序同目录，保持文件名大小写一致，并保留 `shaders/`、`fonts/`。CI 不包含原版 DAT。

| 平台 | 普通启动 | 诊断启动 |
| --- | --- | --- |
| Windows | `launch.cmd` | `run-with-diagnostics.cmd` |
| Linux | `./launch.sh` | `./diagnose.sh` |
| macOS | `Launch.command` | `Diagnose.command` |

普通入口明确关闭日志；诊断入口开启低流量日志。Windows 日志为程序旁的 `retdec_trace.log`，其他平台为 `kinoko-trace.log`，已有日志追加保留。原生平台的定向特效诊断可直接运行程序并设置 `KINOKO_TRACE=1 KINOKO_TRACE_VERBOSE=1 KINOKO_TRACE_FILTER=star`。

Linux 面向 Ubuntu 24.04 x86_64 或兼容新系统，需要 Vulkan。macOS 面向支持 Metal 的 macOS 14+，包含两种 CPU 架构，尚未签名/公证。WSL2 可通过 WSLg 运行 Linux 包；本机当前使用 CPU Vulkan（llvmpipe），评估性能前请看 WSL 记录。若再次遇到 WSLg COPY MODE 无窗口问题，重启 WSL 可恢复；`wsl --shutdown` 会关闭所有 WSL 进程。

## Status / 状态

脚本随机数在所有平台保持原版 Windows CRT 的序列和 0–32767 范围。Metal 正确处理行/层跨度，混合零默认值也分别与 D3D12/Vulkan 一致。没有通过修改游戏着色器、物理或资源尺寸掩盖这些问题。

完整游戏 CI 执行不含游玩的定向回归：原版字节码/脚本数值、存档往返及损坏文件、资源包查找/解码、随机序列、计时/同步、目录匹配。日志与存档/资源包测试文件随 CI 附件保留。GPU 上传/下载测试需要图形后端，单独执行。自动测试不代表已验证过关或实际游戏存档重启加载。

各平台包使用同一源码提交，包含哈希清单、着色器、字体、许可证。Windows 附件为 `windows-game-Win32` / `windows-game-x64`；原生附件为 `game-ubuntu-24.04` / `game-macos-14`。最新构建结果与本地路径见下方收尾记录。

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7 · stb_truetype 1.26 · Noto Sans CJK JP。D3D9/D3DX 已移除，更新时须保留 SDL/Squirrel 本地补丁。DAT/字节码/存档宽度与原生指针宽度分离。Windows 保留模态拖动及崩溃适配，普通服务使用 SDL/C++。动态文字保留供未来多语言使用，附带字体不保证完全复现 GDI 度量。

下一步：用户在各平台验证进关/过关、特效、存档退出重启加载；之后拆分输入动作、建立统一输入层，再扩展 TAS/MOD。

## Documentation / 文档

- [跨平台收尾、测试与运行包](../docs/modern-platform/cross-platform-closeout.md)
- [星星/飘分修复证据](../docs/modern-platform/linux-effects-01.md)
- [WSL 安装与运行证据](../docs/modern-platform/wsl-validation-02.md)
- [完整原生迁移历史](../docs/modern-platform/full-native-game.md)
- [依赖来源](../docs/modern-platform/dependencies.json)
- [迁移路线](../docs/modern-platform/TODO.md)

Independent action bindings preserve keyconfig.dat: [input actions](../docs/modern-platform/input-actions.md). Existing settings UI is unchanged.

Windows startup recording/replay: [replay guide](../docs/modern-platform/replay.md). Session saves are isolated; gameplay replay validation is pending.
