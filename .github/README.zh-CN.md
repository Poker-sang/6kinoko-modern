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

用户已确认 `modern-x64-gameplay-01`（源码 `54995147`）能够启动，数字、
踩踏/踢走毛玉、背景滚动及地图道路爆炸特效这四项问题均已恢复。这是用户验证。

最新交付为 `modern-x64-files-02`，源码
`47ec4e47067a075a91446ef2064f7afa0feb0abd`。DAT、普通文件、存档和键位配置
已接入可移植文件服务边界，删除无调用的 32 位文件地址适配。
用户已确认上一版 savedata-01 的存档读写修复正常。
x64 游戏及三个文件/归档/存档合约已编译；`modern-files-02` 完成 Win32
游戏及全部 81 个合约的编译。Linux/macOS/Windows 平台模块 CI 编译已通过
（36438818634）；本批游戏运行验收仍待确认。
EXE：`runtime-builds/modern-x64-files-02/kinoko_modern_gpu.exe`。
三个原版 DAT 已复制并校验大小和 SHA256，两个精灵着色器已部署。
请保留 EXE 同目录的 DAT 文件和 `shaders` 目录。

两架构 D3D9 静态审计通过。代理未运行游戏、CTest 或任何合约程序。
完整关卡及存档兼容仍属实验性验证范围。
详见[文件服务与证据](../docs/modern-platform/file-services.md)及
[构建说明](../docs/modern-x64/full-game.md)。

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


应用/音频线程、事件和时钟迁移见[运行时服务](../docs/modern-platform/runtime-services.md)。用户已确认 Windows files-02 正常；完整 Linux/macOS 游戏仍受宿主和字体服务阻塞。


图形、ACT 和角色池内部同步已改用跨平台递归锁，闲置音频锁已删除。详见[内部锁迁移](../docs/modern-platform/internal-locks.md)。
