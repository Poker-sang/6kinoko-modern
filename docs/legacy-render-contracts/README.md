# Retired graphics fixture evidence

These files are retained as migration reference, not build targets or supported
implementations. They use fake D3D9 COM vtables and cannot exercise the SDL GPU
resource model. Their last supported comparison commit is
`b4a37a1a438db1496ec04f2ef3b97571682048ba`.

The mixed `stage_contract.c` also contains gameplay/serialization assertions.
Those assertions are preserved here rather than discarded; porting that harness
to the modern resource model is still pending. Do not count it as current test
coverage. Its relative includes describe its original location under tests/.

The active GPU resource, matrix, vertices, portable draw-state/resource contracts
and all independent gameplay/VM contracts remain in the build. Compilation does
not establish runtime equivalence; no contract execution is claimed.
