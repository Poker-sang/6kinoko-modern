# 6kinoko-modern

6kinoko-rebuild 的现代化分支。本批接入 SDL3 窗口、物理输入与音频输出，升级 Ogg/Vorbis。完整游戏目前仍限 Windows x86；可移植模块单独构建，尚未验证游戏运行。

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

SDL 3.4.16 · libogg 1.3.6 · libvorbis 1.3.7.

Windows x86 game: `kinoko_modern_gpu.exe` uses SDL window/input/audio/GPU.
默认只构建现代版，移除其 D3D9/D3DX DLL 导入、SDK 头文件依赖、SDL D3D9 渲染器
及 D3D9 DLL 加载入口。SDL 包含[已记录的本地补丁](../third_party/SDL3-3.4.16/KINOKO_PATCHES.md)。
旧版对照目标、SDK 别名分支、复制构建库和 D3DX 导入库已移除。
仍适用的游戏逻辑/VM 测试使用唯一现代后端编译；旧 COM 测试源码仅作为非构建迁移证据
保留，其中尚未迁移的断言不计入当前测试覆盖。
用户反馈 `modern-width-01` 目前正常，不记为代理运行验证。
完整游戏的 x64 和非 Windows 平台支持仍待完成。
Keep the staged `shaders` directory and three DAT files beside the game EXE.
Portable modules: Windows x64, Linux, macOS build workflow; contracts compiled, not executed.

字体/图集、共享字符串所有权和 Squirrel 对象负载存储已通过独立 Windows x64 编译；
ACT 纹理渲染改用具名状态快照。完整游戏仍依赖 x86 ACT/VM 布局和方法适配，
这些独立编译结果不代表 x64 游戏运行兼容。
用户反馈 `modern-window-move-01` 正常，这是用户运行验证。
ACT 纹理/渲染目标已在创建、克隆、属性读写、脚本绑定、渲染访问和释放中使用原生资源存储。
克隆持有的额外引用直接保存在对象内，原指针键所有权表已移除。芯片资源的克隆、加载、
地图缓存和脚本访问已统一为原生存储与共享 MCD 所有权；纹理/芯片数组也改用本机宽度元数据。
ACT 调用统一为具名类型接口，旧方法表保留在明确的适配边界内。实际资源/字符串及方法适配
契约已通过独立 Windows x64 编译。网格资源也已改用原生控制器/渲染对象所有权、具名 3D 绘制
接口和可移植的 MSH/MAT 解码器；完整游戏 x64、外层 ACT 布局、网格管理器 ABI 和 VM 绑定仍待迁移。
详见[纹理范围](../docs/modern-x64/act-texture-native.md)及[芯片/方法接口范围](../docs/modern-x64/act-chip-methods.md)。
用户反馈 `modern-act-chip-02` 正常，这是用户运行验证。
详见[网格迁移范围](../docs/modern-x64/mesh-native.md)。

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).

[Runtime graphics width audit / 图形运行时宽度审计](../docs/modern-x64/README.md).
