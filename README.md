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
- Basic player movement state from the D-pad / left analog stick
- Build workflow and artifact output

This is an early technical foundation, not a complete game. The island scenery, collision, missions, NPCs, and save/load are planned as later original systems.
