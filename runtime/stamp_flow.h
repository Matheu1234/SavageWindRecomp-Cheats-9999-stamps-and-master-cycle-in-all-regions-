/* Phase-labelled, opt-in profiler for locating BOTW map-stamp operations.
 * Runtime-only: uses the FN_ENTER instrumentation already present in swgame.dll.
 * Does not read or modify guest storage, game limits, or saves. */
#ifndef SW_STAMP_FLOW_H
#define SW_STAMP_FLOW_H
#include <stdint.h>

/* The fast path in fnprof_hit checks this before the ordinary profiler. */
extern volatile int rt_sw_stamp_flow_active;
void rt_sw_stamp_flow_init(void);
void rt_sw_stamp_flow_frame(uint64_t presented_frame);
void rt_sw_stamp_flow_hit(uint64_t pc, uint64_t guest_lr, uint64_t x0);
void rt_sw_stamp_flow_shutdown(void);
#endif
