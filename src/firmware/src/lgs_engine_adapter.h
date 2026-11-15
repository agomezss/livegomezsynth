/* SPDX-License-Identifier: GPL-3.0-only */
/* Clipper engines inside the OberVave voice core (T2.0).
 *
 * The Clipper engines (eng_digital.c, eng_drawbar.c, ...: Copyright (C) 2026 Leo Kuroshita, Hugelton Instruments, GPL-3.0)
 * render one voice per call over a 32-sample control block into a mono buffer, from a phase increment and an amplitude ramp.
 * This core runs 16-sample blocks and does the voice's pitch, envelopes and pan itself (voice_block). The adapter therefore
 * renders each voice over a 32-sample window every second block, using the amplitude of both blocks, plays the window's
 * first half at once and the second half one block later (a fixed 16-sample delay, 0.36 ms), and pans the mono result with
 * the voice's gains. The engines' own per-block time constants (decays, rotors) keep their Clipper values.
 * The engine's eight parameters live in analog slots the engine does not use (LGS_E_SLOT); a patch stores them minus the
 * parameter's minimum so that every value fits 0..127. */
#include "lgs_engine_tables.h"

#define SINE E32_SINE
#define ENV_EXP E32_ENV_EXP
#define CLIPPER_SYNTH 1
#define NVOICE NV
#define VOICE_FS 24000                          /* one voice peaks near -6 dBFS before the master (Clipper dsp.c) */

enum { P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7 };     /* index into track_t.p */
typedef struct { int16_t p[8]; } track_t;
typedef struct { uint8_t note, vel, gate, active; uint32_t ph[3]; int32_t s[8]; } cvoice_t;
typedef struct { uint32_t inc; int32_t pitch16, amp0, amp1, cutoff, shape, envq15, fine; } vmod_t;
typedef struct {
    const char *name;
    int8_t e[8];
    uint8_t env[4];                             /* ATK DEC SUS REL */
    int8_t fenv;
    uint8_t mono;
    uint8_t fx[4];                              /* DIST CHORUS DELAY REVERB sends, each stored + 1 */
    uint8_t arp[4];
    uint8_t pat;
    int8_t x[8];
} lgs_preset_t;
#define preset_t lgs_preset_t
#define FX(d, c, dl, r) .fx = {(d) + 1, (c) + 1, (dl) + 1, (r) + 1}
#define ARP(m, rt, o, g) .arp = {(m) + 1, (rt) + 1, (o) + 1, (g) + 1}
#define PAT(n) .pat = (n)
#define XP(...) .pat = 0                         /* Clipper's extra parameters (glide, LFO routing) are not carried over */

static inline int32_t mulq15(int32_t a, int32_t b) { return (a * b) >> 15; }
static inline int32_t mulq16(int32_t a, uint32_t k)
{
    return (int32_t)(((a >> 16) * (int32_t)k) + (int32_t)(((uint32_t)(a & 0xFFFF) * k) >> 16));
}
static inline int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline int32_t sine_i(uint32_t ph)
{
    uint32_t i = ph >> 22;
    int32_t a = SINE[i], b = SINE[(i + 1u) & 1023u];
    return a + (((b - a) * (int32_t)((ph >> 7) & 0x7FFFu)) >> 15);
}
static inline uint32_t noise32(int32_t *st) { uint32_t s = (uint32_t)*st; s ^= s << 13; s ^= s >> 17; s ^= s << 5; *st = (int32_t)s; return s; }
static inline int32_t amp_at(const vmod_t *m, uint32_t i)       /* the amplitude ramp over the 32-sample window */
{
    int32_t x = (m->amp1 - m->amp0) * (int32_t)i;
    return m->amp0 + ((x + ((x >> 31) & 31)) >> 5);
}

/* an engine as the UI and the voice core see it */
typedef struct { const char *name; int8_t min, max; } lgs_param_t;
typedef struct {
    const char *name;
    uint8_t id;                                 /* OBV_* */
    lgs_param_t ed[8];
    const lgs_preset_t *presets;
    uint32_t npresets;
    uint8_t macro[4];                           /* K1-K4: 0..7 = engine parameter, 8..11 = ATK DEC SUS REL */
    void (*note_on)(track_t *t, cvoice_t *v);
    void (*render)(track_t *t, cvoice_t *v, int32_t *out, uint32_t n, const vmod_t *m);
    void (*block)(track_t *t);
    int (*done)(track_t *t, cvoice_t *v);        /* 1: the engine's own envelopes have ended: free the voice */
} lgs_eng_t;

static uint32_t lgs_cur_part, lgs_cur_vi;       /* the voice being rendered (engines index their own state by them) */
static const lgs_eng_t *lgs_eng_by_id(uint32_t id);

/* analog parameter slots the plugged engines borrow: none of them touches pitch, envelope, LFO or level */
static const uint8_t LGS_E_SLOT[8] = {P_O1_PW, P_O1_LVL, P_O2_PW, P_O2_LVL, P_NOISE, P_XMOD, P_RESO, P_DRIVE};
static const uint8_t LGS_ENV_SLOT[4] = {P_AA, P_AD, P_AS, P_AR};

