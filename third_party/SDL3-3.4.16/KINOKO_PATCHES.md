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
  D3D12, Vulkan, Metal, DXGI and audio configuration are unchanged.

Carry these changes forward when updating SDL; do not present this tree as an
unmodified upstream release. Original SDL license notices remain intact.
