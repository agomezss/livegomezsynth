/* SPDX-License-Identifier: GPL-3.0-only
 * FM6: classic 6-operator FM. The synthesis is msfa (Dexed's core, Apache-2.0) as ported to integer C by Clipper
 * (lgs_fm6_core.h); this file is the glue, adapted from Clipper's eng_fm6.c (Copyright (C) 2026 Leo Kuroshita,
 * Hugelton Instruments, GPL-3.0). The patch (one of Clipper's eight authored voices) is the sound; the eight engine values are
 * macros on top of it: ALG (0 = the patch's), FB, MLVL (modulator level: the brightness), MRAT, MEG (modulator envelope
 * speed), VMOD, DTUN (carrier spread), PTCH (which voice). The operator envelopes are the voice's amplitude and end it
 * (done): the patch's ADSR here only has to stay out of the way. */
#include "lgs_fm6_data.h"
#include "lgs_fm6_dx.h"
#include "lgs_fm6_core.h"
#include "lgs_fm6_patch.h"
#define FM6_PACKED 128u
#define FM6_NSLOT (FM6_NFACTORY + FM6_NDX)           /* Clipper's eight voices, then the SDK's 128 (D-69) */

static uint8_t fm6_patch[NPART][FP_SIZE + 1u];  /* each part's voice */
static uint8_t fm6_slot[NPART] = {255, 255, 255, 255};
static uint8_t fm6_gen[NPART];
static struct {
    uint8_t p[FP_SIZE + 1u];
    uint8_t gen, alg, fb, ok;
    int16_t e[7];
    int32_t dt[6];
} fm6_eff[NPART];
static fm6_lfo_t fm6_lfo[NPART];
static int32_t fm6_lfo_v[NPART], fm6_lfo_d[NPART];
static fm6_note_t fm6_notes[NV];
static int32_t fm6_mbus[2][2][FM6_N];

static void fm6_load_slot(uint32_t part, uint32_t s)
{
    uint8_t v[FP_SIZE + 1u];
    s %= FM6_NSLOT;
    fm6_unpack(s < FM6_NFACTORY ? FM6_FACTORY[s] : FM6_DX[s - FM6_NFACTORY], v);
    memcpy(fm6_patch[part], v, FP_SIZE + 1u);
    fm6_slot[part] = (uint8_t)(s % FM6_NSLOT);
    fm6_gen[part]++;
}

static void fm6_sync(const track_t *t, uint32_t tr)     /* the patch through E0..E6, cheap when nothing changed */
{
    uint32_t k, j, car, n = 0;
    const int16_t *e = &t->p[P_E0];
    int32_t meg, step;
    if (fm6_slot[tr] != (uint8_t)clamp(t->p[P_E7], 0, FM6_NSLOT - 1)) fm6_load_slot(tr, (uint32_t)clamp(t->p[P_E7], 0, FM6_NSLOT - 1));
    if (fm6_eff[tr].ok && fm6_eff[tr].gen == fm6_gen[tr]) {
        for (k = 0; k < 7u && fm6_eff[tr].e[k] == e[k]; k++) ;
        if (k == 7u) return;
    }
    fm6_eff[tr].gen = fm6_gen[tr];
    memcpy(fm6_eff[tr].p, fm6_patch[tr], FP_SIZE);
    for (k = 0; k < 7u; k++) fm6_eff[tr].e[k] = e[k];
    {
        uint8_t *p = fm6_eff[tr].p;
        uint32_t alg = e[0] >= 1 && e[0] <= 32 ? (uint32_t)e[0] - 1u : p[FP_ALG] & 31u;
        fm6_eff[tr].alg = (uint8_t)alg;
        fm6_eff[tr].fb = (uint8_t)clamp(p[FP_FB] + e[1], 0, 7);
        car = fm6_carriers(alg);
        meg = clamp(e[4], -64, 63) * 40 / 64;
        step = clamp(e[6], 0, 127) * (12 * 13981) / 127;
        for (k = 0; k < 6u; k++) {
            uint8_t *op = p + (5u - k) * FP_OP;
            uint32_t ki = 5u - k;
            fm6_eff[tr].dt[ki] = 0;
            if ((car >> ki) & 1u) {
                static const int8_t SPREAD[6] = {0, 1, -1, 2, -2, 3};
                fm6_eff[tr].dt[ki] = SPREAD[n++] * step;
                continue;
            }
            if (!op[FP_MODE]) op[FP_FC] = (uint8_t)clamp(op[FP_FC] + e[3], 0, 31);
            for (j = 0; j < 4u; j++) op[FP_R1 + j] = (uint8_t)clamp(op[FP_R1 + j] - meg, 0, 99);
            op[FP_KVS] = (uint8_t)clamp(op[FP_KVS] + e[5], 0, 7);
        }
        fm6_lfo_reset(&fm6_lfo[tr], p);
    }
    fm6_eff[tr].ok = 1;
}

