# 6kinoko-modern

6kinoko-rebuild 的 SDL3 现代化分支。完整游戏现已能构建 Windows x86/x64、Linux x86_64 及 macOS Intel/Apple Silicon 版本。非 Windows 运行包已编译并部署资源，但目标机实际游戏验证仍待完成。

## Build / 构建

Windows：先提交源码，每次使用新名称：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_x64_probe.ps1 -Name <unique-name> -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

Linux/macOS 完整游戏需要 CMake 3.24+、C++17 编译器、SDL 平台开发依赖及 `glslangValidator`；macOS 还需要 `spirv-cross`：

```sh
cmake -S . -B build-runs/native-01 -DCMAKE_BUILD_TYPE=Release -DKINOKO_RUNTIME_DIR="<absolute-new-runtime-directory>"
cmake --build build-runs/native-01 --target kinoko_modern_gpu --parallel 4
```

macOS 双架构构建在配置时增加 `'-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64'`。CI 工作流列出了 Linux 开发依赖；macOS 使用 `brew install glslang spirv-cross`。`KINOKO_PLATFORM_ONLY=ON` 仍可只构建可移植模块；`tools/build_staged.ps1` 保留 Windows x86 全目标构建。

始终使用新的构建及运行目录。将自己的原版 `6kinoko_a.dat`、`6kinoko_b.dat`、`6kinoko_c.dat` 放在程序同目录，并保留同目录的 `shaders/` 和 `fonts/`。游戏及合约程序仍由用户执行。

## Status / 状态

Apple Silicon 后续：用户反馈候选版 `32dc959a` 无效，其深度/Retina 改动已在
`444faba` 撤回。录像及原版资源检查确认 SDL Metal 纹理上传忽略行/层跨度的错误。
现已修正后端对通用上传参数的处理；Mac 实际显示结果仍需用户验证。详见
[渲染交接](../docs/modern-platform/macos-rendering-02.md)。

源码 `6b39bfe4dfd8925003cc3d530ab91effcfc5af93` 的[六项 CI 均通过](https://github.com/Poker-sang/6kinoko-modern/actions/runs/36460831199)：完整 Windows、Linux、macOS 双架构游戏，以及三平台可移植模块编译。

本地 Windows 交付：`runtime-builds/modern-x64-native-05/kinoko_modern_gpu.exe`。x64 游戏及相关合约已编译；`modern-native-05` 完成 Win32 游戏及全部 85 个合约编译。DAT 哈希与 D3D9 静态审计通过。代理未运行游戏、CTest 或任何合约程序。

原生 CI 附件 `game-ubuntu-24.04`、`game-macos-14` 提供 `.tar.gz` 运行包，保留可执行权限，包含着色器、字体、许可证和哈希清单，不包含原版 DAT。Linux 面向 Ubuntu 24.04 x86_64 或兼容的新系统，需要 Vulkan 驱动。macOS 面向支持 Metal 的 macOS 14+，包含两种 CPU 架构；尚未签名或公证。解压到可写目录，通过 `launch.sh` 或 `Launch.command` 启动。

用户已确认上一版 `modern-x64-graphics-02` 及此前游戏行为、存档修复正常；这不代表当前字体/后端变更或非 Windows 运行已获验证。动态文字保留原有布局和图集接口，使用通用 CP932 解码与附带的 Noto Sans CJK JP。字体名称统一回退到该字体，不保证与 GDI 完全相同的字形度量。UTF-8 多语言接口留待后续实现。

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7 · stb_truetype 1.26 · Noto Sans CJK JP。游戏旧 D3D9/D3DX 后端已移除，更新 SDL 时须保留本地补丁。原版 DAT、字节码和存档宽度仍与原生指针分离。Windows 保留模态拖动钩子和崩溃诊断；普通平台服务使用 SDL 或标准 C++。

下一步：目标机启动、游戏行为与存档验证，并修复反馈的平台问题。编译成功不等于运行兼容性已验证。

## Documentation / 文档

- [完整原生游戏迁移和交付证据](../docs/modern-platform/full-native-game.md)
- [依赖来源](../docs/modern-platform/dependencies.json)
- [迁移路线](../docs/modern-platform/TODO.md)
- [历史平台基础记录](../docs/modern-platform/README.md)
- [SDL GPU 集成](../docs/modern-gpu/GAME.md)
- [原生宽度迁移历史](../docs/modern-x64/BUILD.md)
