# Separate engine and authored Mod content

The user requested that concrete Mod content stay outside the engine repository,
and selected a local independent project without a remote repository.

All concrete Mod projects, recipes, artwork and dedicated generators/tests have
moved to `C:/WorkSpace/6kinoko-mod-content`, including the remaining content
examples and delivery notes. They are removed from the engine source tree and
new native packages. Historical Git commits remain available; this change does
not rewrite history. General ACT editing, archive reading, resource loading,
registration, package installation and isolated sessions/saves remain here.

Engine snapshot `4c980562` was packaged using the unchanged existing Windows x64
game binary. Fresh CMake configuration succeeded in
build-runs/mod-content-separation-01 after removing the dedicated content target.
The local content project owns current gameplay fixes and their checks. Its
README records the delivery path and validation details. No content repository
was created remotely and the game was not run by the agent.

## Complete separation check

Source/package snapshot `bb4afce1` completes separation beyond the first classic
course batch. Engine CI no longer invokes content-specific Python tests and the
native registry contract no longer depends on a concrete gameplay script. Its
synthetic registry/spawn/error checks remain. Dedicated checks and their script
fixture now compile in the independent local content project.

Fresh engine CMake configuration succeeded in build-runs/mod-engine-only-17.
Four generic Python suites passed, as did the migrated content resource checks.
The edited generic native script contract and independent content script
contract were freshly compiled and executed against retained engine libraries.
Logs/fixtures are retained under the content project's
build-runs/content-separation-01. Native gameplay source was not changed.

The package in runtime-builds/mod-engine-only-17 was audited to contain no
bundled Mod projects, recipes, specific generators or specific launchers. The
unchanged earlier Windows x64 game binary is reused; three original DAT files
were staged and SHA256-verified beside it. No game run is claimed. A tracked-file
reference audit found no remaining removed-content references in engine source,
tools, tests, CMake, workflows or current documentation. Existing local build
and runtime artifacts remain available and are not source distribution content.
