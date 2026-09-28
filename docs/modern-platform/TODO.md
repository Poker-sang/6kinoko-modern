# Modern 迁移与后续功能 TODO

## 当前检查点：输入／音频／应用边界
- [x] 输入管理器、脚本临时对象及宿主输入/地图类缓冲采用完整本机对象宽度。
- [x] 输入/场景虚调用显式传递占位参数；IME 使用 HWND/HIMC/WPARAM/LPARAM。
- [x] 音频原生所有权移除 legacy 地址依赖，保留 32 位句柄和输入配置文件格式。
- [x] 12 个生产文件及 1 个断言文件独立通过 x64 编译，静态确认 AMD64；未执行。
- [x] modern-input-audio-02 全量 Win32 无日志构建、77 合约编译、DAT 校验及 D3D9 静态审计。
- [ ] 清理完整 x64 探针剩余文件/位图服务、角色边界与 VM 桥接阻塞；最新 62 条诊断含连带错误。
- 范围与证据：`../modern-x64/input-audio-application-native.md`、`../modern-x64/BUILD.md`。

## 已完成编译检查点：ACT 收尾
- [x] 2D/3D 绘制、克隆、Blit 缓存、数组/链表和生命周期共 11 个生产文件独立通过 x64 编译（未执行）。
- [x] 迁移 act_binding.cpp VM 对象槽、回调签名和属性发布偏移。
- [x] 统一 ACT 地图工厂/渲染布局、容器克隆、地图管理器及碰撞引用。
- [x] 移除无生产调用的旧网格管理器；47 个生产文件及 1 个断言文件通过独立 x64 编译。
- [ ] 完整 x64 链接后核验 DAT 字节码/存档及 ACT 运行行为；目前仅编译，未运行。
- 范围与限制：`../modern-x64/act-map-publication-native.md`。

## 当前主线：完整 Windows x64 游戏
- [x] 建立完整游戏 x64 构建诊断入口；保留源代码布局/地址保护，不将失败当成可运行版本。
- [x] 公共 ACT/脚本调用显式保留 EDX 占位参数；浮点参数不再借用整数位模式传递。
- [x] 历史真实 thiscall 测试库从游戏依赖中分离；捕获的函数地址按指针宽度检查。
- [x] 序列化哈希固定为原版 32 位算法，独立编译原版类型 ID 与有符号字符样例。
- [x] 崩溃日志使用对应架构寄存器；临界区使用本机布局，实际源文件通过 x64 编译。
- [ ] 按完整编译诊断迁移基础容器/引用计数边界，再迁移角色、相机和 ACT/地图布局及分配尺寸。
- [ ] 将三整数 SqPlus 对象参数、注册键与嵌入式 VM 对象迁移为完整指针宽度；清除剩余直接调用 ABI。
- [ ] 完整 x64 游戏链接、DAT 校验，随后由用户确认基本运行。
- [ ] 迁移 GDI 字体及剩余 Windows 服务，交付 Linux 可玩版本。
- [ ] 完成 macOS 构建、打包及用户运行验证。
- 构建入口、实际阻塞和兼容约束：`../modern-x64/full-game.md`。

## 已完成检查点：SDL 平台基础
- [x] 新建独立的 6kinoko-modern 私有仓库，保留 rebuild 原版基线。
- [x] SDL3 窗口、事件与物理输入接口；原编号兼容适配。
- [x] 更新并固定 SDL/libogg/libvorbis 源码版本与来源。
- [x] 完成 Win32 完整构建、DAT 校验及 Windows x64 平台模块编译（未执行）。
- [x] Windows x64、Linux、macOS 平台模块 CI 编译成功（run 36314871824；未执行测试）。
- [ ] 用户验证窗口、键盘/手柄、焦点切换、Alt+Enter 和游戏行为。

## 输入动作拆分（用户 2026-09-27 已授权）
实施时机：SDL 输入接入稳定后，在逻辑动作层建立时实施，先于 TAS
录制格式和 Mod 输入 API 定版。当前物理输入层不假装实现动作拆分。

