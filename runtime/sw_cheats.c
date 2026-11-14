#include "sw_cheats.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define SW_STAMPS_CHEAT_LIMIT 9999u

static size_t smaller(size_t a, size_t b) { return a < b ? a : b; }

bool sw_cheat_may_place_stamp(const SW_CheatConfig *cfg, const SW_StampState *s) {
    if (!cfg || !s) return false;
    size_t limit = s->vanilla_stamp_limit;
    if (cfg->stamps_9999 && s->capacity_and_save_layout_verified) {
        limit = SW_STAMPS_CHEAT_LIMIT;
    }
    limit = smaller(limit, s->stamp_capacity_in_memory);
    limit = smaller(limit, s->stamp_capacity_in_save);
    /* Do not add counts, to avoid overflow for untrusted save metadata. */
    if (s->occupied_pins > s->combined_map_marker_capacity) return false;
    size_t room_after_pins = s->combined_map_marker_capacity - s->occupied_pins;
    limit = smaller(limit, room_after_pins);
    return s->occupied_stamps < limit;
}

bool sw_cheat_motorcycle_allowed(const SW_CheatConfig *cfg, bool unlocked,
                                  bool normally_allowed_region,
                                  bool other_conditions_pass) {
    if (!cfg || !unlocked || !other_conditions_pass) return false;
    return normally_allowed_region || cfg->motorcycle_all_regions;
}

static void set_err(char *buf, size_t cap, const char *fmt, ...) {
    if (!buf || !cap) return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
}
static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
    return s;
}
static void lower_ascii(char *s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}
static int parse_bool(const char *s) {
    if (!strcmp(s, "1") || !strcmp(s, "true") || !strcmp(s, "on")) return 1;
    if (!strcmp(s, "0") || !strcmp(s, "false") || !strcmp(s, "off")) return 0;
    return -1;
}

bool sw_cheat_load_config(const char *path, SW_CheatConfig *out,
                          char *error, size_t error_size) {
    if (!out || !path) {
        set_err(error,error_size,"invalid arguments"); return false;
    }
    *out = (SW_CheatConfig){0};
    if (error && error_size) error[0] = 0;
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) return true; /* safely disabled by default */
        set_err(error,error_size,"cannot open %s", path);
        return false;
    }
    char line[512];
    size_t ln = 0;
    bool in_cheats = false, seen_stamps = false, seen_motorcycle = false, good = true;
    while (fgets(line, sizeof line, f)) {
        ++ln;
        if (!strchr(line,'\n') && !feof(f)) {
            set_err(error,error_size,"line %zu too long",ln); good=false; break;
        }
        char *p=trim(line);
        char *comment=strpbrk(p,"#;");
        if (comment) *comment=0;
        p=trim(p);
        if (!*p) continue;
        if (*p=='[') {
            size_t len=strlen(p);
            if (len<2 || p[len-1]!=']') {
                set_err(error,error_size,"bad section at line %zu",ln);good=false;break;
            }
            p[len-1]=0;
            char *section=trim(p+1); lower_ascii(section);
            in_cheats = !strcmp(section,"cheats");
            continue;
        }
        if (!in_cheats) continue;
        char *eq=strchr(p,'=');
        if (!eq) { set_err(error,error_size,"bad key at line %zu",ln);good=false;break; }
        *eq=0;
        char *key=trim(p),*val=trim(eq+1);
        lower_ascii(key);lower_ascii(val);
        int parsed=parse_bool(val);
        bool *destination=NULL,*seen=NULL;
        if (!strcmp(key,"stamps_9999")) {destination=&out->stamps_9999;seen=&seen_stamps;}
        else if (!strcmp(key,"motorcycle_all_regions")) {destination=&out->motorcycle_all_regions;seen=&seen_motorcycle;}
        else { set_err(error,error_size,"unknown key '%s' at line %zu",key,ln);good=false;break; }
        if (*seen || parsed<0) {
            set_err(error,error_size,"duplicate or invalid key '%s' at line %zu",key,ln);
            good=false;break;
        }
        *seen=true;*destination=(bool)parsed;
    }
    if (ferror(f)) {set_err(error,error_size,"read failure");good=false;}
    fclose(f);
    if (!good) *out=(SW_CheatConfig){0};
    return good;
}


/* Only explicitly implemented handlers for this exact 1.9.0 build are allowed.
 * Untrusted manifests never supply code or arbitrary patch addresses. */
