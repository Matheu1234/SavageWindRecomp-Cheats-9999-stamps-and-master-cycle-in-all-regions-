# Cemu graphic packs → Savage Wind (versão v4)

**Atualização:** ambos os cheats têm handlers experimentais para BOTW Switch 1.9.0.
A implementação `stamps_9999` altera apenas a capacidade interna em RAM. **O save ainda serializa 200 marcadores no total**, e testes de persistência/estabilidade no Windows são indispensáveis. Consulte `STAMPS-CHEAT-V4-PT.md`.

## Registro histórico da investigação anterior (antes da v4)

# Cemu graphic packs → Savage Wind: source audit and safe groundwork

**Status: NOT functional game cheats.** No ARM64 guest sites have been identified or replaced, and no mod/cheat interface has been activated. The two patches provided by the user are included **only** as references in `reference/cemu_graphic_packs`.

## Verified references

| Pack | Wii U module | PowerPC patch | Intent |
|---|---|---|---|
| `9999StampsLimit` | `0x6267BFD0` | `0x02E9F2C0 = li r0, 0x270F` | Raises one 100-stamp limit comparison to 9,999 |
| `MotorcycleAllRegions` | `0x6267BFD0` | `0x02A32A30 = li r3, 1` | Forces a specific boolean result to true; do **not** assume every other prerequisite is preserved |

These addresses are **PowerPC offsets in the Wii U executable**, not addresses or symbols in Savage Wind's Switch ARM64 guest code. Transplanting addresses or instruction words is wrong and may corrupt the game.

## Work performed

- Source copies stored as references; input format and values validated by `python3 tools/check_cemu_packs.py --verify`.
- `runtime/sw_cheats.h` and `runtime/sw_cheats.c`: isolated, host-side example predicates. They compile with the Savage Wind host code automatically through the Makefile wildcard, but **are not called** from game execution.
- `make test-sw-cheats`: standalone unit tests for the predicates and the configuration parser.

## Missing integration work — must be finished before any user-facing release

1. For **Switch 1.9.0** (not Wii U v208), identify the exact function(s) and instruction(s) handling stamp count, marker array capacity, save serialization/deserialization, and the Master Cycle Zero region guard. Use `tools/nso.py` and `tools/game_sites.py` against the player's local `extracted/exefs/main`, whose build ID must be `cd57b23fa4bbad65`. There are no Switch code bytes in the open-source ZIP.
2. Add versioned, unique-site assertions and original-instruction validation to `tools/game_sites.py` / `tools/patches.py`, or safe host overrides with equivalent semantics. Refuse the patch if a site is missing, ambiguous, or changed in another version.
3. **9,999 stamps**: verify UI loop bounds, backing capacity, combined marker count, serialization, and backup/recovery. The Cemu wiki warns that **pins + stamps over 200 can disappear** with the Wii U patch. Making only a one-instruction change is not safe. Keep this cheat **off** until end-to-end validation.
4. **Motorcycle everywhere**: locate which predicate is replaced, verify DLC/quest-unlock status and environmental edge cases; test in Gerudo Desert, Eldin, shrines, Divine Beasts, and ordinary regions. Do not infer the original patch preserves DLC checks; it writes 1 to `r3`.
5. Add real on/off toggles only after code hooks work, and implement a genuine mods/cheats loading interface if needed. Retest clean saves, DLC states, all game versions supported, crashes, and Windows native build using MSYS2/UCRT64/Clang.
6. The open source project **does not distribute proprietary game code**. Compile `swgame.dll` locally from your own game files; do not distribute it bundled with copied game code.

## What is NOT tested

No BOTW game boot, stamp save round-trip, Master Cycle Zero summon, or Windows `.exe` build has been performed in this environment. Unit tests verify **only the isolated example logic and reference parsing**.

## Local diagnostic for the owner's Switch game executable

Run `python tools/check_switch_main.py extracted/exefs/main` from this source folder to verify the Switch main NSO build ID. Optional `--symbols` prints candidate exported symbols but **does not validate patch addresses**. This script does not read ROMFS, modify game files, or run the title.

## Analysis of the player's supplied BOTW 1.9.0 `main` (stage 2)

The exact NSO supplied for investigation was **read-only** throughout. The file was
validated with `tools/check_switch_main.py`: build ID `cd57b23fa4bbad65`;
SHA-256 `f6f0b20bb1f2b67d164c24871cd473147eef1d285f12e7bd492e3126c6022e27`.
Its `.text` is 0x15a1c64 bytes; the rebuilt memory image is 0x1e98000 bytes.

A new diagnostic, `python tools/cheat_site_audit.py <path-to-main> --json`,
finds code references to relevant literal names and enumerates 100-related
instructions; it *does not patch anything*. Candidate addresses use Savage
Wind's `0x7000000000` guest base, and must not be treated as approved sites.
Among the direct ADRP+ADD string references observed in this build:

| Clue | Referencing instruction (guest virtual address) |
|---|---|
| `mc_openStampMenu` | `0x7000dd8208` |
| `Pa_StampNum_00` | `0x7000dd86b4` |
| `Pa_StampBox_00` | `0x7000df6174`, `0x7000df618c` |
| `BikeAppearPointDecide` | `0x7000b38be8` |
| `BikeAppearPointPreview` | `0x7000b38bbc` |
| `IsGet_Obj_Motorcycle` | `0x7000d27400`, `0x7000e79394` |

Only six `MOVZ W0,#100` instructions and 101 `CMP Wn,#100` instructions were
found by the exact checks; this is **not proof** that any of them is the
stamp-limit function. The string references show menu/event code, not yet
the count guard, the underlying data capacity, or the save serializer.
Likewise, the motorcycle strings may be events or UI states; none has been
verified as the regional summon predicate.

`tools/test_cheat_site_audit.py`: four synthetic tests of the instruction
recognizers and ADRP/ADD reference finder passed. `tools/game_sites.py find`
completed without error for the supplied NSO. `make test-sw-cheats` passed.
The broader `tools/test_game_sites.py` requires `capstone`, absent from the
current build container; attempts to install it were blocked by unavailable DNS.

**No functional cheat, game boot, save round-trip or Windows build is claimed.**
A decompiler/call-graph review (for example, the project's Ghidra workflow)
remains necessary, followed by real gameplay and save tests. This report omits
the proprietary NSO; do not put it in a public source archive.
