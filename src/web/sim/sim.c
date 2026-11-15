/* SPDX-License-Identifier: GPL-3.0-only */
/* LiveGomezSynth in a browser: the firmware's own application shell (firmware/src/lgs_app.c) on the host harness's fake
 * SMK-37 (tests/host.c: screen, buttons, pads, LEDs, MIDI queue), compiled to WebAssembly by web/sim/build.sh (zig cc,
 * wasm32-freestanding). The pattern is that of SMKSYNTH's/Makkuro37's/winampFM's simulators (GPL-3.0): the page's
 * AudioWorklet calls sim_render(n) and the device's clock runs with the audio.
 *
 * app_main() never returns (for (;;) { platform_poll(); ... }), so it runs as a coroutine with Binaryen's Asyncify
 * (wasm-opt --asyncify): platform_poll() parks the loop and the next millisecond resumes it there. The ISR is
 * app_audio(256 frames) as on the device; sim_render serves the worklet's 128-frame quanta out of it. */
#define BOARD_SMK37 1
#ifndef APP_VER
#define APP_VER "sim"
#endif
#define OBV_HOST 1
#define OBV_WASM 1
#include <stdint.h>
#include <stddef.h>

#define EXPORT __attribute__((used, visibility("default")))

void *memset(void *d, int c, size_t n) { return __builtin_memset(d, c, n); }
void *memcpy(void *d, const void *s, size_t n) { return __builtin_memcpy(d, s, n); }
void *memmove(void *d, const void *s, size_t n) { return __builtin_memmove(d, s, n); }
int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p = a, *q = b;
    for (; n; n--, p++, q++)
        if (*p != *q) return *p - *q;
    return 0;
}
#define printf(...) ((void)0)
static char *getenv(const char *n) { (void)n; return 0; }

#include "../../firmware/src/lgs_app.c"

/* ---- Asyncify: the main loop as a coroutine */
__attribute__((import_module("asyncify"), import_name("start_unwind"))) void asyncify_start_unwind(void *p);
__attribute__((import_module("asyncify"), import_name("stop_unwind"))) void asyncify_stop_unwind(void);
__attribute__((import_module("asyncify"), import_name("start_rewind"))) void asyncify_start_rewind(void *p);
__attribute__((import_module("asyncify"), import_name("stop_rewind"))) void asyncify_stop_rewind(void);
static struct { void *pos, *end; } aco;
static uint8_t aco_stack[16384] __attribute__((aligned(8)));
static volatile uint32_t parked, booted;

__attribute__((noinline)) static void platform_poll(void)
{
    if (!parked) {
        parked = 1;
        asyncify_start_unwind(&aco);
    } else {
        parked = 0;
        asyncify_stop_rewind();
    }
}
static void sim_app(void) { app_main(); }
static void (*volatile sim_app_p)(void) = sim_app;   /* (a pointer: app_main never returns) */
__attribute__((noinline)) static void sim_pass(void)
{
    if (!booted) {
        booted = 1;
        aco.pos = aco_stack;
        aco.end = aco_stack + sizeof aco_stack;
    } else
        asyncify_start_rewind(&aco);
    sim_app_p();
    asyncify_stop_unwind();
}

/* ---- the clock: audio in the ISR's 256-frame blocks, the main loop every millisecond */
#define SIM_MAXN 1024u
static float out_l[SIM_MAXN], out_r[SIM_MAXN];
static int32_t blk[2u * HALF_FRAMES];
static uint32_t blk_pos, ms_acc;
static int32_t master_q12 = 4096;               /* K8 MASTER (platform.c: (k10 * k10) >> 8) */
static int32_t peak;

EXPORT int sim_init(uint32_t rate)
{
    if (rate != FS || booted) return booted ? 0 : -1;
    fm1_in.wheel[0] = 512;                      /* the wheels rest: pitch centre, mod at its upper raw end */
    fm1_in.wheel[1] = 620;
    sim_pass();
    master_q12 = (smk_master * smk_master) >> 8;
    return 0;
}
EXPORT uint32_t sim_rate(void) { return FS; }
EXPORT float *sim_out_l(void) { return out_l; }
EXPORT float *sim_out_r(void) { return out_r; }
EXPORT void sim_render(uint32_t n)
{
    uint32_t i, j;
    if (!booted) return;
    for (i = 0; i < n && i < SIM_MAXN; i++) {
        if (blk_pos >= HALF_FRAMES) {
            master_q12 = (smk_master * smk_master) >> 8;
            memset(blk, 0, sizeof blk);
            app_audio(blk, HALF_FRAMES);
            for (j = 0; j < 2u * HALF_FRAMES; j++) {
                int32_t v = (blk[j] * master_q12) >> 12;
                blk[j] = v > 32767 ? 32767 : v < -32767 ? -32767 : v;
                if (blk[j] > peak) peak = blk[j];
                else if (-blk[j] > peak) peak = -blk[j];
            }
            blk_pos = 0;
        }
        out_l[i] = (float)blk[2u * blk_pos] * (1.0f / 32768.0f);
        out_r[i] = (float)blk[2u * blk_pos + 1u] * (1.0f / 32768.0f);
        blk_pos++;
        if ((ms_acc += 1000u) >= FS) {
            ms_acc -= FS;
            fm1_ms++;
            fm1_in.frames++;
            sim_pass();
        }
    }
}
EXPORT uint32_t sim_ms(void) { return fm1_ms; }
EXPORT int32_t sim_peak(void) { int32_t p = peak; peak = 0; return p; }
EXPORT void sim_master(uint32_t k10)            /* K8 MASTER, 0..1023 (the device's curve) */
{
    smk_master = k10 > 1023u ? 1023 : (int32_t)k10;
    master_q12 = (int32_t)(((uint32_t)smk_master * (uint32_t)smk_master) >> 8);
}
EXPORT int32_t sim_master_current(void) { return smk_master; }