static bool supported_hook(const char *id, const char *handler, const char *build, const char *status) {
    if (strcmp(build, "cd57b23fa4bbad65") || strcmp(status, "experimental")) return false;
    if (!strcmp(id, "motorcycle_all_regions"))
        return !strcmp(handler, "motorcycle_climate_mask");
    if (!strcmp(id, "stamps_9999"))
        return !strcmp(handler, "stamp_capacity_9999");
    return false;
}
static bool safe_id(const char *id) {
    if (!*id) return false;
    for (const char *p=id; *p; ++p)
        if (!( (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p=='_' )) return false;
    return true;
}
static bool safe_name(const char *name) {
    if (!*name || !strcmp(name, ".") || !strcmp(name, "..")) return false;
    return !strstr(name, "..") && !strchr(name, '/') && !strchr(name, '\\');
}
static bool read_manifest(const char *folder, SW_CheatItem *out) {
    if (!safe_name(folder)) return false;
    char path[512];
    if (snprintf(path,sizeof path,"cheats/%s/cheat.ini",folder) >= (int)sizeof path) return false;
    FILE *f=fopen(path,"rb"); if (!f) return false;
    char id[64]="", title[128]="", description[256]="", handler[80]="", build[80]="", status[80]="";
    char buf[512]; bool valid=true;
    while (fgets(buf,sizeof buf,f)) {
        if (!strchr(buf,'\n') && !feof(f)) { valid=false; break; }
        char *line=trim(buf), *comment=strpbrk(line,"#;");
        if (comment) *comment=0;
        line=trim(line);
        if (!*line || *line=='[') continue;
        char *eq=strchr(line,'='); if (!eq) {valid=false;break;}
        *eq=0; char *key=trim(line), *val=trim(eq+1);
        char *dest=NULL; size_t cap=0;
        if (!strcmp(key,"id")) dest=id,cap=sizeof id;
        else if (!strcmp(key,"name")) dest=title,cap=sizeof title;
        else if (!strcmp(key,"description")) dest=description,cap=sizeof description;
        else if (!strcmp(key,"handler")) dest=handler,cap=sizeof handler;
        else if (!strcmp(key,"build_id")) dest=build,cap=sizeof build;
        else if (!strcmp(key,"status")) dest=status,cap=sizeof status;
        else { valid=false; break; }
        if (strlen(val)>=cap) {valid=false;break;}
        strcpy(dest,val);
    }
    if (ferror(f)) valid=false;
    fclose(f);
    if (!valid || !safe_id(id) || !*title || !*handler || !*build || !*status) return false;
    memset(out,0,sizeof *out);
    snprintf(out->id,sizeof out->id,"%s",id);
    snprintf(out->title,sizeof out->title,"%s",title);
    snprintf(out->folder,sizeof out->folder,"%s",folder);
    snprintf(out->description,sizeof out->description,"%s",description);
    out->compatible=supported_hook(id,handler,build,status);
    const char *note = out->compatible ? "Experimental: restart required" :
                       (!strcmp(status,"planned") ? "Not implemented: cannot enable" :
                       "Unsupported patch/handler or game version");
    if (out->compatible && !strcmp(id,"stamps_9999"))
        note = "EXPERIMENTAL. Save stores 200 markers total; BACK UP SAVES. Rebuild game DLL first.";
    snprintf(out->note,sizeof out->note,"%s",note);
    return true;
}
size_t sw_cheat_list(SW_CheatItem *items, size_t capacity) {
    if (!items || !capacity) return 0;
    DIR *d=opendir("cheats"); if (!d) return 0;
    SW_CheatConfig cfg={0}; char error[256];
    if (!sw_cheat_load_config("savagewind-cheats.ini",&cfg,error,sizeof error)) cfg=(SW_CheatConfig){0};
    struct dirent *it; size_t n=0;
    while ((it=readdir(d)) && n<capacity && n<SW_CHEAT_MAX) {
        if (!safe_name(it->d_name)) continue;
        char dir[512];
        if (snprintf(dir,sizeof dir,"cheats/%s",it->d_name) >= (int)sizeof dir) continue;
        struct stat st;
        if (stat(dir,&st)!=0 || !S_ISDIR(st.st_mode)) continue;
        SW_CheatItem entry;
        if (!read_manifest(it->d_name,&entry)) continue;
        bool duplicate=false;
        for (size_t j=0;j<n;j++) if (!strcmp(items[j].id,entry.id)) duplicate=true;
        if (duplicate) continue;
        entry.enabled=entry.compatible &&
            ((!strcmp(entry.id,"motorcycle_all_regions") && cfg.motorcycle_all_regions) ||
             (!strcmp(entry.id,"stamps_9999") && cfg.stamps_9999));
        items[n++]=entry;
    }
    closedir(d);
    return n;
}
static bool supported_and_present(const char *id) {
    SW_CheatItem entries[SW_CHEAT_MAX];
    size_t n=sw_cheat_list(entries,SW_CHEAT_MAX);
    for (size_t i=0;i<n;i++) if (!strcmp(entries[i].id,id)) return entries[i].compatible;
    return false;
}
bool sw_cheat_set_enabled(const char *id, bool enabled, char *error, size_t error_size) {
    if (error && error_size) error[0]=0;
    if (!id || !supported_and_present(id)) {
        set_err(error,error_size,"cheat is not installed or lacks an audited handler"); return false;
    }
    SW_CheatConfig cfg={0};
    if (!sw_cheat_load_config("savagewind-cheats.ini",&cfg,error,error_size)) return false;
    if (!strcmp(id,"motorcycle_all_regions")) cfg.motorcycle_all_regions=enabled;
    else if (!strcmp(id,"stamps_9999")) cfg.stamps_9999=enabled;
    else {set_err(error,error_size,"unknown cheat id");return false;}
    const char *tmp="savagewind-cheats.ini.tmp", *target="savagewind-cheats.ini";
    FILE *f=fopen(tmp,"wb");
    if (!f) {set_err(error,error_size,"cannot write cheat settings");return false;}
    bool good=fprintf(f,"[cheats]\nmotorcycle_all_regions = %d\nstamps_9999 = %d\n",
                      cfg.motorcycle_all_regions ? 1:0, cfg.stamps_9999 ? 1:0)>0;
    if (fclose(f)) good=false;
    if (!good) {remove(tmp);set_err(error,error_size,"could not save cheat settings");return false;}
#ifdef _WIN32
    if (!MoveFileExA(tmp,target,MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
#else
    if (rename(tmp,target)) {
#endif
        remove(tmp);set_err(error,error_size,"could not replace cheat settings");return false;
    }
    return true;
}

/* Experimental stamp-site diagnostics: opt-in, no game-state modifications.
 * 203 CMP sites (immediates 99/100/199), 6 MOV W0,#100, 4 UI refs.
 * Multiple threads may reach these; counters are atomic and each site has
 * bounded log volume to avoid flooding a normal user session. */
int rt_sw_stamp_trace_enabled = 0;
#define SW_STAMP_TRACE_SITE_CAP 256u
static unsigned stamp_trace_visits[SW_STAMP_TRACE_SITE_CAP];
static unsigned stamp_trace_ui_visits[4];
void rt_sw_stamp_trace_record(unsigned index, uint64_t pc,
                              unsigned kind, unsigned value, unsigned immediate) {
    if (!rt_sw_stamp_trace_enabled || index >= SW_STAMP_TRACE_SITE_CAP) return;
    unsigned visits = __atomic_fetch_add(&stamp_trace_visits[index], 1u, __ATOMIC_RELAXED);
    if (visits < 3u || (visits == 9u) || (visits == 99u)) {
        fprintf(stderr, "[stamp-trace] type=%s pc=%#llx value=%u imm=%u hits=%u\n",
                kind == 1u ? "cmp" : "mov", (unsigned long long)pc,
                value, immediate, visits + 1u);
    }
}
void rt_sw_stamp_trace_ui(uint64_t pc, const char *event) {
    if (!rt_sw_stamp_trace_enabled) return;
    unsigned index = 0;
    switch (pc) {
        case 0x7000dd8208ull: index = 0; break;
        case 0x7000dd86b4ull: index = 1; break;
        case 0x7000df6174ull: index = 2; break;
        case 0x7000df618cull: index = 3; break;
        default: return;
    }
    unsigned hits = __atomic_fetch_add(&stamp_trace_ui_visits[index],1u,__ATOMIC_RELAXED);
    if (hits < 30u)
        fprintf(stderr, "[stamp-trace] type=ui pc=%#llx event=%s hits=%u\n",
                (unsigned long long)pc, event ? event : "unknown", hits + 1u);
}

/* Called before game threads start, once per boot; cached for frequent guest calls. */
static SW_CheatConfig g_sw_runtime_cheats;

void rt_sw_cheat_init(void) {
    /* This is a diagnostic-only environment switch, not a cheat toggle.
     * The trace remains off unless explicitly enabled for a test session. */
    const char *trace = getenv("SAVAGEWIND_STAMP_TRACE");
    rt_sw_stamp_trace_enabled = trace && !strcmp(trace, "1");
    if (rt_sw_stamp_trace_enabled)
        fprintf(stderr, "[stamp-trace] enabled (read-only; no game limits patched)\n");
    SW_CheatConfig cfg = {0};
    char error[256];
    if (!sw_cheat_load_config("savagewind-cheats.ini", &cfg, error, sizeof error)) {
        fprintf(stderr, "[cheats] invalid configuration, all cheats disabled: %s\n", error);
        g_sw_runtime_cheats = (SW_CheatConfig){0};
        return;
    }
    if (!supported_and_present("motorcycle_all_regions")) cfg.motorcycle_all_regions = false;
    if (!supported_and_present("stamps_9999")) cfg.stamps_9999 = false;
    g_sw_runtime_cheats = cfg;
    fprintf(stderr, "[cheats] Motorcycle in all regions: %s\n",
            cfg.motorcycle_all_regions ? "experimental ON" : "OFF");
    fprintf(stderr, "[cheats] 9999 Map Stamps Limit: %s (guest init @ 0x7000d844e0; SAVE LIMIT NOT EXPANDED)\n",
            cfg.stamps_9999 ? "EXPERIMENTAL ON" : "OFF");
    if (cfg.stamps_9999)
        fprintf(stderr, "[cheats] WARNING: marker save serialization remains 200 entries; back up saves, avoid 200+ markers.\n");
}

int rt_sw_motorcycle_all_regions(void) {
    return g_sw_runtime_cheats.motorcycle_all_regions ? 1 : 0;
}

/* Called from the generated swgame.dll instruction replacement.  The bool is
 * cached at boot: no filesystem I/O or mid-frame configuration changes. */
int rt_sw_stamps_9999_enabled(void) {
    return g_sw_runtime_cheats.stamps_9999 ? 1 : 0;
}
