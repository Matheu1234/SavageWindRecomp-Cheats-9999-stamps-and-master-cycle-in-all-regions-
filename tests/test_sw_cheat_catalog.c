#include "../runtime/sw_cheats.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    SW_CheatItem entries[SW_CHEAT_MAX];
    char error[256];
    size_t n=sw_cheat_list(entries,SW_CHEAT_MAX);
    assert(n>=2);
    int motorcycle=0, stamps=0;
    for(size_t i=0;i<n;i++) {
        if(!strcmp(entries[i].id,"motorcycle_all_regions")) {
            motorcycle++;
            assert(entries[i].compatible);
        }
        if(!strcmp(entries[i].id,"stamps_9999")) {
            stamps++;
            assert(entries[i].compatible);
            assert(!entries[i].enabled);
        }
    }
    assert(motorcycle==1 && stamps==1);
    assert(!sw_cheat_set_enabled("nonsense",true,error,sizeof error));
    assert(sw_cheat_set_enabled("stamps_9999",true,error,sizeof error));
    assert(sw_cheat_set_enabled("motorcycle_all_regions",true,error,sizeof error));
    n=sw_cheat_list(entries,SW_CHEAT_MAX);
    bool found_stamp=false,found_motorcycle=false;
    for(size_t i=0;i<n;i++) {
        if(!strcmp(entries[i].id,"motorcycle_all_regions")) {
            assert(entries[i].enabled);found_motorcycle=true;
        }
        if(!strcmp(entries[i].id,"stamps_9999")) {
            assert(entries[i].enabled);found_stamp=true;
        }
    }
    assert(found_stamp && found_motorcycle);
    SW_CheatConfig cfg;
    assert(sw_cheat_load_config("savagewind-cheats.ini",&cfg,error,sizeof error));
    assert(cfg.motorcycle_all_regions && cfg.stamps_9999);
    assert(sw_cheat_set_enabled("motorcycle_all_regions",false,error,sizeof error));
    assert(sw_cheat_set_enabled("stamps_9999",false,error,sizeof error));
    assert(sw_cheat_load_config("savagewind-cheats.ini",&cfg,error,sizeof error));
    assert(!cfg.motorcycle_all_regions && !cfg.stamps_9999);
    remove("savagewind-cheats.ini");
    puts("OK: both compatible manifests, independent enable/disable, persistence");
    return 0;
}