static int lgs_other_held(uint32_t part, uint32_t vi)   /* another voice of the part is still held by its key */
{
    uint32_t k;
    for (k = 0; k < NV; k++)
        if (k != vi && vc[k].part == part && vc[k].gate && vc[k].ae.stage != E_IDLE) return 1;
    return 0;
}

static track_t lgs_trk[NPART];

static void lgs_track_refresh(uint32_t part, const lgs_eng_t *E)
{
    const uint8_t *v = live_p[part].v;
    uint32_t i;
    for (i = 0; i < 8u; i++) lgs_trk[part].p[i] = (int16_t)((int32_t)v[LGS_E_SLOT[i]] + E->ed[i].min);
}

static void lgs_block(uint32_t part)                    /* once per window (odd blocks), before the voices */
{
    const lgs_eng_t *E = lgs_eng_by_id(obv_engine_track[part].active);
    if (!E) return;
    lgs_track_refresh(part, E);
    if ((eng_t >> 4 & 1u) && E->block) { lgs_cur_part = part; E->block(&lgs_trk[part]); }
}

static void lgs_start(voice_t *v)
{
    const lgs_eng_t *E = lgs_eng_by_id(obv_voice_engine[v - vc]);
    cvoice_t cv;
    if (!E) return;
    memset(&cv, 0, sizeof cv);
    cv.note = v->note; cv.vel = v->vel; cv.gate = 1; cv.active = 1;
    lgs_cur_part = v->part; lgs_cur_vi = (uint32_t)(v - vc);
    lgs_track_refresh(v->part, E);
    if (E->note_on) E->note_on(&lgs_trk[v->part], &cv);
    memcpy(v->ev.ph, cv.ph, sizeof cv.ph);
    memcpy(v->ev.s, cv.s, sizeof cv.s);
    v->ev.ok = 0;
    memset(v->ev.eb, 0, sizeof v->ev.eb);
}

static void lgs_emit(voice_t *v, int32_t *out, const int32_t *y)
{
    uint32_t i;
    int32_t gl = v->gl, gr = v->gr, last = 0;
    for (i = 0; i < CTL; i++) {
        int32_t s = y[i];
        if (v->guard_in) { s = (int32_t)((int64_t)s * (STEAL_FADE - v->guard_in) / STEAL_FADE); v->guard_in--; }
        out[2u * i] += (s * gl) >> 12;
        out[2u * i + 1u] += (s * gr) >> 12;
        last = s;
    }
    v->guard_l = (last * gl) >> 12;
    v->guard_r = (last * gr) >> 12;
}

static void lgs_voice_render(voice_t *v, int32_t *out)
{
    uint32_t vi = (uint32_t)(v - vc);
    const lgs_eng_t *E = lgs_eng_by_id(obv_voice_engine[vi]);
    lgs_ev_t *ev = &v->ev;
    if (!E) return;
    if (!(eng_t >> 4 & 1u)) {                   /* first block of a window: play the previous window's second half */
        ev->a0 = v->amp0;
        ev->ok = 1;
        lgs_emit(v, out, ev->eb);
        memset(ev->eb, 0, sizeof ev->eb);
    } else {                                    /* second block: render the whole window */
        cvoice_t cv;
        vmod_t m;
        int32_t y[32];
        uint32_t i;
        for (i = 0; i < 32u; i++) y[i] = 0;
        memcpy(cv.ph, ev->ph, sizeof cv.ph);
        memcpy(cv.s, ev->s, sizeof cv.s);
        cv.note = v->note; cv.vel = v->vel; cv.gate = v->gate; cv.active = 1;
        m.inc = v->inc1;
        m.pitch16 = v->pitch16 >> 12;
        m.amp0 = ev->ok ? ev->a0 : v->amp0;
        m.amp1 = v->amp1;
        m.cutoff = (int32_t)ctl_mod_cut[v->part & 3u] << 8;
        m.shape = 64 << 8;
        m.envq15 = v->amp1;
        m.fine = 0;
        lgs_cur_part = v->part; lgs_cur_vi = vi;
        E->render(&lgs_trk[v->part], &cv, y, 32, &m);
        if (E->done && E->done(&lgs_trk[v->part], &cv)) { v->gate = 0; v->ae.stage = E_IDLE; v->ae.level = 0; v->amp0 = v->amp1 = 0; }
        memcpy(ev->ph, cv.ph, sizeof cv.ph);
        memcpy(ev->s, cv.s, sizeof cv.s);
        for (i = 0; i < 16u; i++) ev->eb[i] = y[16u + i];
        lgs_emit(v, out, y);
    }
}

static void lgs_engine_reset(uint32_t part) { (void)part; }
static const obv_engine_ops_t lgs_clipper_ops = {"CLIPPER", lgs_start, lgs_voice_render, 0, 0, lgs_engine_reset, 0, lgs_block};