/* ---- the panel */
static uint32_t key_velocity = 100u;
EXPORT void sim_key(uint32_t k, uint32_t down)  /* keybed key 0..36; keys 5..31 are the firmware's 27 note keys */
{
    if (k >= 37u) return;
    fm1_in.key_vel[k] = (uint8_t)key_velocity;
    if (down) fm1_in.keys |= (uint64_t)1 << k; else fm1_in.keys &= ~((uint64_t)1 << k);
    if (k >= 5u && k <= 31u) { if (down) fm1_in.notes |= 1u << (k - 5u); else fm1_in.notes &= ~(1u << (k - 5u)); }
    else host_qput(1, (int32_t)(k << 8 | (down ? key_velocity : 0u)));
}
EXPORT void sim_velocity(uint32_t v) { key_velocity = v < 1u ? 1u : v > 127u ? 127u : v; }
EXPORT void sim_pad(uint32_t p, uint32_t velocity) { if (p < 16u) { fm1_in.pad_st[p] = velocity ? 128 : 0; host_qput(0, (int32_t)(p << 8 | (velocity & 127u))); } }
EXPORT void sim_fader(uint32_t f, uint32_t v) { if (f < 4u) fm1_in.fader[f] = (uint8_t)(v & 127u); }
EXPORT void sim_pedal(uint32_t v) { fm1_in.pedal = (uint16_t)(v > 1023u ? 1023u : v); }
EXPORT void sim_wheel(uint32_t w, uint32_t v) { if (w < 2u) fm1_in.wheel[w] = (uint16_t)v; }
EXPORT uint32_t sim_pad_rgb(uint32_t p) { return p < 16u ? padled_rgb[p] : 0u; }
EXPORT void sim_button(uint32_t b, uint32_t down)   /* BTN_* (platform.c): 0 KNOB BANK 1 FADER BANK 2 LEFT 3 RIGHT 4 OCT- 5 OCT+ 6 ARP 7 LFO 8 SEL 9 ENV
                                                       10 GLO 11 SAVE 12 HOME 13 EDIT 14 FX 15 SEQ 16 PLAY 17 REC */
{
    if (b >= 18u) return;
    if (down) {
        fm1_in.buttons |= 1u << b;
        host_edges |= 1u << b;
    } else {
        fm1_in.buttons &= ~(1u << b);
        if (host_edges >> b & 1u) host_rel_next |= 1u << b;
        else host_rel |= 1u << b;
    }
}
EXPORT void sim_enc(uint32_t e, int32_t steps)  /* K1..K8 = 0..7, + = clockwise; K8 is the master pot */
{
    if (e == LGS_K8) { smk_master = clampi(smk_master + steps * 24, 0, 1023); master_q12 = (smk_master * smk_master) >> 8; }
    else if (e < (uint32_t)HOST_NENC) { host_enc[e] += steps; host_enc_pos[e] += steps; }
}
EXPORT void sim_midi(uint32_t st, uint32_t d1, uint32_t d2)
{
    if (st < 0x80u || st == 0xF0u || st == 0xF7u || st == 0xFEu || mq_w - mq_r >= 64u) return;
    mq[mq_w % 64u][0] = (uint8_t)st; mq[mq_w % 64u][1] = (uint8_t)(d1 & 0x7Fu); mq[mq_w % 64u][2] = (uint8_t)(d2 & 0x7Fu);
    mq_w++;
}
EXPORT void sim_panic(void)
{
    for (uint32_t k = 0; k < 37u; k++) sim_key(k, 0);
    for (uint32_t p = 0; p < 16u; p++) sim_pad(p, 0);
    for (uint32_t b = 0; b < 18u; b++) { fm1_in.buttons &= ~(1u << b); host_rel |= 1u << b; }
    for (uint32_t ch = 0; ch < 16u; ch++) sim_midi(0xb0u | ch, 123u, 0);
}

