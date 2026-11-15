/* SPDX-License-Identifier: GPL-3.0-only */
/* The step sequencer is removed (D-50). obv_engine.h still calls five hooks; this stub answers "no sequencer". */
static uint32_t seq_hook_beat;          /* the arpeggiator's own clock is in obv_engine.h; nothing to run here */
static int seq_phase(uint32_t *ph) { (void)ph; return 0; }
static int seq_starting(void) { return 0; }
static void seq_tick(void) {}
static uint32_t seq_q8_now(uint32_t t) { (void)t; return 0xFFFFFFFFu; }
static void seq_engine_reset(uint32_t part) { (void)part; }
