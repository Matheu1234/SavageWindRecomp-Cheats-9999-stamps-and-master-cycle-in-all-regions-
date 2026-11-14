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

## Supported Platform and Runtime Requirements

### Fork target

This fork's **documented build target is Windows 10 or later (x86-64)**. The fork-specific build and packaging workflow described below uses the Windows toolchain and has not been validated as a Linux or macOS build. This is a scope statement for this fork; it does not mean that the original Savage Wind project is limited to Windows.

For running Savage Wind, the upstream 0.4.1-alpha.4 requirements include:

- A Vulkan 1.3-compatible graphics device and driver.
- A CPU supporting SSE4.2.
- Approximately 20 GB of free disk space for setup and generated data.
- The user's own legally obtained Nintendo Switch game files, updated to the supported game version. These files are not included in this repository.

The cheat work documented here targets the **Nintendo Switch release, version 1.9.0**. The Wii U release is not a supported target of this fork's cheat implementations. No Linux, macOS, or Wii U compatibility is claimed for the fork-specific changes.

## Building the Source on Windows (x86-64)

The steps below build the fork from source using the Windows development environment described by the upstream Savage Wind Recomp 0.4.1-alpha.4 build guide. Run commands from the repository root unless a step says otherwise.

### 1. Clone your fork

Clone your own public fork and enter its directory:

```bash
git clone https://github.com/<YOUR_USERNAME>/<YOUR_FORK>.git
cd <YOUR_FORK>
```

Replace the placeholders with your GitHub username and fork repository name.

### 2. Install the Windows build environment

Open **PowerShell** in the repository root and run:

```powershell
powershell -ExecutionPolicy Bypass -File tools\setup_windows.ps1
```

The setup script prepares the MSYS2 UCRT64 toolchain and the Python environment used by the project. The upstream setup includes the compiler and linker, Make, SDL3, FFmpeg, Vulkan development files, shaderc, Vulkan Memory Allocator, validation layers, and Python 3.12 with the project's required tooling packages.

Allow the setup process to finish before proceeding.

### 3. Open the MSYS2 UCRT64 terminal

Launch **MSYS2 UCRT64** from the Start menu. Change to the cloned repository directory. For example:

```bash
cd /c/path/to/<YOUR_FORK>
```

Add the UCRT64 tools and libraries to the current shell's `PATH`:

```bash
export PATH="/c/msys64/ucrt64/bin:/c/msys64/usr/bin:$PATH"
```

Keep using this MSYS2 UCRT64 terminal for the remaining build commands.

### 4. Compile the application and runtime

For a quick compilation check of the runtime objects:

```bash
make runtime-objs
```

To build the application from source:

```bash
make -j
```

The normal build uses the project's configured build directory. If the local checkout does not contain generated game-code output, the application can still be built; the game-code setup/recompilation step is handled locally through the application's setup flow when the user supplies their own game files.

### 5. Run the fork's isolated tests

The fork adds tests for the cheat catalog and its host-side configuration/support logic. Run:

```bash
make test-sw-cheats
make test-cheat-catalog
```

These tests validate the fork's isolated support code. **They do not prove that an unfinished cheat works in gameplay**, and they do not replace testing the application on a Windows machine with the user's own game files. In particular, the 9999 Map Stamps Limit cheat remains unvalidated as described above.

### 6. Build a portable Windows distribution ZIP (optional)

To create a packaged Windows distribution rather than only compiling the local application, run these commands in the MSYS2 UCRT64 terminal:

```bash
make deps-windows
make gen-helper
tools/fetch_zig.sh
make dist
```

- `make deps-windows` prepares the Windows FFmpeg dependency bundle. The upstream build guide notes that this step downloads FFmpeg source and requires NASM; it is generally a one-time dependency build.
- `make gen-helper` builds the standalone game-code generator used by the application's setup flow.
- `tools/fetch_zig.sh` fetches the Zig compiler used by that setup flow.
- `make dist` creates the distributable ZIP under `build/dist/`, with a filename following the `SavageWind-<version>-windows.zip` pattern.

The fork's packaging changes include its cheat catalog files in the distribution. The package must **not** contain proprietary game code, game binaries, extracted game data, or user save data. The game code required at runtime is generated locally by each user from their own game files; it must not be uploaded or redistributed.

### 7. Test the packaged build

Before distributing a ZIP, extract it into a new writable folder on Windows (not `Program Files`), launch it, and verify that the application starts. Complete the application's game-file setup only with the user's own legally obtained files. Keep the test installation and its generated data separate from the source repository.

For additional details or troubleshooting, consult the upstream [Windows build guide for Savage Wind Recomp 0.4.1-alpha.4](https://github.com/Revivedx/SavageWindRecomp/blob/v0.4.1-alpha.4/docs/BUILD-windows.md). The commands above describe the upstream build environment together with this fork's added test and packaging targets; successful compilation and gameplay must still be verified on the machine performing the build.

## Copyright and Distribution Notice

- This fork does **not** include or distribute proprietary BOTW game code, game functions, symbols, offsets, disassemblies, executable files, game binaries, assets, or extracted game data from either the **Wii U** or **Nintendo Switch** release.
- No game-specific code or binary material is reproduced in this README.
- The repository is intended to contain changes to the Savage Wind Recomp project and its supporting documentation only. Users must provide their own legally obtained game materials where required by the upstream project.
- *The Legend of Zelda: Breath of the Wild* and related Nintendo properties belong to Nintendo. This is an unofficial fan project and is not affiliated with, endorsed by, or sponsored by Nintendo.
- Preserve the upstream `LICENSE` file and all applicable third-party license and copyright notices when redistributing this fork.

## Acknowledgements

Thanks again to **[Revivedx](https://github.com/Revivedx/SavageWindRecomp)** and the Savage Wind Recomp contributors for creating and maintaining the original project on which this fork is based.
