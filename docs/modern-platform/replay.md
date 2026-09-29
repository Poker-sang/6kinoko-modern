# Startup recording and replay — format 1

This is the first replay baseline, not a frame editor or save-state system.
Windows is the delivery/test platform. Each session starts a fresh process and
records from startup, including title/menu navigation. Ordinary launch is unchanged.

## 使用方法（Windows）

1. 先在普通游戏中设置好按键并退出。新版本可以复制原有 `keyconfig.dat`、
   `input-actions.cfg` 和 `marisaA.dat` / `marisaB.dat` / `marisaC.dat`。
2. 双击 **Record-Replay.cmd**。启动器保存初始存档/配置快照，使用当前 EXE 和独立存档目录启动游戏。
   正常操作，最后关闭窗口结束录制。看到 `COMPLETED ... frames` 表示文件已完成。
3. 双击 **Play-Replay.cmd** 回放最近一次录制。它会从初始快照另建一个存档目录，
   回放时忽略实时游戏操作输入，到最后一帧自动退出。仍可关闭窗口提前终止。
4. 结果见运行目录的 **replay-status.txt**。`COMPLETED` 表示全部检查点一致；
   `FAILED at frame ...` 给出首个检测到差异的帧；提前关闭则为 `ABORTED`。

会话保存在 EXE 旁的 `recordings/session-.../`：`initial/` 只保存不可修改的初始存档和配置快照，
`record/` 保存录制运行和 `session.krec`，每次回放新建 `play-.../`。
原有游戏目录里的存档不会被录制或回放改写；录制产生的新进度也不会自动导回。
EXE、三个 DAT、字体和着色器直接使用当前游戏目录里的文件，不复制进会话。
回放直接读取 `record/session.krec`，也不复制录制文件；新增空间只有存档、配置和少量日志。
资源仍从 EXE 自身目录加载，独立存档目录通过启动参数选择，所有产物保留。

回放指定会话：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\replay_session.ps1 -Mode play -Session "完整会话目录"
```

当前限制：不在录制/回放期间修改游戏内键位；这样做会停止会话并报错。也不要修改
`initial/` 下的任何文件。旧版录制与更新后的 EXE 不混用：启动器会校验当前程序和资源，更新版本后须使用原版本的程序与资源回放。
旧版完整复制会话（manifest version 1）继续用原来保留的包和启动器回放；新版会话为 version 2，录制二进制格式仍为 1。
首版不支持中途开始、快进、逐帧、输入编辑或快照恢复。

## Direct executable arguments

The same executable supports `--save-dir DIRECTORY` for ordinary play. Relative
command-line paths resolve against the caller's working directory before startup
switches resource loading to the executable directory. Saves, legacy keyconfig
and input-actions.cfg use the selected directory without fallback to ordinary saves.

For recording/playback, the launcher supplies:

```text
kinoko_modern_gpu.exe --save-dir "D:\sessions\example\play-001" --replay "D:\sessions\example\record\session.krec" --replay-status "D:\sessions\example\play-001\replay-status.txt" --replay-identity <64-digit identity>
```

Use `--record FILE` instead of `--replay FILE` to record. The exact executable and
arguments are saved in each run's `launch-arguments.json`. The identity comes from
the session manifest. The launcher performs runtime/snapshot SHA256 validation;
manual invocation must preserve the same binary/resources and restore the initial
saves/configuration. The runtime checks the recording's identity and frame checksums,
but does not independently hash all assets. Reusing a modified save directory does
not restore initial state; use the launcher for repeatable playback.

The Windows entry uses SDL_RunApp for Unicode argument decoding; shared C++ code
parses options and resolves paths. The existing Windows file adapter retains ANSI
semantics for original paths and adds UTF-8 opening for explicit save-directory paths.

## Contracts

- Stable logical action frames use the same publication function for devices and
  playback. Physical counters, script-writable fields, and recorded frames have
  separate lifetimes. Legacy x/y/buttons/releases/digits are also recorded for
  original any-button prompts and remaining consumers.
- Only replay sessions use a fixed 60 Hz simulation clock starting at 1000 ms.
  Script time, actor time and deferred ACT wake deadlines share it. SDL event
  pumping, frame scheduling and audio use real time. This is deliberately a
  deterministic session mode, not a change to ordinary gameplay timing.
- The original portable CRT-compatible script RNG is unchanged. Its state is
  observed before and after every game update; playback never overwrites RNG
  state to hide a divergence. Startup uses the same fresh-process/default RNG
  state and deterministic script seed sources.
- A per-frame checkpoint covers live actors sorted by pool handle (identity,
  position, velocity, activity, flags, collision hits and animation progress),
  camera position, map dimensions/ID, masks, and selected root game scalars.
  Raw pointers, render resources and struct padding are excluded.
- Checkpoints cover selected state, not the whole VM, every menu-local variable
  or every save write. Matching checkpoints are useful evidence, not proof of
  complete state equivalence. User gameplay replay validation remains required.
- Session identity is SHA256 over the current EXE/assets and initial configs/saves.
  The launcher hashes immutable runtime files in place and verifies the initial
  snapshot and every writable run copy. No EXE, assets or replay file are duplicated.
  Existing recordings cannot be overwritten; playback always uses a fresh copy.
- The binary format uses explicit little-endian widths, version/action schema,
  identity, frame indices, per-record checksums and a complete-stream footer.
  FNV checksums detect corruption; they are not cryptographic authentication.
  Truncation, changed identity, unsupported schema and trailing bytes are rejected
  before game initialization. One recording is limited to 1,000,000 game frames.
- File finalization occurs after the game thread is joined. Crashed or failed
  recordings lack a valid footer and cannot be mistaken for complete recordings.

## Validation

`replay_contract` verifies wire round-trips, corruption/truncation rejection,
publication and deterministic time. `replay_runtime_contract` executes a synthetic
four-frame session through the real recorder/playback integration, with the real
Squirrel RNG and actor index; it checks hardware isolation and detects injected
actor/RNG divergence at frame 2. It does not run the game or original DAT scripts.
`replay_session_contract` prepares fake file-only sessions without executing an EXE,
checking initial-save restoration, ordinary-save isolation and snapshot rejection.

The test records and all session directories are retained. Actual gameplay
determinism is reported separately from automated contract results.
