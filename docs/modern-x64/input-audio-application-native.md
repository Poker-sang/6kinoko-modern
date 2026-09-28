# Native input, audio and application boundary

This batch migrates in-memory boundaries only. Input assignments remain 68-byte
records; counters, game values, audio generation handles and the CV3 header retain
their original widths. No gameplay, key mapping, audio fade or scene order changes
are intended. The user accepted the preceding modern-act-map-02 batch; that is
user feedback, not agent execution.

- Input manager embeds a complete native SqPlus reference; script instance
  temporaries and host input/map class storage grow with that reference. Field
  publication uses offsetof. Host allocation covers the actual manager size.
- Input physical/cluster update keeps the historical mixed id/address result in
  intptr_t. Consumers still ignore that result. Typed fastcall method slots pass
  the explicit reserved argument at both widths; cluster adapters preserve the
  base-first relationship. Assignment files continue to serialize only the fixed
  assignment record, never the native manager layout.
- Audio owns buffers with make_unique and the manager as a native static object;
  all live accesses use named fields. Path, queues, buffer stores and critical
  sections retain their true native pointers. Retired opaque slots stay reserved;
  predecessor/successor remain numeric handles. Decoder malloc owners use the
  architecture-independent memory helper. Trace names never dereference low words.
- Application configuration is copied as a typed value. Scene/manager/transition
  dispatch passes the reserved argument explicitly, including creation, scene
  enter/leave and deletion. The render queue receives the original camera pointer.
- IME receives HWND/UINT/WPARAM/LPARAM and stores HIMC/HWND without integer
  truncation. Windows IME remains a platform dependency, not a portable service.

The compile-only kinoko_input_audio_width_compile target covers 12 production
translation units and one layout/signature assertion unit. It does not link or
execute the game. Win32 historical layout assertions are retained; native-width
relationships and fixed serialized widths are checked separately. Existing
application/input/game lifecycle contracts are adapted and compiled, not run.

Remaining work: full-game host/actor boundaries, bitmap/file services and VM
bridges, then original DAT bytecode/save compatibility. Full x64 compilation is
not proof of runtime compatibility. Linux/macOS still require remaining Windows
services to be migrated. See BUILD.md for final artifacts and exact evidence.
