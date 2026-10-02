# Separate engine and authored Mod content

The user requested that concrete Mod content stay outside the engine repository,
and selected a local independent project without a remote repository.

The classic course recipe, presentation script, dedicated generator and its
resource/original-script fixtures have moved to
`C:/WorkSpace/6kinoko-mod-content`. They were removed from the engine source tree
and from new native package contents. Historical Git commits and delivery notes
remain available; this change does not rewrite history. General ACT editing,
resource loading, package installation and isolated sessions/saves remain here.

Engine snapshot `4c980562` was packaged using the unchanged existing Windows x64
game binary. Fresh CMake configuration succeeded in
build-runs/mod-content-separation-01 after removing the dedicated content target.
The local content project owns current gameplay fixes and their checks. Its
README records the delivery path and validation details. No content repository
was created remotely and the game was not run by the agent.
