/* SPDX-License-Identifier: GPL-3.0-only
 * SP: IMA ADPCM sample playback. Derived from Clipper's eng_sample.c (Copyright (C) 2026 Leo Kuroshita (@kurogedelic),
 * Hugelton Instruments, GPL-3.0): the zone lookup, the ADPCM decoder, the interpolating player and its TUNE / BITS / LOOP /
 * CUT / DRV controls are the same; the user-slot flash code is not carried over. The sets are built-in (lgs_samples.h, made by
 * tools/gen_lgs_samples.py). Voice state: ph[0] position, ph[1] fraction Q16, s[0] predictor, s[1] step index, s[2] previous
 * sample, s[3] current sample, s[4] zone, s[6] ended, s[7] low-pass. */
typedef struct {
    uint32_t off, n, ls, le;     /* byte offset, samples, loop start / end (sample index) */
    uint32_t rate;               /* source rate / 44100, Q16 */
    int16_t root16;              /* root pitch in 1/16 semitone */
    int16_t pred;                /* ADPCM state at the loop start */
    uint8_t idx, lo, hi, looped; /* looped: 0 one-shot, 1 loops [ls, le] */
} smp_zone_t;
typedef struct { const char *name; uint16_t z0, nz; } smp_set_t;
#include "lgs_samples.h"

static const int16_t IMA_STEP[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428,
    4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767};
static const int8_t IMA_IDX[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

/* 2^(d16 / 192), Q16, for d16 >= -3072 (16 octaves down) */
static uint32_t pow2_q16(int32_t d16)
{
    uint32_t u = (uint32_t)(d16 + 192 * 16), oct = u / 192u, r = E32_POW2[u % 192u];
    return oct >= 16u ? r << (oct - 16u) : r >> (16u - oct);
}

static const smp_zone_t *smp_zone(uint32_t zi) { return &SMP_ZONES[zi]; }

static inline int32_t sample_next(const smp_zone_t *z, cvoice_t *v, int loop)
{
    uint32_t pos = v->ph[0], b = SMP_DATA[z->off + (pos >> 1)];
    uint32_t code = (pos & 1u) ? (b >> 4) : (b & 15u);
    int32_t step = IMA_STEP[(uint32_t)v->s[1] <= 88u ? v->s[1] : 88], vd = step >> 3;
    if (code & 4u) vd += step;
    if (code & 2u) vd += step >> 1;
    if (code & 1u) vd += step >> 2;
    v->s[0] = clamp(v->s[0] + ((code & 8u) ? -vd : vd), -32768, 32767);
    v->s[1] = clamp(v->s[1] + IMA_IDX[code & 7u], 0, 88);
    pos++;
    if (pos > z->le && z->looped && loop) {
        pos = z->ls;
        v->s[0] = z->pred;
        v->s[1] = z->idx;
    }
    v->ph[0] = pos;
    return v->s[0];
}

static void sample_note_on(track_t *t, cvoice_t *v)
{
    uint32_t si = (uint32_t)t->p[P_E0] % SMP_NSETS, i, zi = 0xFFFFu;
    const smp_set_t *set = &SMP_SETS[si];
    for (i = 0; i < set->nz; i++)
        if (v->note >= SMP_ZONES[set->z0 + i].lo && v->note <= SMP_ZONES[set->z0 + i].hi) zi = set->z0 + i;
    v->s[4] = (int32_t)(zi == 0xFFFFu ? set->z0 : zi);
    v->ph[0] = 0;
    v->ph[1] = 0;
    v->s[0] = 0;
    v->s[1] = 0;
    v->s[2] = v->s[3] = 0;
    v->s[6] = zi == 0xFFFFu;
    v->s[7] = 0;
}

static void sample_render(track_t *t, cvoice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    const smp_zone_t *z = smp_zone((uint32_t)v->s[4]);
    uint32_t i, frac = v->ph[1];
    int32_t d16 = clamp(m->pitch16 + p[P_E1] * 16 - z->root16, -1536, 576);   /* up to 3 octaves up: bounded decode load */
    uint32_t r = pow2_q16(d16), stepq = (r >> 8) * (z->rate >> 8);          /* Q16 samples per output */
    int32_t bits = p[P_E2], lp = 4000 + ((clamp((p[P_E4] << 8) + m->cutoff, 0, 127 << 8) * 28767) >> 15);
    int32_t drv = p[P_E6], sh = bits / 10;
    if (v->s[6] || !z->n) return;
    if (v->ph[0] == 0 && v->ph[1] == 0 && v->s[3] == 0) v->s[3] = sample_next(z, v, p[P_E3]);
    for (i = 0; i < n; i++) {
        int32_t s;
        frac += stepq;
        while (frac >= 65536u) {
            frac -= 65536u;
            v->s[2] = v->s[3];
            if (v->ph[0] >= z->n) { v->s[6] = 1; v->s[3] = 0; break; }
            v->s[3] = sample_next(z, v, p[P_E3]);
        }
        s = v->s[2] + (((v->s[3] - v->s[2]) * (int32_t)(frac >> 1)) >> 15);
        if (sh) s = (s >> sh) << sh;
        if (drv) s = softclip(s + (((s >> 2) * (drv * 150)) >> 11));
        v->s[7] += mulq15(s - v->s[7], lp);
        out[i] += mulq15(mulq15(v->s[7], amp_at(m, i)), VOICE_FS) << 1;
        if (v->s[6]) break;
    }
    v->ph[1] = frac;
}

static const lgs_eng_t LGS_ENG_SP = {
    "SP", OBV_SP,
    {{"SET", 0, 15}, {"TUNE", -24, 24}, {"BITS", 0, 127}, {"LOOP", 0, 1}, {"CUT", 0, 127}, {"-", 0, 0}, {"DRV", 0, 127}, {"-", 0, 0}},
    SMP_PRESET_TABLE, sizeof SMP_PRESET_TABLE / sizeof SMP_PRESET_TABLE[0],
    {4, 6, 8, 11}, sample_note_on, sample_render, 0,
};