- [ ] 从原版 DAT 脚本及 native 输入注册核对 X 与方向键的消费位置；记录证据。
- [ ] 将 X 的攻击与加速拆成 Attack / Run，可独立绑定。
- [ ] 将进门与向上移动/爬梯拆成 Interact / MoveUp 或证据支持的独立 Climb 动作。
- [ ] 保留原版预设（允许一键多动作），另提供分离预设和用户配置。
- [ ] 分清按下、持续按住与松开语义；不改攻击频率、加速数值、门/梯判定范围。
- [ ] 若原脚本共用输入编号，迁移实际消费逻辑；仅添加按键不能算完成。
- [ ] 采用可替换脚本/资源覆盖，不改用户原始 DAT，保存格式需版本化。
- [ ] 编译针对动作组合的回归用例；未执行与用户游戏反馈分开记录。
- [ ] 验收：独立攻击不加速、独立加速不攻击；爬梯不误进门；原版预设行为保持。
- [ ] 后续 TAS 记录稳定的逻辑动作编号和映射版本；Mod 复用同一接口。

具体默认新键位在实现时选择并记录，当前不硬编码未经用户确认的布局。

## 完整跨平台后续
- [x] 替换 D3D9/D3DX 渲染，覆盖混合、纹理、网格和设备生命周期。
- [x] 实现 DirectSound -> SDL 输出迁移并完成 Win32 构建/DAT 校验（modern-audio-03）。
- [ ] 用户验证 SDL 音频的 BGM/音效/暂停/淡出与听感。
- [ ] 迁移 Win32 线程/事件、文件、路径、IME、诊断边界。
- [ ] 清理 x86 指针/布局与调用约定，单独验证 x64。
- [ ] Linux/macOS 完整游戏构建与用户运行验证。

## 后续独立批次
- [ ] TAS：输入记录/重播、暂停/逐帧、编辑后从头模拟；快照回退另行设计。
- [ ] Mod：覆盖包、内容注册、版本依赖，再制作编辑器。


# 完整路线图与阶段门槛

状态规则：勾选只表示条目写明的动作完成。实现、编译、自动测试执行、
用户运行验证分别记录。所有新批次独立命名，保留产物、日志及源码提交。
以下待办不表示 rebuild 旧整理尚未完成；不重复开启已完成的 ACT 整理。

## A. 平台拆分与依赖现代化（当前主线）
前置：固定原版基线、DAT 与用户验证版本；先保持 Windows 可运行。
- [x] 将 SDL 可移植模块与 Windows/x86 游戏桥接构建分开。
- [ ] 梳理窗口、渲染、音频、输入、计时、文件、线程及系统服务的依赖图。
- [ ] SDL 窗口/输入验证：焦点切换、热插拔、键位保存、Alt+Enter。
- [ ] 明确逐帧更新与渲染的边界；按原版证据恢复时间步长，不假定 60Hz。
- [ ] 主线程窗口/事件生命周期与工作线程移交；避免跨线程调用 SDL 限定 API。
- [x] 迁移 D3D9/D3DX：纹理格式、混合、坐标、裁剪、网格、状态缓存、设备重建。
- [x] Rendering capability audit selects SDL GPU; see `../modern-render/README.md`.
- [x] Portable central/ACT blend commands now feed the production D3D9 adapter.
- [ ] Isolate textures, quad submission, scoped state, meshes and device lifecycle; implement SDL GPU.
- [x] 实现 SDL 音频设备输出与混音，保留解码、循环、淡入淡出和关停接口。
- [x] SDL 音频 Win32 编译/DAT 校验完成（modern-audio-03，未执行测试）。
- [x] SDL 音频当前版本 Windows x64 / Linux / macOS CI 编译成功（run 36316087743，未执行测试）。
- [ ] 用户验证 SDL 音频听感与运行行为。
- [ ] 迁移 Win32 线程、锁、事件、定时器；核对场景加载/退役顺序与所有权。
- [ ] 迁移文件/路径：从 EXE 目录加载 DAT，兼容 Unicode、大小写、路径分隔符。
- [ ] 迁移 IME、剪贴板、窗口配置、错误报告与诊断；按系统能力显式降级。
- [ ] 依赖升级逐项评估：zlib、Boost、Squirrel、绑定库；记录格式/ABI/API 影响。
- [ ] Squirrel 2.2.2 的升级单列决策：先验证脚本、字节码、GC、协程、绑定语义。
- [ ] 依赖版本锁定、源码校验、许可证、无网络构建与各平台工具链说明。
验收：Windows 新后端运行原 DAT；明确每个遗留系统接口的去向；游戏逻辑差异
由证据解释。升级编解码库不要求 PCM 位级相同，但格式、时长、循环应验证。

