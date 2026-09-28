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

ACT 脚本发布、统一地图布局/克隆和碰撞引用存储已按本机指针宽度编译。
全部重构 ACT 源文件及关联地图/碰撞/文本调用方共 47 个生产文件，加 1 个签名/布局
断言文件通过独立 x64 编译（未执行）。无生产调用的旧网格管理器适配已移除。
完整游戏链接、字节码/存档与运行兼容仍待完成。详见 [ACT/地图范围](../docs/modern-x64/act-map-publication-native.md)。

ACT 2D/3D 绘制、克隆、Blit 存储及原生生命周期/容器访问已独立通过 x64 编译
（11 个生产文件，未执行）。ACT 脚本发布、地图/碰撞布局和网格管理器 ABI 仍待完成。
详见 [ACT 绘制迁移范围](../docs/modern-x64/act-render-native.md)。

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

字符串布局和字形队列的创建、ACT/脚本属性、克隆、渲染与销毁已统一为原生 C++ 存储。
图集使用地址稳定的共享所有权，容器扩容或复制文字后销毁源布局不再使字形引用失效。
原生所有权契约已独立通过 Windows x64 编译（未执行）。
用户反馈 `modern-mesh-native-01` 正常，这是用户运行反馈。
详见[字符串迁移范围](../docs/modern-x64/string-native.md)。

下一里程碑是完整 Windows x64 游戏。新增完整目标编译诊断入口，记录实际阻塞，
不绕过源代码布局与地址保护。ACT/通用脚本调用在两种位宽下显式传递接收者、保留参数
和真实浮点值；序列化类型哈希保持原版 32 位结果，崩溃诊断与临界区已通过 Windows x64 编译。
完整 x64 游戏仍被阻塞，独立编译成功不代表游戏可运行。
用户反馈 `modern-string-native-01` 正常。详见[完整游戏里程碑](../docs/modern-x64/full-game.md)。

角色/相机脚本槽、回调所有权、对象池分配、动画寻址和基础容器/引用记录已使用原生指针宽度。
17 个实际实现文件及新增所有权契约已通过 Windows x64 编译，运行时断言未执行。
VM 注册/属性桥接及 ACT/地图/应用布局仍阻塞完整 x64 游戏。
用户反馈 `modern-x64-entry-03` 正常。

通用 SqPlus 类/属性绑定、全局脚本回调签名及 ACT 文档/图层/脚本/运行时所有权已使用原生位宽。
实际绑定、ACT 生命周期和源码 VM 值/GC 实现已通过独立 Windows x64 编译，未执行运行时测试。
剩余 ACT 发布对象槽、地图/输入/应用依赖及字节码格式兼容仍需处理，尚未得到完整可玩的 x64 游戏。
用户反馈 `modern-actor-native-03` 正常。详见[绑定/ACT 范围](../docs/modern-x64/binding-act-native.md)。

## Documentation / 文档

[Migration details and remaining work](../docs/modern-platform/README.md) · [Dependency provenance](../docs/modern-platform/dependencies.json).

[SDL audio migration / 音频迁移](../docs/modern-audio/README.md) · [Latest audio build / 音频构建交接](../docs/modern-audio/BUILD.md).

[SDL GPU game integration / 游戏渲染迁移](../docs/modern-gpu/GAME.md).

[Runtime graphics width audit / 图形运行时宽度审计](../docs/modern-x64/README.md).
