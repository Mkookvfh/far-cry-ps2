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
- Perspective-projected 3D island terrain built from triangle meshes with height variation, beaches, hills, and a raised mountain area
- 3D camera transforms with yaw and pitch; yaw rotates through a full 360 degrees
- 3D palms, village huts, roof geometry, lookout tower, and a raised golden mission beacon
- Triangle-based 3D scene renderer with far-to-near painter ordering and a 2D HUD overlay
- First-person HUD mockup: crosshair, objective panel, minimap, and status bars
- D-pad / left analog movement with analog deadzone and normalized diagonal speed
- Right analog camera-look input with full horizontal rotation and bounded vertical pitch
- Three-checkpoint island trail with a moving golden beacon and objective progress
- Sprinting with R1, increased movement speed, stamina drain, and stamina recovery
- Objective completion feedback in the HUD and console
- Forward and sideways movement follows the current facing direction
- Island movement limits and first-person HUD
- GitHub Actions build and ELF artifact

This is a small playable 3D gameplay slice, not a complete game: move around the island, follow the golden beacon through three checkpoints, and finish the trail. Hold R1 while moving to sprint; stamina drains while sprinting and recovers when you stop sprinting.\n\nThe island now uses real 3D world coordinates and perspective projection rather than the previous screen-space scenery. The renderer currently sorts triangles from far to near instead of using a complete hardware depth-buffer pipeline, so intersecting geometry and clipping still need refinement. Next milestones are emulator/console verification, improved depth/clipping and collision, textured assets, NPCs, expanded missions, and save/load.
