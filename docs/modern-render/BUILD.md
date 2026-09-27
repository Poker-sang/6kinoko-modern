# Build handoff

The first complete Win32 Release build, modern-render-01, succeeded at
c9a0fa6dcc540b4f2aa8a71d6d61682c92366fcd with all three DAT hashes verified.
Game and contract executables were not run. Its artifacts/logs are retained.

A follow-up makes the header transition table an inline C++17 variable so the
shared inline algorithm refers to the same table in every translation unit.
The final build is modern-render-02; its exact source and hashes will be copied
from the build manifest after successful compilation/staging.
