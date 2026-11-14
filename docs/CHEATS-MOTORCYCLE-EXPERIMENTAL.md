# Experimental BOTW 1.9.0 Master Cycle region whitelist

This is **not** a completed or in-game validated cheat. It is a narrow, opt-in
hook in the ARM64 -> C generator for Switch BOTW 1.9.0 (`cd57b23fa4bbad65`).
It does not change any copyrighted game file or bypass motorcycle unlock rules.

## Enable in the Savage Wind data directory

Create `savagewind-cheats.ini` beside `savagewind.ini` (Windows: next to the
.exe when using the portable layout):

```ini
[cheats]
motorcycle_all_regions = 1
stamps_9999 = 0
```

The default with no file is OFF. Restart the game and regenerate its game
library with this modified generator. If the previous `swgame.dll` was built
from the original source, it will have a different build ID and must be
recompiled from your own game files.

To disable, set `motorcycle_all_regions = 0` or remove the file. It is
cached once at startup; the `.ini` is not polled each frame.

## Technical detail

Ghidra and Capstone analysis: `0x7000B370B0` checks 20 climate IDs;
`0x7000B3711C` (`mov w8, #0x9fdf`) and `0x7000B37120`
(`movk w8, #7, lsl #16`) form the bitmask `0x79fdf`.
The masked-off climates are GerudoDesertClimate (5), EldinClimateLv1 (13),
EldinClimateLv2 (14), and GerudoDesertClimateLv2 (19).
The hook replaces only the second guest instruction, setting register W8
as the complete 20-bit mask `0xfffff` when enabled or preserving `0x79fdf`
when disabled. Two exact instruction-word checks and the game Build ID
fail closed on a mismatch. Other checks in that function remain intact.

## Test limits

Unit tests prove the patch is scoped to the correct build, original bytes,
mask values, and that the optional config defaults to off. These do not prove
in-game behavior. Before production use, test in normal Hyrule, Gerudo Desert,
Eldin, inaccessible terrain, and with the motorcycle still locked. Back up
saves, compare game logs and check for regressions.

The 9999 map-stamp cheat is **not** integrated. The current C helpers only
perform defensive checks and require further validation of save layout.
