/* SPDX-License-Identifier: GPL-3.0-only */
/* LiveGomezSynth data model (T1.1, T1.4): catalog index, favorites, performances, loading a sound
 * into an engine part, global effect units, note routing by octave range. No UI in here.
 * Parts: LAYER 1 = 0, LAYER 2 = 1, SOLO 1 = 2, SOLO 2 = 3 (D-30). */
#include "lgs_catalog_data.h"

#define LGS_NFAV 64u                            /* 4 banks x 16 pads */
#define LGS_NPERF 112u                          /* 8 banks x 14 combos */
#define LGS_NLINE 4u
#define NTHEME 8u                                /* screen themes (lgs_ui.h) */
#define LGS_NFX 11u                             /* P_CHO_RATE .. P_REV_MIX */

/* K1-K4 of a sound: which patch byte, its name, and how far it goes (value stored = native value - min) */
typedef struct { const char *name; uint8_t slot; int8_t min; uint8_t range; } knob_t;
static const uint8_t KNOB_SLOT_SYN[4] = {P_CUTOFF, P_RESO, P_FENV_AMT, P_AR};
static const uint8_t KNOB_SLOT_DRM[4] = {DP_MORPH, DP_TONE, DP_LENGTH, DP_FOLD};     /* drum kits: morph, tone, length, fold */
static const lgs_eng_t *lgs_eng_by_id(uint32_t id)
{
    return id == OBV_FM2 ? &LGS_ENG_FM2 : id == OBV_ORG ? &LGS_ENG_ORG : id == OBV_SP ? &LGS_ENG_SP : id == OBV_FM6 ? &LGS_ENG_FM6 : id == OBV_VOC ? &LGS_ENG_VOC : 0;
}
static knob_t knob_desc(uint32_t engine, uint32_t i)
{
    static const char *const DN[4] = {"MORPH", "TONE", "LENGTH", "FOLD"}, *const EN[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
    const lgs_eng_t *E = lgs_eng_by_id(engine);
    knob_t k;
    if (E) {
        uint32_t m = E->macro[i];
        if (m < 8u) { k.name = E->ed[m].name; k.slot = LGS_E_SLOT[m]; k.min = E->ed[m].min; k.range = (uint8_t)(E->ed[m].max - E->ed[m].min); }
        else { k.name = EN[m - 8u]; k.slot = LGS_ENV_SLOT[m - 8u]; k.min = 0; k.range = 127; }
    } else if (engine == OBV_DRUMS) { k.name = DN[i]; k.slot = KNOB_SLOT_DRM[i]; k.min = 0; k.range = 127; }
    else { k.name = PDEF[KNOB_SLOT_SYN[i]].name; k.slot = KNOB_SLOT_SYN[i]; k.min = 0; k.range = 127; }
    return k;
}

typedef struct { uint16_t patch; uint8_t tw[4], snd[4]; } sound_t;      /* patch id (0 = empty), K1-K4, sends CHO DLY DIST REV */
typedef struct { sound_t s; uint8_t origin, lo, hi, vol, flags; } lgline_t;   /* origin: favorite number 1..64 or 0 (display only) */
typedef struct { lgline_t line[LGS_NLINE]; } perf_t;
enum { LF_SUSTAIN = 1, LF_BEND = 2, LF_MOD_SHIFT = 2, LF_OCT_SHIFT = 5 };   /* flags bit 2..4: mod action 0..5; bit 5..7: the line's own octave -2..+2 (two's complement, 0 in older data) */
enum { MOD_DEFAULT, MOD_OFF, MOD_FILTER, MOD_BRIGHT, MOD_SWELL, MOD_REVERB, MOD_N };
static int32_t line_oct(const lgline_t *l) { int32_t v = (int32_t)(l->flags >> LF_OCT_SHIFT & 7u); return v > 3 ? v - 8 : v; }
static void line_oct_set(lgline_t *l, int32_t o) { l->flags = (uint8_t)((l->flags & 31u) | (uint32_t)(o & 7) << LF_OCT_SHIFT); }

/* the user data: this block is what gets persisted (T1.9) */
#define LGS_DB_MAGIC 0x4C47530Au
#define LGS_DB_VER 2u
static struct { uint32_t magic, ver, gen, theme; sound_t fav[LGS_NFAV]; perf_t perf[LGS_NPERF]; uint32_t crc; } db;   /* layout of lgs_ab.h */
static uint8_t db_dirty;

/* ---- catalog --------------------------------------------------------------------------------- */
static uint16_t cat_first[LGS_NCAT + 1];
static uint8_t cat_n[LGS_NCAT + 1];        /* index into LGS_PATCH; each category is one contiguous run (tools/gen_catalog.py sorts them) */

static void cat_init(void)
{
    uint32_t i;
    for (i = 0; i < LGS_NPATCH; i++) {
        uint32_t c = LGS_PATCH[i].cat;
        if (!cat_n[c]++) cat_first[c] = (uint16_t)i;
    }
}

static const lgs_patch_t *pat_by_id(uint32_t id)
{
    uint32_t i;
    for (i = 0; i < LGS_NPATCH; i++)
        if (LGS_PATCH[i].id == id) return &LGS_PATCH[i];
    return 0;
}

static uint32_t pat_index(uint32_t id)          /* position in the global order; 0 if unknown */
{
    uint32_t i;
    for (i = 0; i < LGS_NPATCH; i++)
        if (LGS_PATCH[i].id == id) return i;
    return 0;
}

/* ---- effects: the three effect units are global, each part feeds them through its sends (D-24) --- */
static uint8_t fx_g[LGS_NFX];

static void lgs_fx_init(void)
{
    patch_t d;
    uint32_t i;
    patch_defaults(&d);
    for (i = 0; i < LGS_NFX; i++) fx_g[i] = d.v[P_CHO_RATE + i];
    fx_g[P_CHO_MIX - P_CHO_RATE] = fx_g[P_DLY_MIX - P_CHO_RATE] = fx_g[P_REV_MIX - P_CHO_RATE] = 127;   /* full return: the sends set the amount */
    live_p[0] = d;
    for (i = 0; i < LGS_NFX; i++) live_p[0].v[P_CHO_RATE + i] = fx_g[i];
    ctl_dv_mix = 127;                           /* DIST send bus: parallel, already in the engine */
}

/* ---- sounds ------------------------------------------------------------------------------------ */
static void patch_build(patch_t *np, const lgs_patch_t *cp)
{
    const lgs_eng_t *E = lgs_eng_by_id(cp->engine);
    if (cp->engine == OBV_DRUMS) obv_drums_preset(np, cp->data);
    else if (E) {                                   /* a Clipper engine preset: its eight parameters, envelope and mode */
        const lgs_preset_t *pr = &E->presets[cp->data];
        uint32_t i;
        patch_defaults(np);
        obv_patch_set_engine(np, E->id);
        for (i = 0; i < 8u; i++) np->v[LGS_E_SLOT[i]] = (uint8_t)clampi(pr->e[i] - E->ed[i].min, 0, 127);
        for (i = 0; i < 4u; i++) np->v[LGS_ENV_SLOT[i]] = pr->env[i];
        np->v[P_MODE] = pr->mono ? M_MONO : M_POLY;
        for (i = 0; i < PATCH_NAME && pr->name[i]; i++) np->v[NPARAM + i] = (uint8_t)pr->name[i];
    } else if (cp->data >= NFACT) patch_from_factory(np, &FACTORY_LGS[cp->data - NFACT]);
    else patch_from_factory(np, &FACTORY[cp->data]);
    if (cp->id == 353u || cp->id == 356u || cp->id == 359u) np->v[P_MODE] = M_POLY;     /* one-shot effects (Reso Zap, Laser, Thump): fast hits overlap */
}

static uint32_t sound_engine(const sound_t *s) { const lgs_patch_t *cp = pat_by_id(s->patch); return cp ? cp->engine : 0u; }

static void sound_defaults(sound_t *s, uint32_t patch_id)      /* a patch as it ships: its own K1-K4 values and effect amounts */
{
    const lgs_patch_t *cp = pat_by_id(patch_id);
    patch_t np;
    uint32_t i;
    s->patch = (uint16_t)patch_id;
    if (!cp) return;
    patch_build(&np, cp);
    for (i = 0; i < 4u; i++) s->tw[i] = np.v[knob_desc(cp->engine, i).slot];
    s->snd[0] = np.v[P_CHO_MIX];
    s->snd[1] = np.v[P_DLY_MIX];
    s->snd[2] = 0;
    s->snd[3] = np.v[P_REV_MIX];
    if (lgs_eng_by_id(cp->engine)) {                /* Clipper presets carry their sends: DIST CHORUS DELAY REVERB, each + 1 */
        const lgs_preset_t *pr = &lgs_eng_by_id(cp->engine)->presets[cp->data];
        s->snd[0] = pr->fx[1] ? (uint8_t)(pr->fx[1] - 1u) : 0u;
        s->snd[1] = pr->fx[2] ? (uint8_t)(pr->fx[2] - 1u) : 0u;
        s->snd[2] = pr->fx[0] ? (uint8_t)(pr->fx[0] - 1u) : 0u;
        s->snd[3] = pr->fx[3] ? (uint8_t)(pr->fx[3] - 1u) : 0u;
    }
}

static uint32_t vol_to_lvl(uint32_t vol) { return vol * 100u / 99u; }   /* 0..99 -> engine level (100 = unity) */

static uint8_t fx_sends4 = 1;                   /* PATCH: the four sends (chorus, delay, dist, reverb); PERFORM and EDIT: the reverb send only (less CPU) */
static uint8_t mod_w;                           /* mod wheel 0..127 */
static uint8_t mod_act[LGS_NLINE];              /* MOD_* of each part (lines' flags, lgs_ui.h flags_apply) */

static void part_levels(uint32_t p, const sound_t *s, uint32_t vol)
{
    const lgs_patch_t *cp = pat_by_id(s->patch);
    uint32_t lvl = vol_to_lvl(vol), rev = s->snd[3], act = mod_act[p];
    int32_t cut = 0;
    ctl_mix_trim[p] = cp ? cp->trim : 64u;
    if (act == MOD_SWELL) { lvl += mod_w * 27u / 127u; if (lvl > 127u) lvl = 127u; }
    else if (act == MOD_REVERB) rev += mod_w * (127u - rev) / 127u;
    else if (act == MOD_FILTER) cut = mod_w / 2;
    else if (act == MOD_BRIGHT) cut = mod_w;
    ctl_mod_cut[p] = (int8_t)cut;
    ctl_mix_lvl[p] = (uint8_t)lvl;
    ctl_mix_cho[p] = fx_sends4 ? s->snd[0] : 0u;
    ctl_mix_dly[p] = fx_sends4 ? s->snd[1] : 0u;
    ctl_mix_drv[p] = fx_sends4 ? s->snd[2] : 0u;
    ctl_mix_rev[p] = (uint8_t)rev;
}

static void part_apply(uint32_t p, const sound_t *s, uint32_t vol)       /* load a sound into an engine part */
{
    const lgs_patch_t *cp = pat_by_id(s->patch);
    patch_t np;
    uint32_t i;
    if (!cp) return;
    patch_build(&np, cp);
    for (i = 0; i < 4u; i++) np.v[knob_desc(cp->engine, i).slot] = s->tw[i];
    if (p == 0u)
        for (i = 0; i < LGS_NFX; i++) np.v[P_CHO_RATE + i] = fx_g[i];
    fm1_irq_off();
    live_p[p] = np;
    fm1_irq_on();
    part_levels(p, s, vol);
}

/* ---- defaults shipped with the firmware (demo content for the simulator, replaced by the real
 * factory set in M3) ----------------------------------------------------------------------------- */
static uint32_t pid_of(uint32_t factory)
{
    uint32_t i;
    for (i = 0; i < LGS_NPATCH; i++)
        if (LGS_PATCH[i].engine == OBV_ANALOG && LGS_PATCH[i].data == factory) return LGS_PATCH[i].id;
    return 0;
}

static void line_set(lgline_t *l, uint32_t factory, uint32_t lo, uint32_t hi)
{
    sound_defaults(&l->s, pid_of(factory));
    l->origin = 0; l->lo = (uint8_t)lo; l->hi = (uint8_t)hi; l->vol = 99; l->flags = LF_SUSTAIN | LF_BEND;
}

static void db_defaults(void)
{
    static const uint8_t FAV_FACTORY[] = {50, 52, 51, 6, 1, 8, 57, 0, 60, 42, 40, 59};   /* factory indices (obv_patches.h) */
    uint32_t i;
    memset(&db, 0, sizeof db);
    db.magic = LGS_DB_MAGIC;
    db.ver = LGS_DB_VER;
    for (i = 0; i < sizeof FAV_FACTORY; i++) sound_defaults(&db.fav[i], pid_of(FAV_FACTORY[i]));
    /* A-1: bass below, brass above, a lead and a pad as the two solos */
    line_set(&db.perf[0].line[0], 6, 1, 3);
    line_set(&db.perf[0].line[1], 1, 4, 7);
    line_set(&db.perf[0].line[2], 57, 1, 7);
    line_set(&db.perf[0].line[3], 8, 1, 7);
    /* A-2: keys with a pad layer, two lead solos */
    line_set(&db.perf[1].line[0], 50, 1, 7);
    line_set(&db.perf[1].line[1], 42, 1, 7);
    line_set(&db.perf[1].line[2], 59, 1, 7);
    line_set(&db.perf[1].line[3], 58, 1, 7);
    /* A-3: arp bass below, arp above */
    line_set(&db.perf[2].line[0], 37, 1, 3);
    line_set(&db.perf[2].line[1], 0, 4, 7);
    db_dirty = 1;
}

static uint32_t perf_filled(const perf_t *pf)
{
    uint32_t i;
    for (i = 0; i < LGS_NLINE; i++)
        if (pf->line[i].s.patch) return 1;
    return 0;
}

/* ---- note routing (D-27): the line sounds when the played note, after the OCTAVE shift, lies in its range --- */
static int8_t oct_shift;                        /* OCTAVE -/+ buttons: -2..+2 octaves */
static uint8_t held_mask[128], held_note[128][NPART];   /* per physical key: the parts it went to and the note each got */

static uint32_t route_mask(const perf_t *pf, uint32_t solo, uint32_t n)
{
    int32_t o = (int32_t)n / 12 - 1;
    uint32_t m = 0, i, first = solo ? 1u + solo : 0u, last = solo ? 1u + solo : 1u;
    for (i = first; i <= last; i++) {
        const lgline_t *l = &pf->line[i];
        if (l->s.patch && (i >= 2u || (o >= (int32_t)l->lo && o <= (int32_t)l->hi))) m |= 1u << i;   /* a solo line plays on the whole keyboard */
    }
    return m;
}

static void keys_release_all(void)
{
    uint32_t k, p;
    for (k = 0; k < 128u; k++)
        if (held_mask[k]) {
            for (p = 0; p < NPART; p++)
                if (held_mask[k] >> p & 1u) note_event_p(p, 0, held_note[k][p], 0);
            held_mask[k] = 0;
        }
}
