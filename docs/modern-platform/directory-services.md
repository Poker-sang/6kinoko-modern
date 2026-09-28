# Directory search and startup paths (2026-09-29)

User accepted modern-x64-locks-03. This is user feedback, not agent runtime testing.

ACT FindFirst/Next/Name/Close now use an opaque directory search owner. The Windows
backend retains FindFirstFileA/FindNextFileA/FindClose and native ANSI filenames.
POSIX uses opendir/readdir/closedir with UTF-8 names and native directory order.
The final pattern component supports * and ?; brackets remain literal. Both slash
styles are accepted, *.* includes extensionless entries, and foo.* includes foo.
POSIX matching remains case-sensitive; Windows matching follows Windows. This is
not an emulation of every DOS wildcard/short-name rule. No sorting or file-only
filter is introduced. Failed next preserves the last successful name. Closing a
search releases its native handle once; ACT IDs/counts/wrap and owner cleanup remain.

Application startup selects the executable directory through a portable service
before launching workers. SDL supplies the path on Windows/Linux; macOS uses the
actual binary path rather than SDL's bundle Resources default, keeping all three
DAT beside the executable. A failure aborts startup instead of searching an
unrelated working directory. The duplicate process-path cwd setter and unread fixed-size path cache are removed.
Existing DAT filename lookup and save formats remain.

Contracts cover matching, missing paths, extensionless entries, directories,
concurrent search ownership, EOF, and executable-directory selection. Contracts
are compiled only. Full Linux/macOS games remain blocked by window/COM/IME/font
and other host boundaries; this batch delivers portable services, not a playable
non-Windows release.
