#include "../runtime/sw_cheats.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(void) {
    SW_CheatConfig off = {0}, on = {true,true};
    SW_StampState s = {
        .occupied_stamps=99, .occupied_pins=5,
        .stamp_capacity_in_memory=100, .stamp_capacity_in_save=100,
        .combined_map_marker_capacity=200, .vanilla_stamp_limit=100,
        .capacity_and_save_layout_verified=false
    };
    assert(sw_cheat_may_place_stamp(&off,&s));
    s.occupied_stamps=100;
    assert(!sw_cheat_may_place_stamp(&off,&s));
    assert(!sw_cheat_may_place_stamp(&on,&s)); /* '9999' alone is UNSAFE */
    s.capacity_and_save_layout_verified=true;
    s.stamp_capacity_in_memory=9999; s.stamp_capacity_in_save=9999;
    s.combined_map_marker_capacity=11000;
    s.occupied_stamps=9998;
    assert(sw_cheat_may_place_stamp(&on,&s));
    s.occupied_stamps=9999;
    assert(!sw_cheat_may_place_stamp(&on,&s));
    s.occupied_stamps=110; s.combined_map_marker_capacity=115;
    assert(!sw_cheat_may_place_stamp(&on,&s)); /* pin + stamp sum */
    s.occupied_pins=(size_t)-1;
    assert(!sw_cheat_may_place_stamp(&on,&s)); /* integer overflow resistant */
    assert(!sw_cheat_may_place_stamp(NULL,&s));
    assert(!sw_cheat_may_place_stamp(&off,NULL));
    assert(!sw_cheat_motorcycle_allowed(&off,true,false,true));
    assert(sw_cheat_motorcycle_allowed(&on,true,false,true));
    assert(!sw_cheat_motorcycle_allowed(&on,false,false,true)); /* no DLC unlock bypass */
    assert(!sw_cheat_motorcycle_allowed(&on,true,false,false)); /* other restrictions */
    assert(sw_cheat_motorcycle_allowed(&off,true,true,true));
    char err[256]; SW_CheatConfig cfg;
    assert(sw_cheat_load_config("tests/__file_absent__.ini",&cfg,err,sizeof err));
    assert(!cfg.stamps_9999 && !cfg.motorcycle_all_regions);
    const char *tmp="tests/.test_cheats.ini";
    FILE *f=fopen(tmp,"wb"); assert(f);
    fputs("[cheats]\nstamps_9999 = 1\nmotorcycle_all_regions = on\n",f);
    fclose(f);
    assert(sw_cheat_load_config(tmp,&cfg,err,sizeof err));
    assert(cfg.stamps_9999 && cfg.motorcycle_all_regions);
    f=fopen(tmp,"wb");assert(f);
    fputs("[cheats]\nstamps_9999 = 1\nstamps_9999 = 0\n",f);
    fclose(f);
    assert(!sw_cheat_load_config(tmp,&cfg,err,sizeof err));
    assert(!cfg.stamps_9999);
    f=fopen(tmp,"wb");assert(f);
    fputs("[cheats]\nstamps_9999 = maybe\n",f);
    fclose(f);
    assert(!sw_cheat_load_config(tmp,&cfg,err,sizeof err));
    remove(tmp);
    puts("OK: stamp limits, memory/save guards, region/DLC checks, INI parsing");
    return 0;
}
