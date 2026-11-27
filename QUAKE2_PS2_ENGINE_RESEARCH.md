# Quake II PS2 Engine Research / Porting Plan

## Reference project

- Project: Quake II PS2 homebrew port
- Repository: https://github.com/glampert/quake2-ps2
- License shown by GitHub: GNU General Public License v2.0 (GPL-2.0)
- This repository is used as an engineering reference; no Quake II source files or original game assets are copied into this project by this note.

## Important license and asset boundary

If we directly copy, modify, or combine GPL-2.0 Quake II source code into a distributed game, the resulting derivative work must follow the applicable GPL-2.0 requirements, including source-code and license notices. Check the exact license files and obligations before publishing binaries.

Quake II game data, textures, maps, sounds, models, names, and other original assets are separate from the engine source. Do not assume the engine license grants permission to redistribute those assets. Island of Ashes should use original or separately licensed assets.

## Current project status

The current `far-cry-ps2` project is an original PS2SDK C prototype with:
- A perspective-projected 3D island made from triangles
- First-person camera rotation and movement
- Three exploration checkpoints
- A basic HUD and stamina

It is not yet a Quake II engine port and does not yet have a full BSP renderer, hardware depth buffer, texture pipeline, collision system, animation system, or Quake II's game engine.

## Recommended integration approach

1. Keep the current playable prototype on `main` as a safe baseline.
2. Study the Quake II PS2 port's architecture and build requirements before importing any code.
3. If choosing a direct GPL-2.0 engine port, do it on a separate branch and retain required copyright/license notices.
4. First prove a clean PS2SDK build of the reference engine in an isolated branch or test repository.
5. Identify the minimum modules needed for an original island exploration game: renderer, input, map loading, collision, audio, and game loop.
6. Replace Quake II-specific game logic and all protected game data with original Island of Ashes content.
7. Merge only after the test build works and the license implications are understood.

## First technical milestones

- Confirm the reference port's exact build instructions and required PS2SDK version.
- Build the unmodified reference port separately.
- Document the rendering and input APIs used by the port.
- Make an original test map or scene.
- Add player movement and collision on the test scene.
- Decide whether to continue the existing renderer or adopt the reference engine.

Do not describe the project as a Far Cry port or as a complete Quake II-based game until the relevant engine code has actually been integrated and tested.
