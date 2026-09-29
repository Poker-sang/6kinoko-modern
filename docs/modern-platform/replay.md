# Startup recording and replay — format 1

This is the first replay baseline, not a frame editor or save-state system.
Windows is the delivery/test platform. Each session starts a fresh process and
records from startup, including title/menu navigation. Ordinary launch is unchanged.

## 使用方法（Windows）

1. 先在普通游戏中设置好按键并退出。新版本可以复制原有 `keyconfig.dat`、
   `input-actions.cfg` 和 `marisaA.dat` / `marisaB.dat` / `marisaC.dat`。
2. 双击 **Record-Replay.cmd**。启动器保存初始文件快照，并在独立目录启动游戏。
   正常操作，最后关闭窗口结束录制。看到 `COMPLETED ... frames` 表示文件已完成。
3. 双击 **Play-Replay.cmd** 回放最近一次录制。它会从初始快照另建一个运行目录，
   回放时忽略实时游戏操作输入，到最后一帧自动退出。仍可关闭窗口提前终止。
4. 结果见运行目录的 **replay-status.txt**。`COMPLETED` 表示全部检查点一致；
   `FAILED at frame ...` 给出首个检测到差异的帧；提前关闭则为 `ABORTED`。

会话保存在 EXE 旁的 `recordings/session-.../`：`initial/` 是不可修改的初始快照，
`record/` 保存录制运行和 `session.krec`，每次回放新建 `play-.../`。
原有游戏目录里的存档不会被录制或回放改写；录制产生的新进度也不会自动导回。
所有资源包括三个 DAT 都复制到各运行目录的 EXE 旁，不依赖原版工作目录。
每份快照/运行目录会占用一份游戏资源的空间，所有产物保留。

回放指定会话：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\replay_session.ps1 -Mode play -Session "完整会话目录"
```

当前限制：不在录制/回放期间修改游戏内键位；这样做会停止会话并报错。也不要修改
`initial/` 下的任何文件。旧版录制与更新后的 EXE 不混用：每个会话保留自己的程序。
首版不支持中途开始、快进、逐帧、输入编辑或快照恢复。

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
- Session identity is SHA256 over the exact copied EXE, assets, configs and initial
  saves. The launcher verifies both the initial snapshot and every run copy.
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
