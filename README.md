# Island of Ashes — PS2 Prototype

An original tropical-island exploration prototype for PlayStation 2, built with PS2SDK.

> This is an independent project. It is not an official Far Cry release and does not use Ubisoft game assets, characters, maps, or story.

## Build

The GitHub Actions workflow builds the PS2 ELF using the PS2 development environment and uploads it as an artifact when successful.

For a local PS2SDK setup:

```sh
make
```

Output: `far-cry-ps2.elf`

## Current prototype

- PS2SDK startup and controller initialization
- First-person-style tropical island view with sky, sea, distant hills, jungle, a path, village huts, and lookout tower
- Simple screen-space perspective scene built from colored shapes for an early PS2 graphics milestone
- First-person HUD mockup: crosshair, objective panel, minimap, and status bars
- D-pad / left analog movement with analog deadzone and normalized diagonal speed
- Right analog camera-look input (yaw/pitch approximation) with bounded look range
- Three-checkpoint island trail with a moving golden beacon and objective progress
- Sprinting with R1, increased movement speed, stamina drain, and stamina recovery
- Objective completion feedback in the HUD and console
- Forward and sideways movement follows the current facing direction
- Island movement limits and first-person HUD
- GitHub Actions build and ELF artifact

This is a small playable gameplay slice, not a complete game: move around the island, follow the golden beacon through three checkpoints, and finish the trail. Hold R1 while moving to sprint; stamina drains while sprinting and recovers when you stop sprinting.\n\nThe scene is still a 2D screen-space prototype that imitates a first-person view; it is not a true 3D camera. The next milestones are emulator/console verification, real 3D geometry and camera, solid collision, environmental assets, NPCs, expanded missions, and save/load.
