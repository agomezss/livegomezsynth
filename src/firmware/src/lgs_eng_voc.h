/* SPDX-License-Identifier: GPL-3.0-only */
/* VOC: a vowel (formant) voice for choirs and vocal pads. LiveGomezSynth's own design.
 *
 * Per voice: two detuned sawtooths (an ensemble of two singers) and a little breath noise, tilted by a one-pole
 * low-pass, through three parallel band-pass filters (state-variable) placed at the formants of a vowel. The vowel is a
 * position along U O A E I (VOWL); SIZE scales the formants (a child up, a bass down); BITE narrows or widens them
 * (a hall of voices against one clear singer); MOVE drifts the vowel slowly; VIB is a vibrato that comes in after the
 * attack; SPRD the ensemble detune; BRTH the breath; TILT the brightness of the source.
 *
 * Cost: per sample and voice two phase steps, one low-pass, three filters (about 12 multiplies). The formants, the
 * vibrato and the drift run once per block and part (voc_block). The amplitude is the core's ADSR. */
#define VOC_NF 3

/* U O A E I: F1 F2 F3 in Hz, then the gains of F2 and F3 (Q12) */
static const int16_t VOC_V[5][5] = {
    {330, 650, 2650, 325, 410},
    {450, 800, 2700, 1455, 817},
    {750, 1150, 2700, 2053, 1029},
    {400, 1900, 2700, 1455, 817},
    {300, 2100, 2800, 1455, 728},
};
/* 2^(SIZE / 36) for SIZE -12..12, Q12 */
static const uint16_t VOC_SIZE[25] = {3251, 3315, 3379, 3445, 3512, 3581, 3651, 3722, 3795, 3869, 3944, 4021, 4096,
                                      4176, 4257, 4340, 4425, 4511, 4599, 4689, 4780, 4874, 4969, 5066, 5165};

typedef struct {                 /* per part, once per block */
    int32_t F[VOC_NF];           /* the formants now (Hz), eased to the vowel */
    int32_t f[VOC_NF];           /* filter coefficient, Q12 */
    int32_t q[VOC_NF];           /* damping 1 / Q, Q12 */
    int32_t gq[VOC_NF];          /* output gain times damping, Q12 */
    uint32_t vph, mph;           /* vibrato and drift phases */
    int32_t vib, spr;            /* pitch modulation and detune, Q16 of the note increment */
    int32_t breath, tilt;        /* breath level, source low-pass coefficient (Q15) */
} voc_trk_t;

typedef struct {                 /* per voice */
    uint32_t pa, pb;
    int32_t lp, nz;
    int32_t bp[VOC_NF], lpf[VOC_NF];
    uint32_t age;
} voc_vc_t;

static voc_trk_t voc_t[NPART];
static voc_vc_t voc_v[NVOICE];

static void voc_block(track_t *t)
{
    voc_trk_t *T = &voc_t[lgs_cur_part];
    const int16_t *p = t->p;
    int32_t pos, i0, fr, k, bw, size = clamp(p[P_E1], -12, 12), mv;
    uint32_t sz = VOC_SIZE[size + 12];
    T->mph += 779000u;                                  /* the drift: 0.25 Hz */
    T->vph += 17143000u;                                /* the vibrato: 5.5 Hz */
    mv = (sine_i(T->mph) * p[P_E6]) >> 15;              /* +-127 at full MOVE */
    pos = clamp(p[P_E0] * 1023 / 127 + (mv * 3 >> 1), 0, 1023);   /* along U O A E I, 256 a vowel */
    i0 = pos >> 8;
    fr = pos & 255;
    bw = 220 - (p[P_E2] * 150) / 127;                   /* first formant's bandwidth (Hz): 220 .. 70 */
    for (k = 0; k < VOC_NF; k++) {
        int32_t a = VOC_V[i0][k], b = VOC_V[i0 + 1 < 5 ? i0 + 1 : 4][k], F, g, bwk, qd;
        F = (int32_t)(((uint32_t)(a + (((b - a) * fr) >> 8)) * sz) >> 12);
        T->F[k] += (F - T->F[k]) >> 2;
        T->F[k] += T->F[k] == 0;
        T->f[k] = (T->F[k] * 2390) >> 12;
        bwk = bw + bw * k / 2;                          /* the upper formants are wider */
        qd = clamp((bwk * 4096) / T->F[k], 200, 4000);
        T->q[k] = qd;
        if (k == 0) g = 4096;
        else {
            int32_t ga = VOC_V[i0][k + 2], gb = VOC_V[i0 + 1 < 5 ? i0 + 1 : 4][k + 2];
            g = ga + (((gb - ga) * fr) >> 8);
        }
        T->gq[k] = (g * qd) >> 12;
    }
    T->vib = (sine_i(T->vph) * p[P_E4]) >> 11;          /* +-0.5 semitone at 127 */
    T->spr = (p[P_E3] * 525) / 127;
    T->breath = p[P_E5];
    T->tilt = 6000 + p[P_E7] * 200;
}

static void voc_note_on(track_t *t, cvoice_t *v)
{
    voc_vc_t *V = &voc_v[lgs_cur_vi];
    uint32_t k;
    (void)t; (void)v;
    V->pa = lgs_cur_vi * 0x9E3779B9u;
    V->pb = V->pa + 0x55555555u;
    V->nz = 0x2468ACE1 + (int32_t)lgs_cur_vi;
    V->lp = 0;
    V->age = 0;
    for (k = 0; k < VOC_NF; k++) V->bp[k] = V->lpf[k] = 0;
}

