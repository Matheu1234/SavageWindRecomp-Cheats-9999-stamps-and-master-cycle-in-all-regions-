#ifndef SW_CHEATS_H
#define SW_CHEATS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Experimental cheats for the exact BOTW Switch 1.9.0 build.  9999 stamps
 * changes RAM allocation only; the on-disk 200-marker save format is unchanged. */

typedef struct {
    bool stamps_9999;
    bool motorcycle_all_regions;
} SW_CheatConfig;

typedef struct {
    size_t occupied_stamps;
    size_t occupied_pins;
    size_t stamp_capacity_in_memory;
    size_t stamp_capacity_in_save;
    size_t combined_map_marker_capacity;
    size_t vanilla_stamp_limit;
    bool capacity_and_save_layout_verified;
} SW_StampState;

/* Returns true only when there is a validated place for another stamp.
 * The extended limit is never enabled from a numeric patch alone. */
bool sw_cheat_may_place_stamp(const SW_CheatConfig *cfg,
                              const SW_StampState *state);

/* Region bypass must never bypass DLC/quest unlocks or other restrictions. */
bool sw_cheat_motorcycle_allowed(const SW_CheatConfig *cfg,
                                 bool motorcycle_unlocked,
                                 bool normally_allowed_region,
                                 bool other_conditions_pass);

/* Config file is optional; when absent both cheats default to false.
 * Expects a simple [cheats] INI with stamps_9999 and motorcycle_all_regions.
 * Returns false on malformed values, long lines, duplicates, or unreadable files. */
bool sw_cheat_load_config(const char *path, SW_CheatConfig *out,
                          char *error, size_t error_size);

/* Safe manifest registry. A folder is NOT executable code: only built-in handlers
 * that have been audited for the supported game build can be enabled. */
#define SW_CHEAT_MAX 32
typedef struct {
    char id[64];
    char title[128];
    char description[256];
    char folder[160];
    bool compatible;
    bool enabled;
    char note[160];
} SW_CheatItem;

size_t sw_cheat_list(SW_CheatItem *items, size_t capacity);
bool sw_cheat_set_enabled(const char *id, bool enabled, char *error, size_t error_size);

/* Called after rt_paths_init() sets the data root/current directory.
 * Guest hooks use cached settings only, with no file access during gameplay. */
/* Optional opt-in, read-only ARM64 instruction trace. With the environment
 * variable unset, gameplay stays unchanged. Diagnostic sites != cheat sites. */
extern int rt_sw_stamp_trace_enabled;
void rt_sw_stamp_trace_record(unsigned index, uint64_t pc,
                              unsigned kind, unsigned value, unsigned immediate);
void rt_sw_stamp_trace_ui(uint64_t pc, const char *event);
void rt_sw_cheat_init(void);
int rt_sw_motorcycle_all_regions(void);
int rt_sw_stamps_9999_enabled(void);
#endif
