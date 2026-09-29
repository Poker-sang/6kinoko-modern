# 6kinoko-modern local patches

Upstream base: SDL 3.4.16; original archive checksum remains in
`docs/modern-platform/dependencies.json`. This vendored tree is modified.

- `CMakeLists.txt`: skip the D3D9 header probe when SDL_RENDER_D3D is OFF.
- `src/video/windows/SDL_windowsvideo.c` and `.h`: compile the D3D9 DLL loader
  and SDK declarations only when SDL_VIDEO_RENDER_D3D is enabled. Otherwise
  preserve the public SDL_GetDirect3D9AdapterIndex symbol with SDL_Unsupported
  and return -1, matching SDL's unsupported-platform convention.
- The parent build forces SDL_RENDER_D3D OFF and rejects D3D9/D3DX includes.
  The game uses SDL GPU; neither game backend calls this SDL adapter helper.
  D3D12, Vulkan, DXGI and audio configuration are unchanged.
- `src/gpu/metal/SDL_gpu_metal.m`: texture upload honors
  `SDL_GPUTextureTransferInfo.pixels_per_row` and `rows_per_layer`, including
  their documented tightly packed defaults. Previously Metal used destination
  width/height for the source strides, reading application padding as pixels.
  The game uploads RGBA rows aligned to 256 bytes; widths not divisible by 64
  were corrupted on Metal. No game shaders, depth state or asset sizes are
  changed to work around this backend defect. Preserve this correction until
  an upstream version provides equivalent handling.

Carry these changes forward when updating SDL; do not present this tree as an
unmodified upstream release. Original SDL license notices remain intact.

Mixed-zero stride follow-up: Metal upload AND download now default each zero
field independently, matching D3D12 and Vulkan. The initial local upload fix
copied Metal download's incorrect combined-zero check. The contract no longer
expects both fields to reset: it tests all four combinations across 3D slices.
Normal game uploads specify both fields and retain their accepted behavior.