static void voc_render(track_t *t, cvoice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const voc_trk_t *T = &voc_t[lgs_cur_part];
    voc_vc_t *V = &voc_v[lgs_cur_vi];
    uint32_t i, inc = m->inc;
    int32_t ramp, vib, da, db, lp = V->lp, nst = V->nz, tilt = T->tilt, br = T->breath;
    uint32_t pa = V->pa, pb = V->pb;
    int32_t bp0 = V->bp[0], bp1 = V->bp[1], bp2 = V->bp[2], lf0 = V->lpf[0], lf1 = V->lpf[1], lf2 = V->lpf[2];
    int32_t f0 = T->f[0], f1 = T->f[1], f2 = T->f[2], q0 = T->q[0], q1 = T->q[1], q2 = T->q[2];
    int32_t g0 = T->gq[0] >> 2, g1 = T->gq[1] >> 2, g2 = T->gq[2] >> 2;
    uint32_t ia, ib;
    (void)t; (void)v;
    if (n > E32_CTL) n = E32_CTL;
    V->age++;
    ramp = V->age > 280u ? 32767 : (int32_t)(V->age * 117u);      /* the vibrato comes in over ~0.4 s */
    vib = mulq15(T->vib, ramp);
    da = vib + T->spr;
    db = vib - T->spr;
    ia = inc + (uint32_t)((int32_t)(inc >> 16) * da);
    ib = inc + (uint32_t)((int32_t)(inc >> 16) * db);
    for (i = 0; i < n; i++) {
        int32_t x, y, hp, s0, s1, s2;
        pa += ia;
        pb += ib;
        x = (((int32_t)(pa >> 18) - 8192) + ((int32_t)(pb >> 18) - 8192)) >> 1;     /* two saws, +-8192 */
        x += (((int32_t)(noise32(&nst) >> 18) - 8192) * br) >> 8;                     /* breath */
        lp += ((x - lp) * tilt) >> 15;
        lf0 += (bp0 * f0) >> 12;
        hp = lp - lf0 - ((bp0 * q0) >> 12);
        bp0 += (hp * f0) >> 12;
        s0 = (bp0 >> 2) * g0;
        lf1 += (bp1 * f1) >> 12;
        hp = lp - lf1 - ((bp1 * q1) >> 12);
        bp1 += (hp * f1) >> 12;
        s1 = (bp1 >> 2) * g1;
        lf2 += (bp2 * f2) >> 12;
        hp = lp - lf2 - ((bp2 * q2) >> 12);
        bp2 += (hp * f2) >> 12;
        s2 = (bp2 >> 2) * g2;
        y = (s0 + s1 + s2) >> 6;
        out[i] += mulq15(mulq15(y, amp_at(m, i)), VOICE_FS) << 1;
    }
    V->pa = pa; V->pb = pb; V->lp = lp; V->nz = nst;
    V->bp[0] = bp0; V->bp[1] = bp1; V->bp[2] = bp2;
    V->lpf[0] = lf0; V->lpf[1] = lf1; V->lpf[2] = lf2;
}

static const lgs_preset_t VOC_PRESETS[] = {
    /* name, {VOWL, SIZE, BITE, SPRD, VIB, BRTH, MOVE, TILT}, {A D S R}, fenv, mono, fx */
    {"CHOIR AAHS", {64, 0, 50, 60, 40, 10, 10, 90}, {40, 64, 127, 70}, 0, 0, FX(0, 40, 0, 60)},
    {"CHOIR OOHS", {14, 0, 40, 60, 40, 8, 8, 70}, {50, 64, 127, 80}, 0, 0, FX(0, 40, 0, 64)},
    {"CHOIR SWELL", {44, 0, 45, 70, 30, 12, 50, 80}, {85, 64, 127, 90}, 0, 0, FX(0, 44, 0, 70)},
    {"MALE CHOIR", {52, -6, 45, 80, 36, 10, 10, 70}, {36, 64, 127, 70}, 0, 0, FX(0, 36, 0, 60)},
    {"CHILD CHOIR", {76, 8, 60, 50, 50, 12, 8, 110}, {30, 64, 127, 60}, 0, 0, FX(0, 36, 0, 64)},
    {"VOWEL PAD", {60, 0, 30, 90, 20, 6, 90, 80}, {90, 64, 127, 100}, 0, 0, FX(0, 56, 0, 80)},
    {"WHISPER CHOIR", {64, 0, 25, 40, 20, 100, 24, 90}, {60, 64, 127, 90}, 0, 0, FX(0, 30, 0, 72)},
    {"EEH SYNTH", {118, 0, 60, 60, 30, 6, 6, 120}, {20, 64, 127, 60}, 0, 0, FX(0, 36, 0, 52)},
};

static const lgs_eng_t LGS_ENG_VOC = {
    "VOC", OBV_VOC,
    {{"VOWL", 0, 127}, {"SIZE", -12, 12}, {"BITE", 0, 127}, {"SPRD", 0, 127}, {"VIB", 0, 127}, {"BRTH", 0, 127},
     {"MOVE", 0, 127}, {"TILT", 0, 127}},
    VOC_PRESETS, sizeof VOC_PRESETS / sizeof VOC_PRESETS[0],
    {0, 2, 6, 5}, voc_note_on, voc_render, voc_block,
};
