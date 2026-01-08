# Status (Jan 2026)

## Summary
- Simulator is stable (segfault fixed) and CI smoke test builds + runs headless.
- Feed scene now uses large pixel-art food with centered text and a two-phase flow.
- Marching music updated to "Johnny I Hardly Knew Ye / When Johnny Comes Marching Home".
- Build warnings cleaned up; PlatformIO config updated to new `build_src_filter`.

## UI/UX Changes
- **Title:** "BOO!" is now uppercase so it renders correctly with the current font.
- **Feed scene:** split into two screens:
  - Screen 1: centered food icon + centered food name.
  - Screen 2: eating animation + centered "SO YUMMY!" near the bottom.
- **Food art:** switched from text-only to pixel-art icons and scaled up for readability.
- **Celebration:** top "SO YUMMY!" text is centered.

## Audio/Music
- **Marching scene** uses the "Johnny I Hardly Knew Ye / When Johnny Comes Marching Home" melody (C major), with a marching tempo.
- Fixed potential out-of-bounds note indexing in the march sequence.

## Simulator Stability
- Fixed a recursion crash in `M5Canvas::print()` by explicitly calling the global `::printf` in the simulator implementation.

## Build/Config Updates
- PlatformIO config now uses `build_src_filter` (deprecated `src_filter` removed).
- Local library deps are pinned to repo paths:
  - `lib/BooGame`
  - `lib/M5CardputerSim`
- `.pio-home/` is ignored to avoid committing PlatformIO cache.

## CI (GitHub Actions)
- **Smoke test** added:
  - Builds simulator.
  - Runs the simulator headlessly under `xvfb` for a full scene pass.
  - Uses `BOO_SMOKE=1` and `SDL_AUDIODRIVER=dummy`.
- **Smoke mode behavior:**
  - Each scene (Feed, Dance, March, Game) runs for 5 seconds.
  - Audio muted and app exits cleanly afterward.

## How to Run the Smoke Mode Locally
```bash
BOO_SMOKE=1 SDL_AUDIODRIVER=dummy ./.pio/build/simulator/program
```

## Known Constraints / Notes
- Font renderer is uppercase-only; lowercase text will not display correctly unless the font is extended.