static void fm6_block(track_t *t)               /* once per window and part: the macros, the LFO */
{
    uint32_t tr = lgs_cur_part;
    fm6_sync(t, tr);
    fm6_lfo_v[tr] = fm6_lfo_sample(&fm6_lfo[tr]);
    fm6_lfo_d[tr] = fm6_lfo_delay(&fm6_lfo[tr]);
}

static void fm6_note_on(track_t *t, cvoice_t *v)
{
    uint32_t tr = lgs_cur_part;
    fm6_note_t *n = &fm6_notes[lgs_cur_vi];
    const uint8_t *p;
    fm6_sync(t, tr);
    p = fm6_eff[tr].p;
    fm6_note_init(n, p, (int32_t)v->note + p[FP_TRNSP] - 24, v->vel, !n->alive);
    fm6_lfo_key(&fm6_lfo[tr]);
}

static int fm6_done(track_t *t, cvoice_t *v)    /* every carrier has gone silent for good: the voice ends */
{
    fm6_note_t *n = &fm6_notes[lgs_cur_vi];
    uint32_t tr = lgs_cur_part;
    (void)t; (void)v;
    if (!n->alive) return 1;
    if (!fm6_note_done(n, fm6_eff[tr].p, fm6_eff[tr].alg)) return 0;
    n->alive = 0;
    return 1;
}

/* output shift per voice, measured against the other engines (tests/engines_test.mjs): the six-carrier DRAWBARS is much quieter */
static const uint8_t FM6_SHIFT[FM6_NFACTORY] = {26, 26, 26, 26, 26, 26, 26, 26};   /* the SDK's voices: 26, matched per patch by the catalog trim */

static void fm6_render(track_t *t, cvoice_t *v, int32_t *out, uint32_t len, const vmod_t *m)
{
    fm6_note_t *n = &fm6_notes[lgs_cur_vi];
    uint32_t tr = lgs_cur_part, i;
    const uint8_t *p;
    int32_t bus[FM6_N], lf, fb, lvl, a, da;
    uint32_t sh = fm6_slot[lgs_cur_part] < FM6_NFACTORY ? FM6_SHIFT[fm6_slot[lgs_cur_part]] : 26u;
    if (!n->alive || len != FM6_N) return;
    p = fm6_eff[tr].p;
    if (!v->gate && n->down) fm6_note_key(n, p, 0);
    lf = fm6_note_logfreq(m->pitch16 + (p[FP_TRNSP] - 24) * 16, m->fine);
    fb = clamp(fm6_eff[tr].fb + ((m->shape - (64 << 8)) >> 11), 0, 7);
    lvl = clamp(t->p[P_E2], -64, 63) * 24 + clamp(m->cutoff >> 8, -150, 150) * 16;
    if (!fm6_note_compute(n, p, bus, fm6_mbus[0], fm6_lfo_v[tr], fm6_lfo_d[tr], lf, fm6_eff[tr].alg, fb, fm6_eff[tr].dt, lvl << 16))
        return;
    a = mulq15(m->amp0, VOICE_FS);
    da = (mulq15(m->amp1, VOICE_FS) - a) >> FM6_LG_N;
    for (i = 0; i < FM6_N; i++) {
        int32_t x = bus[i] - n->dc;
        n->dc += x >> 10;                       /* a one-pole high-pass, ~7 Hz: FM puts sidebands at 0 Hz */
        a += da;
        out[i] += (int32_t)(((int64_t)x * a) >> sh);
    }
}