/* ---- what the panel shows */
EXPORT uint16_t *sim_fb(void) { return fb; }
EXPORT uint32_t sim_fb_hash(void)
{
    uint32_t h = 2166136261u, i;
    const uint32_t *p = (const uint32_t *)fb;
    for (i = 0; i < 240u * 240u / 2u; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}
EXPORT uint32_t sim_leds_buttons(void) { return (uint32_t)host_leds; }

/* ---- a few state probes for the developer panel and tests */
EXPORT uint32_t sim_prog(void) { return pat.patch; }
/* UI state for tests and the developer panel (lgs_ui.h) */
EXPORT int32_t sim_ui(uint32_t i)
{
    switch (i) {
    case 0: return mode;
    case 1: return ov;
    case 2: return solo;
    case 3: return kb_bank;
    case 4: return fb_bank;
    case 5: return cur_slot;
    case 6: return active_line;
    case 7: return edit_line;
    case 8: return pat.patch;
    case 9: return pat_cat;
    case 10: return pat_idx;
    case 11: return ps_slot;
    case 12: return edit_page;
    case 13: return oct_shift;
    case 14: return (int32_t)ctl_tempo;
    case 15: return drum_pads;
    case 16: return ui_trk_peak[0];
    default: return -1;
    }
}
EXPORT int32_t sim_part(uint32_t p, uint32_t i)      /* i: 0 lvl 1 cho 2 dly 3 rev 4 drv 5 voices gating 6 cutoff */
{
    uint32_t v, n = 0;
    if (p >= NPART) return -1;
    switch (i) {
    case 0: return ctl_mix_lvl[p];
    case 1: return ctl_mix_cho[p];
    case 2: return ctl_mix_dly[p];
    case 3: return ctl_mix_rev[p];
    case 4: return ctl_mix_drv[p];
    case 5: for (v = 0; v < NV; v++) if (vc[v].gate && vc[v].part == p) n++; return (int32_t)n;
    case 6: return live_p[p].v[P_CUTOFF];
    case 7: return ctl_hold >> p & 1u;
    case 8: return ctl_bend_off >> p & 1u;
    case 9: return ctl_mod_cut[p];
    case 13: { uint32_t k, h = 0; for (k = 0; k < NPARAM; k++) h = h * 31u + live_p[p].v[k]; return (int32_t)(h & 0x7FFFFFFF); }   /* a fingerprint of the sound in the part */
    case 10: return ctl_vib_off >> p & 1u;
    case 11: return obv_engine_track[p].active;
    case 12: return drums_busy(p);
    default: return -1;
    }
}
EXPORT int32_t sim_line(uint32_t l, uint32_t i)      /* the working performance: 0 patch 1 lo 2 hi 3 vol 4 flags 5 origin */
{
    const lgline_t *x;
    if (l >= LGS_NLINE) return -1;
    x = &cur.line[l];
    switch (i) { case 0: return x->s.patch; case 1: return x->lo; case 2: return x->hi; case 3: return x->vol; case 4: return x->flags; case 5: return x->origin; default: return -1; }
}
EXPORT uint32_t sim_vinc(uint32_t i) { return i < NV ? vc[i].inc1 : 0u; }
EXPORT uint32_t sim_vnote(uint32_t i) { return i < NV ? (vc[i].gate ? vc[i].note : 0u) : 0u; }
EXPORT uint32_t sim_patch_n(void) { return LGS_NPATCH; }
EXPORT uint32_t sim_patch_id(uint32_t i) { return i < LGS_NPATCH ? LGS_PATCH[i].id : 0u; }
EXPORT uint32_t sim_patch_engine(uint32_t i) { return i < LGS_NPATCH ? LGS_PATCH[i].engine : 0u; }
EXPORT void sim_load_patch(uint32_t id)         /* test hook: the PATCH mode sound becomes this catalog patch */
{
    sound_t s;
    memset(&s, 0, sizeof s);
    sound_defaults(&s, id);
    if (mode == M_PATCH) pat_load(&s);
}
EXPORT int32_t sim_fav(uint32_t i) { return i < LGS_NFAV ? db.fav[i].patch : -1; }
EXPORT int32_t sim_perf(uint32_t i, uint32_t l) { return i < LGS_NPERF && l < LGS_NLINE ? db.perf[i].line[l].s.patch : -1; }
EXPORT const char *sim_row(uint32_t r) { return r < ROWS ? row_t[r] : ""; }
EXPORT const char *sim_msg(void) { return msg; }
EXPORT void *sim_db_export(void) { db_seal(); return &db; }   /* the sealed image the page stores */
EXPORT uint32_t sim_db_size(void) { return sizeof db; }
EXPORT void *sim_stage(void) { return store_stage; }            /* the page writes a stored image here before the first frame */
EXPORT void sim_stage_set(uint32_t n) { store_stage_len = n <= sizeof store_stage ? n : 0u; }
EXPORT uint32_t sim_db_dirty(void) { uint32_t d = db_dirty; db_dirty = 0; return d; }
EXPORT uint32_t sim_param(uint32_t p) { return p < NPARAM ? live_p[0].v[p] : 0; }
EXPORT uint32_t sim_voices(void) { uint32_t n = 0; for (unsigned i = 0; i < NV; i++) n += vc[i].gate ? 1u : 0u; return n; }
