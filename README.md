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
- Forward and sideways movement follows the current facing direction
- Island movement limits and first-person HUD
- GitHub Actions build and ELF artifact

This is still a 2D screen-space prototype that imitates a first-person view; it is not a true 3D camera or complete game. Next steps include verifying the scene on a PS2 emulator/console, then adding real 3D geometry, camera rotation, collision, environment assets, NPCs, missions, and save/load.
