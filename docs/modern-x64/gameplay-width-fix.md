# x64 gameplay follow-up (2026-09-28)

User reports from `modern-x64-integer-01`: missing digits, incorrect stomp/kick
behavior, background not scrolling on tall maps, missing map-road explosion.
The user reproduced with the diagnostic launcher; no agent gameplay/test run.

## Evidence and changes

- `retdec_trace.log` records ACT callback result `0xAEB6A250`, the VM low word.
  `kinoko_act_layer_update` forwarded it as signed status and frame update stopped
  at its first negative layer. Return stable SQ_OK after Sqrat Execute, retaining
  its existing policy of ignoring script call status and its error-handler flag.
- Digits use PlayerStatus.cv4 DrawNumber -> BitBlt -> sprite set_rect. The latter
  called its reconstructed set_rect_pivot virtual entry through __thiscall without
  the explicit reserved argument. On x64 texture/UV/size parameters shifted. Use
  the shared native-width method adapter; a regression contract verifies digit
  texture, dimensions, UVs, pivot, scale and homogeneous coordinates.
- Squirrel SQObjectPtr float assignment left high union bytes from previous wide
  values. VM equality/table keys compare all union bytes. Canonicalize float writes
  as the upstream float constructor does; also zero compiler float constants and
  external raw float values. Contract covers dirty register -> float equality,
  numeric table lookup and zero. This is a plausible cause of state/interaction
  errors, not yet user confirmation of the 毛玉 behavior.
- User fault log `fault-20260928-135229-726-p47720.log` has RVA 0x8BAC7 reading
  0x3DEB2E00. IDA database 9256dec4 maps it to animation-list clear; caller at
  0x14006F080 truncates both animation and priority container addresses. Pass
  native member addresses directly in actor_cleanup.cpp. This affects cleanup
  on transitions as well as exit.

IDA inspected the delivered PE SHA256
12a7509516cd212875b5124e1ff63f3490257da54982eeb2833c4fe22bd8e0a2.
E-imports: queried imports include USER32 window/DC/timer services; rendering and
platform are still Windows-hosted SDL GPU. This was targeted fault analysis,
not a security/import-completeness audit. Original CV4 was read statically.

Background scrolling and road effects use the affected script/ACT paths; no
special-case visual or gameplay compensation is added. The user subsequently confirmed all four reported symptoms resolved in
`modern-x64-gameplay-01`. This is user verification, not an agent test run.

## Validation

Source 54995147 built the x64 game and staged DAT successfully. The Win32 game
also compiled, but the all-target build exposed a macro collision in the added
float contract: internal VM headers preceded SqPlus declarations. Reorder only
the contract includes; game source is unchanged. Preserve the failed build logs.
Fresh builds after this contract-only correction are pending.
No game, CTest or contract execution performed by the agent.
Existing diagnostic log/dumps and all build artifacts remain in place.
