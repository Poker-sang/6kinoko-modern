# 6kinoko-modern

6kinoko-rebuild 的现代化分支，采用 SDL 窗口、输入、音频和 GPU，升级 Ogg/Vorbis。完整 Windows x86 与实验性 x64 游戏均已构建；完整 Linux/macOS 游戏仍待迁移。

## Build / 构建

```sh
cmake -S . -B build-runs/platform-01 -DKINOKO_PLATFORM_ONLY=ON -DCMAKE_BUILD_TYPE=Release -DKINOKO_RUNTIME_DIR="<absolute-new-runtime-directory>"
cmake --build build-runs/platform-01 --config Release --parallel 4
```

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_staged.ps1 -Name <unique-name> -SourceDir C:\WorkSpace\6kinoko -Generator "Visual Studio 17 2022"
```

Use a new build name and runtime directory for every batch. 每批使用新的构建名称与运行目录。

## Status / 状态

完整 x64 游戏已在 `modern-full-x64-24` 编译、链接成功，源码提交为
`e720bc466dda17c600023c84a9cd3680da79ae3d`。静态检查确认 AMD64 / PE32+。
EXE：`runtime-builds/modern-full-x64-24/kinoko_modern_gpu.exe`。
三个原版 DAT 已复制并校验大小和 SHA256，两个精灵着色器已部署。
请保留 EXE 同目录的 DAT 文件和 `shaders` 目录。

同一源码在 `modern-full-width-03` 完成 Win32 游戏及 78 个合约的编译。
新增的原版位宽字节码合约也已单独通过 x64 编译。两架构 D3D9 静态审计通过。
未运行游戏、CTest 或任何合约程序。实验性 x64 的真实 DAT 字节码加载、游戏行为
及存档兼容仍未验证；编译成功不代表所有运行时指针流已经正确。
详见[完整游戏交付](../docs/modern-x64/full-game-native.md)及
[构建说明与证据](../docs/modern-x64/full-game.md)。

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7.

唯一现代后端不再依赖 D3D9/D3DX 导入、SDK 头文件、SDL D3D9 渲染器及加载器。
更新 SDL 时须保留[本地补丁](../third_party/SDL3-3.4.16/KINOKO_PATCHES.md)。
旧 COM 合约仅作非构建历史证据。可移植模块 CI 覆盖 Windows x64、Linux 和 macOS，
不代表完整游戏已通过跨平台验证。

ACT/资源、角色/相机、地图/碰撞、输入/音频/应用与 VM 桥接已按原生指针宽度
参与完整游戏编译。原生所有权、具名方法调用与原版固定宽度序列化哈希、CV4 字节码
和存档标量分离；兼容实现仍待运行验证。历史迁移范围和构建尝试保留在
[x64 构建记录](../docs/modern-x64/BUILD.md)。

此前用户反馈 `modern-width-01`、`modern-window-move-01`、`modern-act-chip-02`、
`modern-mesh-native-01`、`modern-string-native-01`、`modern-x64-entry-03` 与
`modern-actor-native-03` 正常。这些是旧版本的用户反馈，不是代理验证，也不是
新 x64 可执行文件的验收结论。

下一步：用户 x64 运行验收，以及字体等剩余 Windows 服务迁移，推进完整 Linux/macOS 游戏。

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).

[Runtime graphics width audit / 图形运行时宽度审计](../docs/modern-x64/README.md).
