# Savage Wind Recomp — Cheats Extension Fork

> **Original project and credits**  
> This fork is based on **[Savage Wind Recomp](https://github.com/Revivedx/SavageWindRecomp)**, originally created and maintained by **[Revivedx](https://github.com/Revivedx)**, with contributions from the upstream project contributors. Full credit goes to the original author and contributors for the work this fork builds upon. The upstream project and its license notices are acknowledged and should remain preserved.

## Purpose of This Fork

The purpose of this fork is to extend **Savage Wind Recomp 0.4.1-alpha.4** with support for two optional cheats for **The Legend of Zelda: Breath of the Wild — Nintendo Switch, version 1.9.0**:

- **9999 Map Stamps Limit** — intended to raise the map-stamp limit from 100 to 9,999.
- **Motorcycle in All Regions** — intended to allow the Master Cycle Zero to be used in regions where its use is normally restricted.

This repository documents changes made to the Savage Wind Recomp project itself. It does not distribute game data or proprietary game material.

## Current Development Status

| Feature | Status | Summary |
|---|---|---|
| Motorcycle in All Regions | **Implemented; successfully tested** | The cheat was implemented and successfully tested in the Gerudo Desert. The test session remained stable. This confirms the reported test scenario, but does not claim exhaustive testing of every region or gameplay condition. |
| 9999 Map Stamps Limit | **In progress; not validated** | The investigation identified the game-side routines involved in the original 100-stamp limit and the map-marker workflow. Implementation tests did not produce the intended in-game result, so this cheat must not be considered functional or complete. |

## Changes Made to the Savage Wind Recomp Project

The changes below concern the fork's application, configuration, build, and diagnostic infrastructure. They are described at a project level and intentionally omit game-specific implementation details.

### 1. Cheat catalog and manifest support

- Added a structured catalog for the fork's built-in cheats.
- Added individual entries for **Motorcycle in All Regions** and **9999 Map Stamps Limit**.
- Each entry can expose user-facing information such as its display name, description, compatibility status, and availability.
- The catalog distinguishes between a cheat that has a usable implementation and one that is still experimental or incomplete.

**Purpose:** provide a consistent way for the application to identify and present supported cheats without treating every catalog entry as a working feature.

### 2. Launcher integration

- Added a **Cheats** area to the Savage Wind configuration interface.
- Integrated discovery of the cheat entries provided by the fork.
- Added individual enable/disable controls for the supported entries.
- Made the interface communicate when a listed feature is experimental or not yet validated.

**Purpose:** allow users to manage the fork's optional features through the existing configuration application instead of editing project settings manually.

### 3. Persistent cheat settings

- Added storage for the user's cheat selections in a separate Savage Wind configuration file.
- Added loading of those settings during application initialization.
- Kept the cheat selections independent, so changing one does not require changing the other.
- The selected state is applied when the application starts; a restart may be required for a change to take effect.

**Purpose:** preserve the user's choices between sessions and keep cheat preferences separate from the original application's general settings.

### 4. Recompilation and compatibility safeguards

- Integrated the fork's optional cheat handling into the existing recompilation workflow.
- Added compatibility checks so experimental changes are restricted to the intended Switch 1.9.0 target build.
- Added validation intended to prevent an experimental modification from being applied when the expected target does not match.
- Kept incomplete features distinguishable from validated ones.

**Purpose:** reduce the risk of applying a version-specific modification to an unsupported target and make the experimental status of each feature explicit.

### 5. Build, packaging, and update workflow

- Updated the project's build and distribution process to include the fork's cheat catalog files.
- Updated the existing-install workflow to account for the built-in cheat entries.
- Added backup-aware handling for replaced built-in configuration entries while preserving unrelated user data and third-party entries.

**Purpose:** make the fork's application-side additions available in packaged builds and reduce the chance of overwriting user configuration during an update.

### 6. Diagnostics and development tests

- Added isolated tests for the fork's cheat catalog, configuration handling, and supporting host-side logic.
- Added diagnostic work to investigate how the map-stamp limit behaves while the map interface is used.
- Used read-only observations during the stamp investigation to follow the relevant execution flow without treating diagnostic observations as proof that the cheat works.
- Kept automated test results separate from in-game validation.

**Purpose:** improve the reliability of the fork's own infrastructure and provide evidence for further investigation without claiming that isolated tests prove correct gameplay behavior.

## Development Notes

### Motorcycle in All Regions

The Motorcycle in All Regions implementation is the first of the two target features to reach a successful in-game test. It was tested in the Gerudo Desert, and the session remained stable. Further testing across additional regions and gameplay conditions may still be useful, but the implementation has passed the reported test scenario.

### 9999 Map Stamps Limit

The investigation successfully narrowed down and identified the game-side routines associated with the original 100-stamp limit and the map-marker handling process. However, identifying the relevant routines did not by itself resolve every condition involved in placing and managing additional markers.

The implementation attempts did not pass the in-game tests. As a result:

- The **9999 Map Stamps Limit** feature remains unfinished and unvalidated.
- The presence of a setting, catalog entry, or experimental implementation must not be interpreted as proof that 9,999 stamps can be placed safely.
- Further work is required to understand and validate the complete marker-management and save-data behavior before this feature can be considered complete.

Users should keep this feature disabled unless they are deliberately testing an experimental build, and should maintain backups of their saves when testing unfinished functionality.

## Compatibility Scope

- **Base project:** Savage Wind Recomp 0.4.1-alpha.4.
- **Target game release:** Nintendo Switch version 1.9.0.
- **Other game versions and platforms:** not covered by the current validation. No compatibility with the Wii U release is claimed.

## Copyright and Distribution Notice

- This fork does **not** include or distribute proprietary BOTW game code, game functions, symbols, offsets, disassemblies, executable files, game binaries, assets, or extracted game data from either the **Wii U** or **Nintendo Switch** release.
- No game-specific code or binary material is reproduced in this README.
- The repository is intended to contain changes to the Savage Wind Recomp project and its supporting documentation only. Users must provide their own legally obtained game materials where required by the upstream project.
- *The Legend of Zelda: Breath of the Wild* and related Nintendo properties belong to Nintendo. This is an unofficial fan project and is not affiliated with, endorsed by, or sponsored by Nintendo.
- Preserve the upstream `LICENSE` file and all applicable third-party license and copyright notices when redistributing this fork.

## Acknowledgements

Thanks again to **[Revivedx](https://github.com/Revivedx/SavageWindRecomp)** and the Savage Wind Recomp contributors for creating and maintaining the original project on which this fork is based.
