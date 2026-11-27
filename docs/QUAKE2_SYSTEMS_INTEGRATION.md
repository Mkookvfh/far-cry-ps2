# Quake II PS2 Systems Integration — Island of Ashes

## Goal

Evolve Island of Ashes from its single-file PS2SDK prototype into an original tropical-island FPS using the proven architecture of the Quake II PS2 port as a technical base/reference. The intended game remains single-player and uses original Island of Ashes maps, textures, models, audio, characters, and mission content.

Reference port reviewed: Quake II PS2 (GPL-2.0).

## Important status

- This branch is an isolated integration workspace: `integration/quake2-systems`.
- The current prototype is still in `main.c`; this document does not claim that Quake II engine code has already been imported.
- The Quake II PS2 port is a complete engine port, while the current prototype is a small custom renderer. A real integration is an engine migration, not a few lines of code.
- Do not add Quake II retail data files or copyrighted game assets.

## Safe integration scope\n\nThis branch focuses on non-combat engine systems: renderer architecture, movement/collision, input, world loading, models/animation, audio, save/load, and performance. Combat and weapon gameplay systems are outside this work.\n\n## Systems to adapt, in order

### 1. PS2 platform/backend seam
Use the upstream backend pattern to separate engine-facing APIs from PS2-specific implementation. Keep platform services behind small interfaces for graphics, pad input, filesystem, audio, timers, and save storage.

### 2. Renderer and world
Study the upstream `refexport_t` renderer interface and GS/VU1 pipeline. The long-term goal is textured world geometry, proper depth testing, clipping, back-face culling, lightmaps, sky, and model rendering. Replace Quake II-specific visual content with original Island of Ashes assets.

### 3. Player movement and collision
Adapt the Quake II movement architecture (`pmove`) to Island of Ashes movement requirements: camera-relative walking, sprint/stamina, slopes, step-up, gravity, ground detection, and collision response. Keep the current simple movement available until the new controller builds and passes tests.

### 4. Input
Use the upstream DualShock 2 input backend as a reference for analog dead zones, button-to-action mapping, camera look, and controller configuration. Preserve Island of Ashes' preferred controls and mission HUD.

### 5. World loading and collision
Adopt a BSP-style static world representation or an equivalent PS2-friendly spatial structure for visibility and collision. Start with one original test island/map; add map streaming only after a stable renderer and memory budget exist.

### 6. Models and animation
Study the MD2 model pipeline as a lightweight PS2-era reference. Use original or properly licensed models and animation data; do not copy Quake II model/texture files.

### 7. Audio, save/load, and game state
Port platform-independent architecture for audio device output, save slots, configuration, and game state. Save Island of Ashes mission/checkpoint progress using an original save format.

## Integration gates

1. Keep this branch separate from `main`.
2. Make the current prototype build cleanly before importing engine modules.
3. Bring up a minimal upstream-derived engine/backend in a separate test target.
4. Confirm the engine boots and renders an original test scene.
5. Add original island content and movement/collision.
6. Only merge to `main` after CI builds and the scene is checked in PCSX2 or on hardware.

## License and assets

The Quake II PS2 port is GPL-2.0. If GPL-covered source is copied or adapted and distributed, retain required notices and comply with GPL-2.0, including source-code availability obligations for distributed derivative binaries. Quake II game data is separately copyrighted and must not be included. Island of Ashes should use original or separately licensed assets.