## B. 64 位与数据布局（从现在设计，独立批次落地）
前置：新平台接口不引入 32 位指针槽；完整游戏迁移前建立清单。
- [x] 新平台模块使用真实指针、size_t 和固定宽度数据；不包含 Windows 头。
- [ ] 建立 int32_t/uint32_t 指针转换、固定偏移、虚表/函数指针与调用约定清单。
- [ ] 区分磁盘格式记录、运行时对象和诊断记录，禁止直接 sizeof(native) 读写磁盘。
- [ ] DAT、存档和脚本数值字段保持其规定宽度；仅运行时地址随宿主指针宽度变化。
- [ ] 检查 Windows LLP64 与 Linux/macOS LP64 差异（long、size_t、printf、文件偏移）。
- [ ] 清理 __thiscall/__fastcall、x86 汇编/SEH、绝对地址和非可移植函数转换。
- [ ] Squirrel/SqPlus/Sqrat 原生对象句柄、引用、GC、用户数据大小及 alignment 审计。
- [ ] 核对第三方库静态构建、SIMD、浮点舍入、结构对齐与端序假设。
- [ ] Windows x64 完整游戏构建，独立产物与原 x86 行为对照。
- [ ] 在 ARM64 上先编译平台层，再按依赖与工具链可用性迁移游戏；不预先宣称支持。
验收：无地址截断；同一 DAT 与存档语义兼容；x64 用户运行验证单独记录。
不得为迁移指针宽度无依据地改变脚本整数或物理浮点语义。

## C. 第二平台与分发
前置：渲染/音频及 Win32 服务解除绑定，完整运行时的 64 位边界明确。
- [ ] Linux 完整游戏构建与 EXE 相邻资源等价的目录策略。
- [ ] macOS 完整游戏构建，应用包路径/资源/签名策略。
- [ ] CI 分别构建平台模块与完整游戏，不用模块成功冒充游戏跨平台成功。
- [ ] 高 DPI、窗口缩放、全屏、多显示器、键盘布局、控制器与音频设备验证。
- [ ] 打包依赖与许可证，不上传原版 DAT 或个人存档；给出资源部署说明。
验收：各系统都能加载相同原版资源；运行验证由用户完成并注明平台/版本。

## D. 逻辑输入（包含上方已授权动作拆分）
前置：SDL 输入稳定；查明原脚本消费方式。
- [ ] 定义稳定逻辑动作 ID、按下/持续/松开语义及可多绑的动作映射。
- [ ] 支持原版预设与分离预设；攻击/加速、进门/爬梯按前述验收实现。
- [ ] 映射版本与配置迁移；键盘/手柄共享动作层，避免设备编号泄露到 Mod API。
- [ ] 建立真实输入与重播输入的统一入口；每帧只提交一次完整输入状态。
验收：原版预设兼容，独立动作没有原先的误触发，重播可接管同一入口。

## E. TAS 第一版
前置：单帧更新边界与逻辑输入确定；与平台替换分批排查。
- [ ] 调查随机数来源/种子、时间查询、初始存档、异步加载与对象更新顺序。
- [ ] 保存版本、资源/Mod 指纹、初始状态/存档与随机种子等重播前置条件。
- [ ] 每帧输入记录与重播；长度限制、损坏文件和不兼容版本处理。
- [ ] 暂停、推进一帧、速度控制；模拟步进不依赖渲染帧率。
- [ ] 关键状态摘要与首次分歧定位；不能仅凭输入一致声称确定性。
- [ ] 输入时间轴编辑与另存分支；编辑后从已知初始点重新模拟。
- [ ] 工具与运行时通过版本化接口连接，可把 TAS 编辑器独立为项目。
验收：同一构建/资源/初始条件下记录可重现；编辑某帧后重算后续；失配能报告。
跨平台逐帧一致性是独立目标，不是 SDL 跨平台自动带来的保证。

## F. TAS 高级能力（不挤入第一版）
- [ ] Squirrel VM、协程、原生对象、引用关系和随机状态完整快照设计。
- [ ] 快照格式版本、对象身份重建、资源复用、恢复失败处理与一致性验证。
- [ ] 检查点缓存、快速回退、任意帧恢复、时间轴分支合并与对比。
- [ ] 跨架构浮点/加载顺序差异评估；必要时限定重播兼容范围。
验收：恢复后和从起点模拟的关键状态一致；不把原始进程内存复制当可移植格式。