/* the ADSR is not the amplitude here (the operators are): a gate-following envelope with a long release */
static const lgs_preset_t FM6_PRESETS[] = {
    {"TINE EP", {0, 0, 0, 0, 0, 0, 0, 0}, {0, 64, 127, 110}, 0, 0, FX(0, 45, 25, 35)},
    {"GLASS BELL", {0, 0, 0, 0, 0, 0, 0, 1}, {0, 64, 127, 110}, 0, 0, FX(0, 10, 30, 70)},
    {"ROUND BASS", {0, 0, 0, 0, 0, 0, 0, 2}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 10, 10)},
    {"BRASS SECT", {0, 0, 0, 0, 0, 0, 0, 3}, {0, 64, 127, 100}, 0, 0, FX(0, 25, 20, 40)},
    {"SOFT PAD", {0, 0, 0, 0, 0, 0, 30, 4}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 70)},
    {"WOOD BARS", {0, 0, 0, 0, 0, 0, 0, 5}, {0, 64, 127, 110}, 0, 0, FX(0, 0, 25, 40)},
    {"DRAWBARS", {0, 0, 0, 0, 0, 0, 0, 6}, {0, 64, 127, 90}, 0, 0, FX(10, 40, 0, 30)},
    {"NYLON PICK", {0, 0, 0, 0, 0, 0, 0, 7}, {0, 64, 127, 110}, 0, 0, FX(0, 20, 35, 30)},
    {"HORN STAB", {0, 0, 8, 0, -12, 4, 0, 3}, {0, 64, 127, 50}, 0, 0, FX(0, 14, 10, 30)},
    {"WARM HORNS", {0, 0, -8, 0, 6, 2, 18, 3}, {0, 64, 127, 100}, 0, 0, FX(0, 30, 14, 44)},
    /* FAMOUS BEGIN (tools/gen_famous.py) */
    /* ICONIC (tools/famous_defs.py) */
    {"AFRICA MARIMBA", {0, 0, 0, 0, -20, 0, 0, 5}, {0, 64, 127, 110}, 0, 0, FX(0, 0, 28, 44)},
    {"SLOW EP", {0, 0, -10, 0, 10, 0, 10, 0}, {0, 64, 127, 110}, 0, 0, FX(0, 50, 24, 40)},
    {"TRIUMPH HORNS", {0, 0, 10, 0, -10, 0, 0, 3}, {0, 64, 127, 100}, 0, 0, FX(0, 18, 14, 40)},
    {"CRYSTAL CHIME", {0, 0, 8, 0, 10, 0, 0, 1}, {0, 64, 127, 110}, 0, 0, FX(0, 10, 30, 70)},
    {"NATURE EP", {0, 0, -6, 0, 5, 0, 10, 0}, {0, 64, 127, 110}, 0, 0, FX(0, 55, 22, 44)},
    {"SUPER EP", {0, 0, 6, 0, 0, 0, 0, 0}, {0, 64, 127, 100}, 0, 0, FX(20, 30, 10, 24)},
    {"FUNK HORNS", {0, 0, 15, 0, -5, 0, 0, 3}, {0, 64, 127, 60}, 0, 0, FX(0, 10, 10, 24)},
    {"ROCK FM BASS", {0, 0, 0, 0, 0, 0, 0, 2}, {0, 64, 127, 70}, 0, 0, FX(20, 0, 0, 6)},
    {"CARRIE EP", {0, 0, -14, 0, 4, 0, 12, 0}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 20, 56)},
    {"Piano 1", {0, 0, 0, 0, 0, 0, 0, 8}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 1", {0, 0, 0, 0, 0, 0, 0, 9}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 1", {0, 0, 0, 0, 0, 0, 0, 10}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 1", {0, 0, 0, 0, 0, 0, 0, 11}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 2", {0, 0, 0, 0, 0, 0, 0, 12}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 2", {0, 0, 0, 0, 0, 0, 0, 13}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 2", {0, 0, 0, 0, 0, 0, 0, 14}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 2", {0, 0, 0, 0, 0, 0, 0, 15}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 3", {0, 0, 0, 0, 0, 0, 0, 16}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 3", {0, 0, 0, 0, 0, 0, 0, 17}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 3", {0, 0, 0, 0, 0, 0, 0, 18}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 3", {0, 0, 0, 0, 0, 0, 0, 19}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 4", {0, 0, 0, 0, 0, 0, 0, 20}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 4", {0, 0, 0, 0, 0, 0, 0, 21}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 4", {0, 0, 0, 0, 0, 0, 0, 22}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 4", {0, 0, 0, 0, 0, 0, 0, 23}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 5", {0, 0, 0, 0, 0, 0, 0, 24}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 5", {0, 0, 0, 0, 0, 0, 0, 25}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 5", {0, 0, 0, 0, 0, 0, 0, 26}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 5", {0, 0, 0, 0, 0, 0, 0, 27}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 6", {0, 0, 0, 0, 0, 0, 0, 28}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 6", {0, 0, 0, 0, 0, 0, 0, 29}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 6", {0, 0, 0, 0, 0, 0, 0, 30}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 6", {0, 0, 0, 0, 0, 0, 0, 31}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 7", {0, 0, 0, 0, 0, 0, 0, 32}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 7", {0, 0, 0, 0, 0, 0, 0, 33}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 7", {0, 0, 0, 0, 0, 0, 0, 34}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 7", {0, 0, 0, 0, 0, 0, 0, 35}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Piano 8", {0, 0, 0, 0, 0, 0, 0, 36}, {0, 64, 127, 100}, 0, 0, FX(0, 35, 20, 30)},
    {"Organ 8", {0, 0, 0, 0, 0, 0, 0, 37}, {0, 64, 127, 70}, 0, 0, FX(10, 40, 0, 25)},
    {"Lead 8", {0, 0, 0, 0, 0, 0, 0, 38}, {0, 64, 127, 90}, 0, 1, FX(0, 10, 25, 25)},
    {"Pad 8", {0, 0, 0, 0, 0, 0, 0, 39}, {0, 64, 127, 110}, 0, 0, FX(0, 60, 30, 65)},
    {"Guitar 1", {0, 0, 0, 0, 0, 0, 0, 40}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 1", {0, 0, 0, 0, 0, 0, 0, 41}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 1", {0, 0, 0, 0, 0, 0, 0, 42}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 1", {0, 0, 0, 0, 0, 0, 0, 43}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 2", {0, 0, 0, 0, 0, 0, 0, 44}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 2", {0, 0, 0, 0, 0, 0, 0, 45}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 2", {0, 0, 0, 0, 0, 0, 0, 46}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 2", {0, 0, 0, 0, 0, 0, 0, 47}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 3", {0, 0, 0, 0, 0, 0, 0, 48}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 3", {0, 0, 0, 0, 0, 0, 0, 49}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 3", {0, 0, 0, 0, 0, 0, 0, 50}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 3", {0, 0, 0, 0, 0, 0, 0, 51}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 4", {0, 0, 0, 0, 0, 0, 0, 52}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 4", {0, 0, 0, 0, 0, 0, 0, 53}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 4", {0, 0, 0, 0, 0, 0, 0, 54}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 4", {0, 0, 0, 0, 0, 0, 0, 55}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 5", {0, 0, 0, 0, 0, 0, 0, 56}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 5", {0, 0, 0, 0, 0, 0, 0, 57}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 5", {0, 0, 0, 0, 0, 0, 0, 58}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 5", {0, 0, 0, 0, 0, 0, 0, 59}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 6", {0, 0, 0, 0, 0, 0, 0, 60}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 6", {0, 0, 0, 0, 0, 0, 0, 61}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 6", {0, 0, 0, 0, 0, 0, 0, 62}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 6", {0, 0, 0, 0, 0, 0, 0, 63}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 7", {0, 0, 0, 0, 0, 0, 0, 64}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 7", {0, 0, 0, 0, 0, 0, 0, 65}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 7", {0, 0, 0, 0, 0, 0, 0, 66}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 7", {0, 0, 0, 0, 0, 0, 0, 67}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Guitar 8", {0, 0, 0, 0, 0, 0, 0, 68}, {0, 64, 127, 100}, 0, 0, FX(0, 20, 25, 30)},
    {"DS Guitar 8", {0, 0, 0, 0, 0, 0, 0, 69}, {0, 64, 127, 90}, 0, 0, FX(30, 10, 15, 20)},
    {"Bass 8", {0, 0, 0, 0, 0, 0, 0, 70}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Synth Bass 8", {0, 0, 0, 0, 0, 0, 0, 71}, {0, 64, 127, 70}, 0, 1, FX(0, 0, 0, 8)},
    {"Brass 1", {0, 0, 0, 0, 0, 0, 0, 72}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 1", {0, 0, 0, 0, 0, 0, 0, 73}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 1", {0, 0, 0, 0, 0, 0, 0, 74}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 1", {0, 0, 0, 0, 0, 0, 0, 75}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 2", {0, 0, 0, 0, 0, 0, 0, 76}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 2", {0, 0, 0, 0, 0, 0, 0, 77}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 2", {0, 0, 0, 0, 0, 0, 0, 78}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 2", {0, 0, 0, 0, 0, 0, 0, 79}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 3", {0, 0, 0, 0, 0, 0, 0, 80}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 3", {0, 0, 0, 0, 0, 0, 0, 81}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 3", {0, 0, 0, 0, 0, 0, 0, 82}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 3", {0, 0, 0, 0, 0, 0, 0, 83}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 4", {0, 0, 0, 0, 0, 0, 0, 84}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 4", {0, 0, 0, 0, 0, 0, 0, 85}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 4", {0, 0, 0, 0, 0, 0, 0, 86}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 4", {0, 0, 0, 0, 0, 0, 0, 87}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 5", {0, 0, 0, 0, 0, 0, 0, 88}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 5", {0, 0, 0, 0, 0, 0, 0, 89}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 5", {0, 0, 0, 0, 0, 0, 0, 90}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 5", {0, 0, 0, 0, 0, 0, 0, 91}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 6", {0, 0, 0, 0, 0, 0, 0, 92}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 6", {0, 0, 0, 0, 0, 0, 0, 93}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 6", {0, 0, 0, 0, 0, 0, 0, 94}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 6", {0, 0, 0, 0, 0, 0, 0, 95}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 7", {0, 0, 0, 0, 0, 0, 0, 96}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 7", {0, 0, 0, 0, 0, 0, 0, 97}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 7", {0, 0, 0, 0, 0, 0, 0, 98}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 7", {0, 0, 0, 0, 0, 0, 0, 99}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Brass 8", {0, 0, 0, 0, 0, 0, 0, 100}, {0, 64, 127, 90}, 0, 0, FX(0, 25, 15, 35)},
    {"Wind 8", {0, 0, 0, 0, 0, 0, 0, 101}, {0, 64, 127, 90}, 0, 0, FX(0, 20, 20, 40)},
    {"Strings 8", {0, 0, 0, 0, 0, 0, 0, 102}, {0, 64, 127, 105}, 0, 0, FX(0, 45, 15, 50)},
    {"Voice 8", {0, 0, 0, 0, 0, 0, 0, 103}, {0, 64, 127, 105}, 0, 0, FX(0, 40, 20, 55)},
    {"Moan", {0, 0, 0, 0, 0, 0, 0, 104}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Odd Hit", {0, 0, 0, 0, 0, 0, 0, 105}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Block", {0, 0, 0, 0, 0, 0, 0, 106}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Waves", {0, 0, 0, 0, 0, 0, 0, 107}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Frog Bell", {0, 0, 0, 0, 0, 0, 0, 108}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Gong", {0, 0, 0, 0, 0, 0, 0, 109}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Strange 1", {0, 0, 0, 0, 0, 0, 0, 110}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Mono Fx 1", {0, 0, 0, 0, 0, 0, 0, 111}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Tibet Bowl", {0, 0, 0, 0, 0, 0, 0, 112}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Log Drum", {0, 0, 0, 0, 0, 0, 0, 113}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Cowbell", {0, 0, 0, 0, 0, 0, 0, 114}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Mono Fx 2", {0, 0, 0, 0, 0, 0, 0, 115}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Finger Cym", {0, 0, 0, 0, 0, 0, 0, 116}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Timpani", {0, 0, 0, 0, 0, 0, 0, 117}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Tambourine", {0, 0, 0, 0, 0, 0, 0, 118}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Descent", {0, 0, 0, 0, 0, 0, 0, 119}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Clang Bell", {0, 0, 0, 0, 0, 0, 0, 120}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Steel Drum", {0, 0, 0, 0, 0, 0, 0, 121}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Conga", {0, 0, 0, 0, 0, 0, 0, 122}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Saw Stack 1", {0, 0, 0, 0, 0, 0, 0, 123}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Bell Hit", {0, 0, 0, 0, 0, 0, 0, 124}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Kalimba", {0, 0, 0, 0, 0, 0, 0, 125}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Gamelan", {0, 0, 0, 0, 0, 0, 0, 126}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Saw Stack 2", {0, 0, 0, 0, 0, 0, 0, 127}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Tube Bells", {0, 0, 0, 0, 0, 0, 0, 128}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Brushes", {0, 0, 0, 0, 0, 0, 0, 129}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Toms", {0, 0, 0, 0, 0, 0, 0, 130}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Saw Stack 3", {0, 0, 0, 0, 0, 0, 0, 131}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Echo Pluck", {0, 0, 0, 0, 0, 0, 0, 132}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Timbales", {0, 0, 0, 0, 0, 0, 0, 133}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Percussion", {0, 0, 0, 0, 0, 0, 0, 134}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    {"Wonder", {0, 0, 0, 0, 0, 0, 0, 135}, {0, 64, 127, 90}, 0, 0, FX(0, 15, 25, 45)},
    /* FAMOUS END */
};

static const lgs_eng_t LGS_ENG_FM6 = {
    "FM6", OBV_FM6,
    {{"ALG", 0, 32}, {"FB", -7, 7}, {"MLVL", -64, 63}, {"MRAT", -16, 16}, {"MEG", -64, 63}, {"VMOD", -7, 7}, {"DTUN", 0, 127}, {"PTCH", 0, FM6_NSLOT - 1}},
    FM6_PRESETS, sizeof FM6_PRESETS / sizeof FM6_PRESETS[0],
    {2, 4, 6, 1}, fm6_note_on, fm6_render, fm6_block, fm6_done,
};
