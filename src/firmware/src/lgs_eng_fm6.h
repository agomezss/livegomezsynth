/* SPDX-License-Identifier: GPL-3.0-only
 * FM6: classic 6-operator FM. The synthesis is msfa (Dexed's core, Apache-2.0) as ported to integer C by Clipper
 * (lgs_fm6_core.h); this file is the glue, adapted from Clipper's eng_fm6.c (Copyright (C) 2026 Leo Kuroshita,
 * Hugelton Instruments, GPL-3.0). The patch (one of Clipper's eight authored voices) is the sound; the eight engine values are
 * macros on top of it: ALG (0 = the patch's), FB, MLVL (modulator level: the brightness), MRAT, MEG (modulator envelope
 * speed), VMOD, DTUN (carrier spread), PTCH (which voice). The operator envelopes are the voice's amplitude and end it
 * (done): the patch's ADSR here only has to stay out of the way. */
#include "lgs_fm6_data.h"
#include "lgs_fm6_core.h"
#include "lgs_fm6_patch.h"
#define FM6_PACKED 128u
#define FM6_NSLOT FM6_NFACTORY

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
    fm6_unpack(FM6_FACTORY[s % FM6_NSLOT], v);
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
static const uint8_t FM6_SHIFT[FM6_NSLOT] = {26, 26, 26, 26, 26, 26, 26, 26};

static void fm6_render(track_t *t, cvoice_t *v, int32_t *out, uint32_t len, const vmod_t *m)
{
    fm6_note_t *n = &fm6_notes[lgs_cur_vi];
    uint32_t tr = lgs_cur_part, i;
    const uint8_t *p;
    int32_t bus[FM6_N], lf, fb, lvl, a, da;
    uint32_t sh = FM6_SHIFT[fm6_slot[lgs_cur_part] % FM6_NSLOT];
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
    /* FAMOUS END */
};

static const lgs_eng_t LGS_ENG_FM6 = {
    "FM6", OBV_FM6,
    {{"ALG", 0, 32}, {"FB", -7, 7}, {"MLVL", -64, 63}, {"MRAT", -16, 16}, {"MEG", -64, 63}, {"VMOD", -7, 7}, {"DTUN", 0, 127}, {"PTCH", 0, FM6_NFACTORY - 1}},
    FM6_PRESETS, sizeof FM6_PRESETS / sizeof FM6_PRESETS[0],
    {2, 4, 6, 1}, fm6_note_on, fm6_render, fm6_block, fm6_done,
};