## G. Mod 平台
前置：资源访问、对象注册、脚本生命周期边界清晰；兼容原版 DAT。
- [ ] 包清单、稳定资源 ID、覆盖优先级、路径隔离、依赖/版本及冲突诊断。
- [ ] 原 DAT 只读基底、独立 Mod 覆盖与独立存档命名空间。
- [ ] 资源替换 -> 新关卡 -> 新敌人 -> 新 Boss -> 新角色/变身逐级开放。
- [ ] 从已有脚本/对象系统提炼注册 API，不另造互不兼容的运行规则。
- [ ] Boss 与角色扩展覆盖动画、碰撞、战斗、状态机、相机、UI、存档与资源生命周期。
- [ ] Mod 身份/版本纳入重播清单；内容变更时明确拒绝或提示重播不兼容。
- [ ] 手工制作至少一个完整示例 Mod，验证安装、卸载、运行、打包与分享。
验收：原版资源不修改；可描述依赖和冲突；示例 Mod 完整运行后再定编辑器数据模型。

## H. Mod 编辑器
前置：至少一个手工 Mod 成功，包格式与内容 API 经实际使用验证。
- [ ] 编辑器可独立项目；共享文档格式和运行时预览接口，不复制游戏规则。
- [ ] 关卡编辑优先：图层、瓦片、碰撞、出生点、门/梯、对象放置、属性与撤销重做。
- [ ] 资源浏览、引用检查、错误定位、运行预览与 Mod 打包。
- [ ] 敌人/Boss 编辑工具：行为、动画、碰撞和资源引用，循序增加专用界面。
- [ ] 角色/变身编辑：能力、状态、动画、战斗、UI 与存档兼容。
- [ ] 复用暂停/逐帧/重播定位编辑器问题；热重载仅在生命周期安全后实现。
验收：编辑 -> 验证 -> 预览 -> 打包 -> 在干净运行目录安装全过程可完成。

## 建议执行顺序
A 平台基础 -> A 渲染/音频/服务 -> B x64 与 C 第二平台分别验证 ->
D 动作层 -> E TAS 第一版 -> G Mod 平台 -> H 编辑器；F 高级 TAS 按需求单列。
D 的接口调查可在 A 期间进行；已授权动作拆分在 SDL 稳定后适时实施。
B 的宽度/布局约束从当前所有新代码开始执行，不等到 x64 批次再补救。

## Latest user feedback
- User reports `modern-audio-03` currently normal; not agent-run validation or exhaustive audio checks.

## Rendering checkpoint 2
- [x] Extract portable pixel upload, texture binding cache and sprite/quad submission interfaces.
- [x] Connect D3D9 texture creation/mapping/retirement and scoped blend/address restoration adapters.
- [x] Record user feedback: modern-render-02 currently normal (not agent-run validation).
- [ ] Replace legacy COM registry consumers together with SDL GPU resources and shaders.
- Details and remaining boundaries: `../modern-render/RESOURCES.md`.

## SDL GPU foundation
- [x] Continue SDL GPU (no bgfx); user reports modern-render-03 currently normal.
- [x] Implement independent GPU 2D renderer and Windows preview with offline shaders.
- [x] Integrate game texture registry, effective state and main-thread GPU presentation.
- [x] Complete depth/alpha-test/mesh/0x4142 support in the separate full GPU game.
- See `../modern-gpu/README.md`; preview is not a migrated game.

## SDL GPU game integration
- [x] Add kinoko_modern_gpu full-game target; no D3D9/D3DX runtime imports.
- [x] Connect texture/font generations, ordered frame handoff and main-thread presentation.
- [x] Add depth/alpha/cull, mesh streams/indices/transforms and XYZW 0x4142 handling.
- [x] Replace D3DX matrix helpers with portable row-vector math.
- [ ] User validation of the GPU game, including offscreen depth and matrix/color rounding differences.
- [x] Remove modern-game SDK enum/record/header dependencies and class-rewriting macros.
- [x] Make legacy comparison targets optional in the default build graph.
- [x] Record user feedback: modern-gpu-game-02 currently normal (not agent-run validation).
- [x] Remove unused vendored SDL D3D9 renderer/adapter-helper build dependency.
- [x] Default to the modern-only build; retain explicit historical comparison mode.
- [x] User reports modern-gpu-api-04 currently normal (not agent-run validation).
- [ ] Separate remaining renderer/runtime records from original x86 layouts.
- [ ] Migrate remaining Win32 services and full-game x64.
- Details: `../modern-gpu/GAME.md`. Legacy D3D9 comparison EXE remains available.

## Old graphics backend retirement
- [x] Remove old game target, cloned VM/native/binding libraries and SDK alias branches.
- [x] Remove D3DX import library and comparison build switches/CI jobs.
- [x] Preserve retired COM fixtures as non-build evidence, including mixed stage assertions.
- [ ] Port retired stage/gameplay assertions to modern resource ownership; do not count them as active coverage.
- [x] User reports modern-no-d3d9-01 normal; not agent-run validation.
- Still-live legacy ABI, fixed-layout records and Win32 services need migration, not blind deletion.

