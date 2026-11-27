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
- First visible island-map scene rendered with the PS2 Graphics Synthesizer
- Ocean, beach, layered jungle areas, river, small village, and simple palm silhouettes
- D-pad / left analog movement with diagonal speed normalization
- Player marker and basic status bars
- GitHub Actions build and ELF artifact

This is an early 2D visual exploration milestone, not a complete 3D game. The next steps are a proper 3D camera and terrain, collision, detailed environment assets, NPCs, missions, and save/load.