## x64 preparation checkpoint 1
- [x] Inventory maintained-source width/layout candidates; manually triage runtime pointers versus fixed-format values.
- [x] Remove renderer's original size/padding/vtable-prefix dependency; use explicit device listener context callbacks.
- [x] Put renderer/texture-slot/listener records in a native-width runtime header; keep texture handles 32-bit indices.
- [x] Compile runtime/listener contract for Windows x64 without original fixed-layout headers (not executed).
- [x] Separate font/atlas runtime storage from original byte views; preserve the x86 ACT configuration bridge.
- [x] Separate texture/chip resource storage and ownership from original byte layouts.
- [x] Migrate mesh resource/controller/render ownership and the 3D draw consumer; remove the 248-byte overlay and +236 list lookup.
- [x] Separate MSH/MAT decoding from the Windows archive adapter; compile the actual decoder and mesh-resource lifetime at native pointer width.
- [ ] Migrate the separate mesh-manager child/update ABI.
- [ ] Migrate Squirrel wrapper addresses and original method ABI before lifting full-game Win32 restriction.
- [x] Use a typed ACT texture render snapshot at draw/layout/target callers; remove the duplicate fixed prefix.
- [x] Store cloned texture references by retained handle; the initial pointer-keyed table has been replaced by per-object ownership.
- [x] Replace byte-backed glyph storage with native geometry and shared atlas ownership.
- [x] Compile the shared string owner and Squirrel object storage at native Windows x64 pointer width; return payload references directly as pointers.
- [x] Replace texture/render-target fixed allocation, clone/retain ownership, property offsets and render access with native texture objects.
- [x] Migrate chip factories, shared MCD lifecycle, map/script consumers and property serialization.
- [x] Centralize ACT resource/document/layer/layout/key operations in named typed method interfaces; preserve real virtual callback ordering.
- [x] Replace texture/chip scalar/array allocation cookies with aligned native metadata and reverse C++ destruction.
- [ ] Retire the remaining method-table ABI adapter and non-resource x86 layouts/allocators as their implementations migrate; current adapters do not enable a full x64 game.
- [x] Replace the 260-byte outer string layout and 256-byte glyph record together with factories, serialization, script properties, clone, draw and destruction.
- [x] Keep atlas addresses stable and retain shared pages across ReplicateText/source destruction.
- [x] Compile native string lifetime/clone and exception-safe allocation contracts at Windows x64; do not execute.
- [x] Record user feedback: modern-mesh-native-01 normal (not agent-run validation).
- [ ] Replace remaining integer-address Squirrel bindings and x86 fastcall/thiscall adapters; then attempt a full x64 game link.
- Audit scope and remaining blockers: `../modern-x64/README.md`.


## Native actor/camera ownership checkpoint
- [x] Widen actor/camera owned script slots and transferred callback arguments together.
- [x] Allocate the actor pool by native host size; preserve native animation pointers and method dispatch.
- [x] Migrate reference, integer-container and buffer record widths while retaining scalar/file formats.
- [x] Compile 17 real actor/camera/container sources and a linked ownership contract as AMD64 (not executed).
- [x] Preserve original x86 actor layout assertions and unmigrated integer-address guards.
- [x] Record user feedback: modern-x64-entry-03 normal; not agent validation.
- [x] Migrate common SqPlus registration/property bridges and ACT document/layer ownership storage.
- [ ] Finish downstream ACT publication slots and remaining script thread/generator/array bridges.
- [ ] Migrate map/collision/input/application records, then complete and validate the full x64 game.


## Native bindings and ACT ownership checkpoint
- [x] Preserve full pointers in class/property metadata, native targets, string/userdata/tag outputs and root bindings.
- [x] Match registered native closures to SQFUNCTION and keep game-property writes 32-bit.
- [x] Migrate internal type identity lookup/publication together; preserve original x86 keys.
- [x] Share native ACT script/callback storage across document, layer, lifecycle and payload IO.
- [x] Compile actual ACT association/frame/runtime and source-backed VM value/GC implementations at x64.
- [x] Record user feedback: modern-actor-native-03 normal (not agent testing).
- [ ] Migrate remaining ACT publication slots, map/draw/layout and script bridge dependencies.
- [ ] Verify original VM bytecode/save numeric formats before claiming x64 DAT compatibility.
- Scope: `../modern-x64/binding-act-native.md`.
