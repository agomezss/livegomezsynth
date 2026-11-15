/* SPDX-License-Identifier: GPL-3.0-only */
/* LiveGomezSynth user interface (M1): PATCH / PERFORM / EDIT modes, the KNOB BANK + FADER BANK chord, favorites,
 * performances, solo, the BT save/copy dialog, the search popup, pad and button LEDs, soft-takeover faders.
 * Layout and flows: DOCs/03-ux-spec.md. Data and routing: lgs_model.h. Storage is RAM-only until T1.9. */
#include "lgs_model.h"
#include "lgs_store.h"

enum { M_PATCH, M_PERF, M_EDIT };
enum { OV_NONE, OV_FAVSAVE, OV_SC, OV_POP, OV_EXIT, OV_CHECK };
#define CHORD_MS 120u
#define HOLD_MS 700u
#define PANEL_MS 700u

static uint8_t mode = M_PATCH, ov;
static uint8_t kb_bank, fb_bank, ps_slot;       /* PATCH bank A-D, PERFORM bank A-H and selected slot (0..13) */
static int8_t cur_slot = -1;                    /* the performance index (bank * 14 + slot) now loaded, -1 = none */
static uint8_t solo, active_line, edit_line, edit_page;
static perf_t cur, edit_bak;
static sound_t pat;                             /* PATCH mode: the sound on part 0 */
static uint8_t pat_cat = 1, pat_idx, pat_pad = 255;
static uint8_t drum_pads, load_from_pad, pad_vel;   /* PATCH with a drum kit: the pads hit the kit (HOME toggles back to favorites) */
static uint8_t ui_dirty = 1;
static int8_t page_btn = -1;                    /* the parameter page open: index into PGBTN, -1 = none */
static uint8_t page_i;
static uint32_t pedal_down, mod_key;

/* ---- text rows ---------------------------------------------------------------------------------- */
#define ROWS 12u
#define COL_WHITE RGB(255, 255, 255)
#define COL_DIM RGB(110, 130, 160)
#define COL_ACC RGB(96, 176, 255)
#define COL_GRN RGB(90, 220, 120)
#define COL_ORG RGB(255, 160, 40)
#define COL_RED RGB(255, 80, 70)
static char row_t[ROWS][40];
static uint16_t row_c[ROWS];

static const char *nb(uint32_t v, uint32_t w)   /* zero-padded number in one of four rotating buffers */
{
    static char b[4][12];
    static uint32_t n;
    char *o = b[n++ & 3u], t[12];
    uint32_t l;
    fmt_u(t, v);
    o[0] = 0;
    for (l = slen(t); l < w; l++) scat(o, "0");
    scat(o, t);
    return o;
}

static void rowf(uint32_t r, uint16_t c, const char *a, const char *b, const char *c2, const char *d)
{
    char *o = row_t[r];
    o[0] = 0;
    row_c[r] = c;
    if (a) scat(o, a);
    if (b) scat(o, b);
    if (c2) scat(o, c2);
    if (d) scat(o, d);
}

static void scr_clear(void) { uint32_t r; for (r = 0; r < ROWS; r++) row_t[r][0] = 0; }

/* theme 0: Classic (plain text on black); theme 1: Studio (SMKSYNTH's blue-black panel: header band, highlighted rows, footer bar) */
static uint8_t theme;
#define TH_BG RGB(14, 18, 28)
#define TH_HEAD RGB(26, 38, 62)
#define TH_SEL RGB(28, 44, 74)
#define TH_FOOT RGB(20, 28, 46)
#define TH_RULE RGB(48, 60, 84)

static uint16_t th_col(uint16_t c)
{
    return c == COL_WHITE ? RGB(226, 232, 240) : c == COL_DIM ? RGB(120, 142, 172) : c == COL_GRN ? RGB(110, 231, 140) : c;
}

static void th_band(uint32_t r, int32_t y)
{
    if (r == 0u) {
        cv_rect(0, y, 240, 19, TH_HEAD);
        cv_rect(0, y + 19, 240, 1, COL_ACC);
    } else if (r == ROWS - 1u) {
        if (row_t[r][0]) { cv_rect(0, y, 240, 20, TH_FOOT); cv_rect(0, y, 240, 1, TH_RULE); }
    } else if (row_t[r][0] && row_c[r] == COL_WHITE) {
        cv_rect(0, y, 240, 20, TH_SEL);
        cv_rect(0, y, 3, 20, COL_ACC);
    }
}

static void scr_flush(void)
{
    uint32_t s, r;
    for (s = 0; s < 2u; s++) {
        cv_begin(240, 120, theme ? TH_BG : C_BLACK);
        for (r = s * 6u; r < s * 6u + 6u; r++) {
            int32_t y = (int32_t)((r - s * 6u) * 20u);
            if (theme) th_band(r, y);
            if (row_t[r][0]) cv_text(6, y + 2, &FONT_S, row_t[r], theme ? th_col(row_c[r]) : row_c[r]);
        }
        cv_blit(0, s * 120u);
    }
}

static char bank_l(uint32_t b) { return (char)('A' + b); }
static const char *bk(uint32_t b) { static char s[2]; s[0] = bank_l(b); s[1] = 0; return s; }
static const char *oct_l(uint32_t o) { static char s[3]; s[0] = 'C'; s[1] = (char)('0' + o); s[2] = 0; return s; }
static const char *pname(uint32_t id) { const lgs_patch_t *p = pat_by_id(id); return p ? p->name : "-"; }
static const char *pshow(const lgs_patch_t *p) { return p->cat == 16u ? p->name : p->name + 4; }   /* the name without its engine prefix (ICONIC has none) */

/* ---- messages and the value bar ----------------------------------------------------------------- */
static char msg[40], bar[40];
static uint16_t msg_c;
static uint32_t msg_until, bar_until;

static void say(const char *s, uint16_t c, uint32_t ms)
{
    uint32_t i;
    for (i = 0; s[i] && i < 39u; i++) msg[i] = s[i];
    msg[i] = 0;
    msg_c = c;
    msg_until = fm1_ms + ms;
    ui_dirty = 1;
}

#define METER_T0 400u                            /* peaks of ui_trk_peak for 1..4 bars */
#define METER_T1 1500u
#define METER_T2 5000u
#define METER_T3 12000u

static uint8_t save_pending;
static const char *save_ok = "Saved successfully";

static void save_request(void)                  /* the data are in db; "Saved" is shown once they are in non-volatile storage */
{
    db_dirty = 1;
    save_pending = 1;
    save_ok = "Saved successfully";
    say("Saving...", COL_ORG, 60000);
}

static void bar_set(const char *name, uint32_t v, uint32_t max, const char *extra)
{
    char *o = bar;
    uint32_t i, n = v * 10u / (max ? max : 1u);
    o[0] = 0;
    scat(o, name);
    scat(o, " ");
    for (i = 0; i < 10u; i++) scat(o, i < n ? "#" : "-");
    scat(o, " ");
    scat(o, nb(v, 0));
    if (extra) scat(o, extra);
    bar_until = fm1_ms + 1500u;
    ui_dirty = 1;
}

/* ---- context: whose K1-K4 and faders ------------------------------------------------------------- */
static sound_t *ctx_s(void) { return mode == M_PATCH ? &pat : &cur.line[mode == M_EDIT ? edit_line : active_line].s; }
static uint32_t ctx_part(void) { return mode == M_PATCH ? 0u : mode == M_EDIT ? edit_line : active_line; }
static uint32_t ctx_vol(void) { return mode == M_PATCH ? 99u : cur.line[ctx_part()].vol; }

/* faders: soft takeover (D-23). After a context change a fader does nothing until it crosses the stored value. */
static const char *const SEND_NAME[4] = {"CHORUS", "DELAY", "DIST", "REVERB"};
static uint8_t fd_last[4], fd_caught[4];
static int8_t fd_side[4];

static void fd_reset(void)
{
    const sound_t *s = ctx_s();
    uint32_t f;
    for (f = 0; f < 4u; f++) {
        fd_side[f] = fd_last[f] > s->snd[f] ? 1 : fd_last[f] < s->snd[f] ? -1 : 0;
        fd_caught[f] = fd_side[f] == 0;             /* already matching: live at once */
    }
}

static void faders_poll(void)
{
    uint32_t f;
    for (f = 0; f < 4u; f++) {
        uint32_t v = fm1_in.fader[f];
        sound_t *s;
        int32_t tgt, sd;
        if (v == fd_last[f]) continue;
        fd_last[f] = (uint8_t)v;
        if (ov == OV_POP || ov == OV_SC || ov == OV_EXIT) continue;
        s = ctx_s();
        tgt = s->snd[f];
        sd = (int32_t)v > tgt ? 1 : (int32_t)v < tgt ? -1 : 0;
        if (!fd_caught[f]) {
            int32_t dd = (int32_t)v - tgt;
            if (sd == 0 || (dd <= 2 && dd >= -2) || (fd_side[f] && sd != fd_side[f])) fd_caught[f] = 1;
            if (sd) fd_side[f] = (int8_t)sd;
        }
        if (fd_caught[f]) {
            s->snd[f] = (uint8_t)v;
            part_levels(ctx_part(), s, ctx_vol());
            bar_set(SEND_NAME[f], v, 127, 0);
        } else {
            char x[16];
            x[0] = 0;
            scat(x, " > ");
            scat(x, nb((uint32_t)tgt, 0));
            bar_set(SEND_NAME[f], v, 127, x);
            ui_dirty = 1;
        }
    }
}

/* ---- loading ------------------------------------------------------------------------------------ */
static void pat_load(const sound_t *s)
{
    page_btn = -1;
    keys_release_all();
    pat = *s;
    drum_pads = !load_from_pad && sound_engine(&pat) == OBV_DRUMS;
    part_apply(0, &pat, 99);
    fd_reset();
    ui_dirty = 1;
}

static void pat_select(void)                    /* the list cursor (pat_cat, pat_idx) -> the sound */
{
    sound_t s;
    memset(&s, 0, sizeof s);
    if (pat_cat == 1u) {
        if (!db.fav[pat_idx].patch) { ui_dirty = 1; return; }
        pat_load(&db.fav[pat_idx]);
        pat_pad = (uint8_t)(pat_idx & 15u);
    } else {
        if (pat_idx >= cat_n[pat_cat]) return;
        sound_defaults(&s, LGS_PATCH[cat_first[pat_cat] + pat_idx].id);
        pat_load(&s);
    }
}

static uint32_t fav_first(uint32_t bank)       /* the first filled favorite of a bank (else its first slot) */
{
    uint32_t i;
    for (i = bank * 16u; i < bank * 16u + 16u; i++) if (db.fav[i].patch) return i;
    return bank * 16u;
}

static uint32_t perf_first(uint32_t bank)      /* the first filled performance of a bank (else the current slot) */
{
    uint32_t s;
    for (s = 0; s < 14u; s++) if (perf_filled(&db.perf[bank * 14u + s])) return s;
    return ps_slot;
}

static void perf_apply_all(void)
{
    page_btn = -1;
    uint32_t i;
    keys_release_all();
    for (i = 0; i < LGS_NLINE; i++)
        if (cur.line[i].s.patch) part_apply(i, &cur.line[i].s, cur.line[i].vol);
    fd_reset();
    ui_dirty = 1;
}

static void perf_select(uint32_t bank, uint32_t slot)
{
    uint32_t idx = bank * 14u + slot;
    fb_bank = (uint8_t)bank;
    ps_slot = (uint8_t)slot;
    ui_dirty = 1;
    if (!perf_filled(&db.perf[idx])) return;
    cur = db.perf[idx];
    cur_slot = (int8_t)idx;
    solo = 0;
    active_line = 0;
    perf_apply_all();
}

static void enter_patch(void)
{
    page_btn = -1;
    keys_release_all();
    mode = M_PATCH; ov = OV_NONE; solo = 0;
    part_apply(0, &pat, 99);
    fd_reset();
    ui_dirty = 1;
}

static void enter_perf(void)
{
    mode = M_PERF; ov = OV_NONE;
    if (cur_slot < 0) perf_select(0, 0); else perf_apply_all();
    ui_dirty = 1;
}

static void enter_edit(void)
{
    page_btn = -1;
    if (mode != M_PERF) enter_perf();
    edit_bak = cur;
    mode = M_EDIT; ov = OV_NONE; solo = 0; edit_line = active_line; edit_page = 0;
    fd_reset();
    ui_dirty = 1;
}

static uint32_t patch_cat_of(uint32_t id) { const lgs_patch_t *p = pat_by_id(id); return p ? p->cat : 2u; }

/* ---- popup (search) and dialogs ------------------------------------------------------------------- */
static uint8_t pop_cat = 2, pop_idx;
static uint8_t sc_bank, sc_slot;
static uint32_t ov_until;
static uint8_t fs_pad;

static uint32_t pop_count(void)
{
    uint32_t i, n = 0;
    if (pop_cat != 1u) return cat_n[pop_cat];
    for (i = 0; i < LGS_NFAV; i++) n += db.fav[i].patch != 0u;
    return n;
}

static uint32_t pop_fav_at(uint32_t k)          /* the k-th non-empty favorite (0-based) -> its number, 64 if none */
{
    uint32_t i;
    for (i = 0; i < LGS_NFAV; i++)
        if (db.fav[i].patch && !k--) return i;
    return 64u;
}

static uint8_t pop_audition;                    /* the edited line's part holds the highlighted sound, not the stored one */

static void pop_preview(void)                   /* the highlighted sound plays on the edited line (undone by pop_restore) */
{
    sound_t s;
    memset(&s, 0, sizeof s);
    if (pop_cat == 1u) {
        uint32_t f = pop_fav_at(pop_idx);
        if (f >= 64u) return;
        s = db.fav[f];
    } else if (pop_idx < cat_n[pop_cat]) sound_defaults(&s, LGS_PATCH[cat_first[pop_cat] + pop_idx].id);
    else return;
    keys_release_all();
    part_apply(edit_line, &s, cur.line[edit_line].vol);
    fd_reset();
    pop_audition = 1;
}

static void pop_restore(void)
{
    pop_audition = 0;
    keys_release_all();
    if (cur.line[edit_line].s.patch) part_apply(edit_line, &cur.line[edit_line].s, cur.line[edit_line].vol);
}

static void pop_open(void)
{
    uint32_t c = patch_cat_of(cur.line[edit_line].s.patch), i;
    pop_cat = (uint8_t)(c ? c : 2u);
    pop_idx = 0;
    for (i = 0; c && i < cat_n[c]; i++)             /* start on the line's own patch: nothing changes until you move */
        if (LGS_PATCH[cat_first[c] + i].id == cur.line[edit_line].s.patch) pop_idx = (uint8_t)i;
    pop_audition = 0;
    ov = OV_POP;
    ov_until = 0;
    ui_dirty = 1;
}

static void edit_set_line(uint32_t l, const sound_t *s, uint32_t origin)
{
    cur.line[l].s = *s;
    cur.line[l].origin = (uint8_t)origin;
    keys_release_all();
    part_apply(l, s, cur.line[l].vol);
    fd_reset();
    ui_dirty = 1;
}

static void edit_set_patch(uint32_t l, uint32_t id)
{
    sound_t s;
    memset(&s, 0, sizeof s);
    sound_defaults(&s, id);
    edit_set_line(l, &s, 0);
}

static void pop_confirm(void)
{
    pop_audition = 0;
    if (pop_cat == 1u) {
        uint32_t f = pop_fav_at(pop_idx);
        if (f < 64u) edit_set_line(edit_line, &db.fav[f], f + 1u);
    } else if (pop_idx < cat_n[pop_cat]) {
        edit_set_patch(edit_line, LGS_PATCH[cat_first[pop_cat] + pop_idx].id);
    }
    ov = OV_NONE;
}

static void sc_open(void)
{
    if (mode == M_PATCH) { sc_bank = kb_bank; sc_slot = pat_pad < 16u ? pat_pad : 0u; }
    else { sc_bank = cur_slot >= 0 ? (uint8_t)(cur_slot / 14) : fb_bank; sc_slot = cur_slot >= 0 ? (uint8_t)(cur_slot % 14) : ps_slot; }
    ov = OV_SC;
    ov_until = fm1_ms + 10000u;
    ui_dirty = 1;
}

static void edit_leave(void)                    /* back to PERFORM, performance already stored or restored */
{
    mode = M_PERF; ov = OV_NONE;
    perf_apply_all();
}

static void sc_confirm(void)
{
    if (mode == M_PATCH) {
        db.fav[sc_bank * 16u + sc_slot] = pat;
    } else {
        uint32_t idx = sc_bank * 14u + sc_slot;
        db.perf[idx] = cur;
        cur_slot = (int8_t)idx;
        fb_bank = sc_bank;
        ps_slot = sc_slot;
    }
    ov = OV_NONE;
    if (mode == M_EDIT) edit_leave();
    save_request();
}

/* ---- knobs --------------------------------------------------------------------------------------- */
static void tweak(uint32_t i, int32_t d)
{
    sound_t *s = ctx_s();
    knob_t k = knob_desc(sound_engine(s), i);
    s->tw[i] = (uint8_t)clampi((int32_t)s->tw[i] + d * (k.range > 20u ? 2 : 1), 0, k.range);
    live_p[ctx_part()].v[k.slot] = s->tw[i];
    bar_set(k.name, s->tw[i], k.range, 0);
}

static int32_t wrap(int32_t v, int32_t n) { return n > 0 ? ((v % n) + n) % n : 0; }

static void tempo_knob(int32_t d)
{
    ctl_tempo = (uint32_t)clampi((int32_t)ctl_tempo + d, 40, 240);
    bar_set("TEMPO", ctl_tempo - 40u, 200, 0);
    ui_dirty = 1;
}

static void pat_nav(int32_t dc, int32_t di)
{
    uint32_t n;
    if (dc) { pat_cat = (uint8_t)(1 + wrap((int32_t)pat_cat - 1 + dc, 16)); pat_idx = (uint8_t)(pat_cat == 1u ? fav_first(kb_bank) : 0u); }
    n = pat_cat == 1u ? LGS_NFAV : cat_n[pat_cat];
    if (di) pat_idx = (uint8_t)wrap((int32_t)pat_idx + di, (int32_t)n);
    if (pat_cat == 1u) kb_bank = (uint8_t)(pat_idx >> 4);     /* the bank letter follows the favorites list */
    pat_select();
    ui_dirty = 1;
}

static void edit_knobs(uint32_t k, int32_t d)
{
    lgline_t *l = &cur.line[edit_line];
    if (k == LGS_K1) { edit_line = (uint8_t)clampi((int32_t)edit_line + d, 0, 3); fd_reset(); }
    else if (!edit_page) {
        if (k == LGS_K2) {
            uint32_t idx = l->s.patch ? pat_index(l->s.patch) : 0u;
            if (l->s.patch) idx = (uint32_t)wrap((int32_t)idx + d, (int32_t)LGS_NPATCH);
            edit_set_patch(edit_line, LGS_PATCH[idx].id);
        } else if (k == LGS_K3 && edit_line < 2u) { l->lo = (uint8_t)clampi((int32_t)l->lo + d, 1, 7); if (l->hi < l->lo) l->hi = l->lo; }
        else if (k == LGS_K4 && edit_line < 2u) { l->hi = (uint8_t)clampi((int32_t)l->hi + d, 1, 7); if (l->lo > l->hi) l->lo = l->hi; }
        else if (k == LGS_K5) { l->vol = (uint8_t)clampi((int32_t)l->vol + d, 0, 99); if (l->s.patch) part_levels(edit_line, &l->s, l->vol); }
    } else {
        if (k == LGS_K5) l->flags ^= LF_SUSTAIN;
        else if (k == LGS_K6) l->flags ^= LF_BEND;
        else if (k == LGS_K7) {
            int32_t m = (int32_t)(l->flags >> LF_MOD_SHIFT & 7u) + d;
            m = clampi(m, 0, (int32_t)MOD_N - 1);
            l->flags = (uint8_t)((l->flags & 3u) | (uint32_t)m << LF_MOD_SHIFT);
        }
    }
    ui_dirty = 1;
}

/* ---- parameter pages (ARP, ENV, LFO, FILTER ... on their buttons; FX = the global effect units) ---------------------- */
enum { PGX_DRIVE = NPAGE };                     /* the global DRIVE send: type, amount, tone (no patch parameter) */
typedef struct { uint8_t btn; uint8_t n; uint8_t pg[5]; } pgbtn_t;
static const pgbtn_t PGBTN[] = {
    {BTN_ARP, 1, {PG_ARP}},
    {BTN_LFO, 2, {PG_LFO, PG_LFODEST}},
    {BTN_SEL, 5, {PG_OSC1, PG_OSC2, PG_OSCMOD, PG_FILTER, PG_FILTER2}},
    {BTN_ENV, 2, {PG_AENV, PG_FENV}},
    {BTN_GLO, 2, {PG_VOICE, PG_OUTPUT}},
    {BTN_FX, 4, {PG_CHORUS, PG_DELAY, PG_REVERB, PGX_DRIVE}},
};

static uint32_t page_id(void) { return PGBTN[page_btn].pg[page_i]; }
static uint32_t page_is_fx(uint32_t pg) { return pg == PG_CHORUS || pg == PG_DELAY || pg == PG_REVERB || pg == PGX_DRIVE; }

static int page_allowed(uint32_t pg)            /* the borrowed analog slots mean something else under the other engines */
{
    uint32_t e = sound_engine(ctx_s());
    if (page_is_fx(pg)) return 1;
    if (e == OBV_DRUMS) return 0;
    return e == OBV_ANALOG || pg == PG_ARP || pg == PG_AENV;
}

static int page_param(uint32_t pg, uint32_t k)  /* the parameter of knob k on a page, -1 none (MIX of the effects is fixed) */
{
    int p;
    if (pg == PGX_DRIVE) return k < 3u ? 100 + (int)k : -1;
    p = PAGE[pg].p[k];
    return p == P_CHO_MIX || p == P_DLY_MIX || p == P_REV_MIX ? -1 : p;
}

static void page_value_text(char *o, int p, uint32_t v)
{
    o[0] = 0;
    if (p == 100) { static const char *const T[4] = {"OVERDRIVE", "DIST", "FUZZ", "CRUSH"}; scat(o, T[v & 3u]); return; }
    if (p >= 100) { scat(o, nb(v, 0)); return; }
    if (PDEF[p].kind == K_ARP) scat(o, ARP_NAME[v % NARPMODE]);
    else if (PDEF[p].kind == K_DIV) scat(o, DIV_NAME[v % NDIV]);
    else if (PDEF[p].kind == K_ONOFF) scat(o, v ? "ON" : "OFF");
    else scat(o, nb(v, 0));
}

static uint32_t page_get(int p)
{
    if (p == 100) return ctl_dv_type;
    if (p == 101) return ctl_dv_amt;
    if (p == 102) return ctl_dv_tone;
    if ((uint32_t)p >= P_CHO_RATE && (uint32_t)p <= P_REV_MIX) return fx_g[(uint32_t)p - P_CHO_RATE];
    return live_p[ctx_part()].v[p];
}

static void page_set(int p, uint32_t v)
{
    if (p == 100) ctl_dv_type = (uint8_t)v;
    else if (p == 101) ctl_dv_amt = (uint8_t)v;
    else if (p == 102) ctl_dv_tone = (uint8_t)v;
    else if ((uint32_t)p >= P_CHO_RATE && (uint32_t)p <= P_REV_MIX) {
        fx_g[(uint32_t)p - P_CHO_RATE] = (uint8_t)v;
        live_p[0].v[p] = (uint8_t)v;
    } else {
        sound_t *s = ctx_s();
        uint32_t i;
        live_p[ctx_part()].v[p] = (uint8_t)v;
        for (i = 0; i < 4u; i++)                        /* a parameter that is also one of K1-K4 keeps them in step */
            if (knob_desc(sound_engine(s), i).slot == (uint32_t)p) s->tw[i] = (uint8_t)v;
    }
}

static void page_knob(uint32_t k, int32_t d)
{
    uint32_t pg = page_id();
    int p = page_param(pg, k);
    uint32_t mx;
    if (p < 0) return;
    mx = p == 100 ? 3u : p >= 100 ? 127u : PDEF[p].max;
    page_set(p, (uint32_t)clampi((int32_t)page_get(p) + d * (mx > 32u ? 2 : 1), 0, (int32_t)mx));
    ui_dirty = 1;
}

static void page_press(uint32_t b)              /* a page button: first press opens, further presses step through the pages, the last leaves */
{
    uint32_t i;
    if (ov != OV_NONE || mode == M_EDIT) return;
    for (i = 0; i < sizeof PGBTN / sizeof PGBTN[0]; i++) {
        if (PGBTN[i].btn != b) continue;
        if (page_btn == (int8_t)i) {
            if (++page_i >= PGBTN[i].n) page_btn = -1;
        } else { page_btn = (int8_t)i; page_i = 0; }
        while (page_btn >= 0 && !page_allowed(page_id())) {          /* skip pages this engine cannot use */
            if (++page_i >= PGBTN[page_btn].n) { page_btn = -1; say("Not for this engine", COL_DIM, 1500); }
        }
        ui_dirty = 1;
        return;
    }
}

static void knobs_poll(void)
{
    uint32_t k;
    int32_t d[7];
    for (k = 0; k < 7u; k++) d[k] = lgs_knob(k);
    if (ov == OV_POP) {
        if (d[0]) { uint32_t n = pop_count(); pop_idx = (uint8_t)wrap((int32_t)pop_idx + d[0], (int32_t)n); pop_preview(); ui_dirty = 1; }
        return;
    }
    if (ov == OV_SC) {
        uint32_t nb_ = mode == M_PATCH ? 4u : 8u, ns = mode == M_PATCH ? 16u : 14u;
        if (d[5]) { sc_bank = (uint8_t)wrap((int32_t)sc_bank + d[5], (int32_t)nb_); ui_dirty = 1; }
        if (d[6]) { sc_slot = (uint8_t)wrap((int32_t)sc_slot + d[6], (int32_t)ns); ui_dirty = 1; }
        return;
    }
    if (ov != OV_NONE) return;
    if (mode == M_EDIT) {
        for (k = 0; k < 7u; k++) if (d[k]) edit_knobs(k, d[k]);
        return;
    }
    if (page_btn >= 0) { for (k = 0; k < 4u; k++) if (d[k]) page_knob(k, d[k]); }
    else for (k = 0; k < 4u; k++) if (d[k]) tweak(k, d[k]);
    if (d[4]) tempo_knob(d[4]);
    if (mode == M_PATCH) {
        if (d[5] || d[6]) pat_nav(d[5], d[6]);
    } else {
        if (d[5]) { perf_select((uint32_t)wrap((int32_t)fb_bank + d[5], 8), ps_slot); }
        if (d[6]) { perf_select(fb_bank, (uint32_t)wrap((int32_t)ps_slot + d[6], 14)); }
    }
}

/* ---- keys --------------------------------------------------------------------------------------- */
static uint8_t kb_ext;                          /* the note comes from MIDI: no OCTAVE shift */

static void kb_note(uint32_t on, uint32_t key, uint32_t vel)
{
    uint32_t n, m, p;
    if (key >= 128u) return;
    if (on) {
        n = (uint32_t)clampi((int32_t)key + (kb_ext ? 0 : oct_shift * 12), 0, 127);
        m = mode == M_PATCH ? 1u : ov == OV_POP && mode == M_EDIT ? 1u << edit_line : route_mask(&cur, solo, n);   /* the popup auditions on the edited line, whatever its range */
        if (held_mask[key]) return;
        held_mask[key] = (uint8_t)m;
        held_note[key] = (uint8_t)n;
        for (p = 0; p < NPART; p++)
            if (m >> p & 1u) note_event_p(p, 1, n, vel);
        if (m && mode != M_PATCH) {
            for (p = 0; p < NPART && !(m >> p & 1u); p++) ;
            if (active_line != p && !(mode == M_EDIT)) { active_line = (uint8_t)p; fd_reset(); ui_dirty = 1; }
        }
    } else if (held_mask[key]) {
        for (p = 0; p < NPART; p++)
            if (held_mask[key] >> p & 1u) note_event_p(p, 0, held_note[key], 0);
        held_mask[key] = 0;
    }
}

/* ---- pads --------------------------------------------------------------------------------------- */
static uint32_t pad_t[16];
static uint16_t pad_dn;

static void solo_press(uint32_t s)
{
    if (solo == s) solo = 0;
    else if (cur.line[1u + s].s.patch) solo = (uint8_t)s;
    else return;
    keys_release_all();
    if (solo) { active_line = (uint8_t)(1u + solo); fd_reset(); }
    ui_dirty = 1;
}

static void pad_press(uint32_t p)
{
    if (ov == OV_POP || ov == OV_SC || ov == OV_EXIT) {
        if (ov == OV_POP) { if (p == 0u) pop_confirm(); else if (p == 1u) ov = OV_NONE; }
        else if (ov == OV_SC) { if (p == 0u) sc_confirm(); else if (p == 1u) ov = OV_NONE; }
        else if (p == 0u) {                                     /* EXIT prompt: save */
            if (cur_slot < 0) cur_slot = 0;
            db.perf[(uint32_t)cur_slot] = cur;
            edit_leave();
            save_request();
        } else if (p == 1u) { cur = edit_bak; edit_leave(); }  /* discard */
        ui_dirty = 1;
        return;
    }
    if (ov == OV_FAVSAVE) {
        if (p == fs_pad) {
            db.fav[kb_bank * 16u + p] = pat;
            save_request();
        }
        ov = OV_NONE;
        ui_dirty = 1;
        return;
    }
    if (mode == M_PATCH && drum_pads) {
        note_event_p(0, 1, DRUM_NOTE[p & 7u], pad_vel);
    } else if (mode == M_PATCH) {
        pad_dn |= (uint16_t)(1u << p);
        pad_t[p] = fm1_ms;
    } else if (mode == M_PERF) {
        if (p == 7u) solo_press(1u);
        else if (p == 15u) solo_press(2u);
        else if (solo) { solo = 0; keys_release_all(); ui_dirty = 1; say("Solo off - tap again to load", COL_ORG, 1500); }
        else perf_select(fb_bank, p < 7u ? p : p - 1u);
    } else if (p < 16u) {                                       /* EDIT: category quick pick (pad 16 = ICONIC) */
        uint32_t c = p + 1u;
        if (c == 1u) {
            uint32_t f = pop_fav_at(0);
            if (f < 64u) edit_set_line(edit_line, &db.fav[f], f + 1u);
        } else if (cat_n[c]) edit_set_patch(edit_line, LGS_PATCH[cat_first[c]].id);
    }
}

static void pad_release(uint32_t p)
{
    if (mode == M_PATCH && ov == OV_NONE && (pad_dn >> p & 1u)) {      /* a tap: load the favorite */
        pad_dn &= (uint16_t)~(1u << p);
        pat_cat = 1;
        pat_idx = (uint8_t)(kb_bank * 16u + p);
        load_from_pad = 1;
        pat_select();
        load_from_pad = 0;
    }
}

static void pads_hold_poll(void)
{
    uint32_t p;
    if (mode != M_PATCH || ov != OV_NONE) return;
    for (p = 0; p < 16u; p++)
        if ((pad_dn >> p & 1u) && fm1_ms - pad_t[p] >= HOLD_MS) {
            pad_dn &= (uint16_t)~(1u << p);
            fs_pad = (uint8_t)p;
            ov = OV_FAVSAVE;
            ov_until = fm1_ms + 5000u;
            ui_dirty = 1;
        }
}

/* ---- buttons ------------------------------------------------------------------------------------ */
static uint32_t kb_t, fb_t;
static uint8_t kb_dn, fb_dn, kb_sup, fb_sup;

static void chord_action(void)
{
    if (mode == M_EDIT) { if (ov == OV_POP) ov = OV_NONE; else if (ov == OV_NONE) pop_open(); }
    else if (ov == OV_NONE) enter_edit();
    ui_dirty = 1;
}

static void kb_single(void)
{
    if (ov != OV_NONE) { ov = OV_NONE; ui_dirty = 1; return; }
    if (mode == M_EDIT) { ov = OV_EXIT; ov_until = fm1_ms + 10000u; }
    else if (mode != M_PATCH) enter_patch();
    else {                                       /* next bank: the list jumps to its first favorite and loads it */
        kb_bank = (uint8_t)((kb_bank + 1u) & 3u);
        pat_cat = 1;
        pat_idx = (uint8_t)fav_first(kb_bank);
        pat_select();
    }
    ui_dirty = 1;
}

static void fb_single(void)
{
    if (ov != OV_NONE) { ov = OV_NONE; ui_dirty = 1; return; }
    if (mode == M_EDIT) { ov = OV_EXIT; ov_until = fm1_ms + 10000u; }
    else if (mode != M_PERF) enter_perf();
    else { uint32_t b = (fb_bank + 1u) & 7u; perf_select(b, perf_first(b)); }   /* next bank: its first performance */
    ui_dirty = 1;
}

static void arrows(int32_t d)
{
    if (mode == M_EDIT) {
        if (ov == OV_NONE) pop_open();
        if (ov == OV_POP) { pop_cat = (uint8_t)(1 + wrap((int32_t)pop_cat - 1 + d, 16)); pop_idx = 0; pop_preview(); }
    } else if (ov == OV_NONE) {
        if (mode == M_PATCH) pat_nav(d, 0);
        else perf_select((uint32_t)wrap((int32_t)fb_bank + d, 8), ps_slot);
    }
    ui_dirty = 1;
}

static void buttons_poll(void)
{
    uint32_t rel, e = fm1_input_edges(&rel);
    if (e >> BTN_KNOB_BANK & 1u) { kb_dn = 1; kb_t = fm1_ms; if (fb_dn && fm1_ms - fb_t <= CHORD_MS) { kb_sup = fb_sup = 1; chord_action(); } }
    if (e >> BTN_FADER_BANK & 1u) { fb_dn = 1; fb_t = fm1_ms; if (kb_dn && fm1_ms - kb_t <= CHORD_MS) { kb_sup = fb_sup = 1; chord_action(); } }
    if (rel >> BTN_KNOB_BANK & 1u) { if (kb_dn && !kb_sup) kb_single(); kb_dn = 0; if (!fb_dn) kb_sup = fb_sup = 0; }
    if (rel >> BTN_FADER_BANK & 1u) { if (fb_dn && !fb_sup) fb_single(); fb_dn = 0; if (!kb_dn) kb_sup = fb_sup = 0; }
    if (kb_dn && !kb_sup && !fb_dn && ov == OV_NONE && fm1_ms - kb_t >= PANEL_MS) { ov = OV_CHECK; kb_sup = 1; ui_dirty = 1; }   /* KNOB BANK held: panel check */
    { static const uint8_t PB[6] = {BTN_ARP, BTN_LFO, BTN_SEL, BTN_ENV, BTN_GLO, BTN_FX}; uint32_t q; for (q = 0; q < 6u; q++) if (e >> PB[q] & 1u) page_press(PB[q]); }
    if (e >> BTN_LEFT & 1u) arrows(-1);
    if (e >> BTN_RIGHT & 1u) arrows(1);
    if ((e >> BTN_OCT_DOWN & 1u) || (e >> BTN_OCT_UP & 1u)) {
        keys_release_all();
        if ((e >> BTN_OCT_DOWN & 1u) && (e >> BTN_OCT_UP & 1u)) oct_shift = 0;
        else oct_shift = (int8_t)clampi(oct_shift + ((e >> BTN_OCT_UP & 1u) ? 1 : -1), -2, 2);
        bar_set("OCTAVE", (uint32_t)(oct_shift + 2), 4, 0);
    }
    if (e >> BTN_SAVE & 1u) { if (ov == OV_NONE) sc_open(); else if (ov == OV_SC) { ov = OV_NONE; ui_dirty = 1; } }
    if ((e >> BTN_HOME & 1u) && mode == M_PATCH && ov == OV_NONE) {
        if (sound_engine(&pat) != OBV_DRUMS) say("Not a drum kit", COL_DIM, 1500);
        else {
            drum_pads = !drum_pads;
            say(drum_pads ? "Pads: drum hits" : "Pads: favorites", COL_ACC, 1500);
            ui_dirty = 1;
        }
    }
    if ((e >> BTN_EDIT & 1u) && mode == M_EDIT && ov == OV_NONE) { edit_page ^= 1u; ui_dirty = 1; }
    if ((e >> BTN_SEQ & 1u) && ov == OV_NONE) {            /* the screen theme, kept across power cycles */
        theme ^= 1u;
        db.theme = theme;
        save_request();
        save_ok = theme ? "Theme: Studio (saved)" : "Theme: Classic (saved)";
        ui_dirty = 1;
    }
}

static void ov_timers(void)
{
    if (ov == OV_CHECK) {                       /* live values while KNOB BANK stays down */
        static uint32_t last;
        if (!kb_dn) { ov = OV_NONE; ui_dirty = 1; }
        else if (fm1_ms - last >= 100u) { last = fm1_ms; ui_dirty = 1; }
    } else if (ov != OV_NONE && ov != OV_POP && (int32_t)(fm1_ms - ov_until) >= 0) { ov = OV_NONE; ui_dirty = 1; }
    if (msg_until && (int32_t)(fm1_ms - msg_until) >= 0) { msg_until = 0; msg[0] = 0; ui_dirty = 1; }
    if (bar_until && (int32_t)(fm1_ms - bar_until) >= 0) { bar_until = 0; bar[0] = 0; ui_dirty = 1; }
}

/* ---- screens ------------------------------------------------------------------------------------ */
static void line_text(char *o, uint32_t i, const lgline_t *l)
{
    static const char *const LB[4] = {"L1 ", "L2 ", "S1 ", "S2 "};
    o[0] = 0;
    scat(o, LB[i]);
    if (!l->s.patch) { scat(o, "---"); return; }
    if (l->origin) { scat(o, "F"); scat(o, nb(l->origin, 3)); }
    else scat(o, nb(l->s.patch, 3));
    scat(o, " ");
    { const lgs_patch_t *pp = pat_by_id(l->s.patch); scat(o, pp ? pshow(pp) : "-"); }
    if (i >= 2u) return;                        /* solos have no range */
    scat(o, " ");
    scat(o, oct_l(l->lo));
    scat(o, "-");
    scat(o, oct_l(l->hi));
}

static void rows_tweaks(void)
{
    const sound_t *s = ctx_s();
    uint32_t i;
    for (i = 0; i < 2u; i++) {
        char *o = row_t[9u + i];
        o[0] = 0;
        scat(o, knob_desc(sound_engine(s), i * 2u).name); scat(o, " "); scat(o, nb(s->tw[i * 2u], 0));
        scat(o, "   ");
        scat(o, knob_desc(sound_engine(s), i * 2u + 1u).name); scat(o, " "); scat(o, nb(s->tw[i * 2u + 1u], 0));
        row_c[9u + i] = COL_DIM;
    }
}

static const char *engine_label(uint32_t e)
{
    return e == OBV_DRUMS ? "Drum Engine" : e == OBV_FM2 ? "4-Op FM Engine" : e == OBV_ORG ? "Organ Engine" : e == OBV_SP ? "Sample Engine" : e == OBV_FM6 ? "6-Op FM Engine" : "Synth Engine";
}

static void rows_footer(void)
{
    char *o = row_t[11];
    o[0] = 0;
    scat(o, engine_label(sound_engine(ctx_s())));
    scat(o, "   ");
    scat(o, nb(ctl_tempo, 0));
    scat(o, " BPM");
    row_c[11] = COL_DIM;
    if (msg[0]) { rowf(8, msg_c, msg, 0, 0, 0); }
    else if (bar[0]) { rowf(8, COL_ACC, bar, 0, 0, 0); }
    else if (mode == M_PATCH) { if (drum_pads) rowf(8, COL_DIM, "Pads play the drum kit", 0, 0, 0); }
}

static void screen_patch(void)
{
    uint32_t n = pat_cat == 1u ? LGS_NFAV : cat_n[pat_cat], first, i;
    const lgs_patch_t *cp = pat_by_id(pat.patch);
    if (drum_pads) rowf(0, COL_WHITE, "[PATCH] Drum pads", 0, 0, 0);
    else rowf(0, COL_WHITE, "[PATCH] Bank ", bk(kb_bank), pat_pad < 16u ? " - " : 0, pat_pad < 16u ? nb(pat_pad + 1u, 0) : 0);
    if (cp) rowf(1, COL_ACC, LGS_CAT_NAME[cp->cat], " - ", nb(cp->id, 3), "");
    if (cp) { scat(row_t[1], " - "); scat(row_t[1], pshow(cp)); }
    first = pat_idx < 2u ? 0u : pat_idx - 2u;
    if (n > 5u && first > n - 5u) first = n - 5u;
    rowf(2, COL_DIM, LGS_CAT_NAME[pat_cat], " (", nb(n, 0), ")");
    for (i = 0; i < 5u && first + i < n; i++) {
        uint32_t k = first + i;
        char *o = row_t[3u + i];
        o[0] = 0;
        scat(o, k == pat_idx ? "> " : "  ");
        if (pat_cat == 1u) {
            scat(o, "F"); scat(o, nb(k + 1u, 3)); scat(o, " ");
            scat(o, db.fav[k].patch ? pname(db.fav[k].patch) : "-");
        } else {
            scat(o, nb(LGS_PATCH[cat_first[pat_cat] + k].id, 3)); scat(o, " ");
            scat(o, LGS_PATCH[cat_first[pat_cat] + k].name);
        }
        row_c[3u + i] = k == pat_idx ? COL_WHITE : COL_DIM;
    }
    rows_tweaks();
}

static uint8_t line_meter[LGS_NLINE];           /* 0..4: how loud each line is sounding (PERFORM) */
static uint32_t meter_t;

static void meters_poll(void)
{
    uint32_t i;
    if (mode != M_PERF || ov != OV_NONE || fm1_ms - meter_t < 80u) return;
    meter_t = fm1_ms;
    for (i = 0; i < LGS_NLINE; i++) {
        uint32_t pk = ui_trk_peak[i], n = pk > METER_T3 ? 4u : pk > METER_T2 ? 3u : pk > METER_T1 ? 2u : pk > METER_T0 ? 1u : 0u;
        ui_trk_peak[i] = 0;
        if (n != line_meter[i]) { line_meter[i] = (uint8_t)n; ui_dirty = 1; }
    }
}

static void screen_perf(void)
{
    uint32_t i, idx = (uint32_t)fb_bank * 14u + ps_slot;
    rowf(0, COL_WHITE, "[PERFORM] Bank ", bk(fb_bank), " - ", nb(ps_slot + 1u, 0));
    if (!perf_filled(&db.perf[idx])) {
        if (cur_slot >= 0) rowf(1, COL_RED, "(empty slot) now: ", bk((uint32_t)cur_slot / 14u), "-", nb((uint32_t)cur_slot % 14u + 1u, 0));
        else rowf(1, COL_RED, "(empty slot)", 0, 0, 0);
    }
    else if ((int32_t)idx != cur_slot) rowf(1, COL_ORG, "loading...", 0, 0, 0);
    else if (solo) rowf(1, COL_ORG, "SOLO ", nb(solo, 0), 0, 0);
    for (i = 0; i < LGS_NLINE; i++) {
        char *o = row_t[3u + i];
        o[0] = 0;
        o[0] = i == active_line ? '>' : ' ';
        o[1] = cur.line[i].s.patch && line_meter[i] ? ".:+#"[line_meter[i] - 1u] : ' ';   /* how loud the line sounds */
        o[2] = ' '; o[3] = 0;
        line_text(o + 3, i, &cur.line[i]);
        row_c[3u + i] = (solo ? (i == 1u + solo) : (i < 2u)) ? COL_WHITE : COL_DIM;
        if (solo && i == 1u + solo) row_c[3u + i] = COL_ORG;
    }
    rows_tweaks();
}

static void screen_edit(void)
{
    const lgline_t *l = &cur.line[edit_line];
    static const char *const LN[4] = {"LAYER 1", "LAYER 2", "SOLO 1", "SOLO 2"};
    static const char *const MODN[MOD_N] = {"DEFAULT", "OFF", "FILTER", "BRIGHT", "SWELL", "REVERB"};
    uint32_t i;
    rowf(0, COL_WHITE, "[EDIT] ", bk(fb_bank), nb(ps_slot + 1u, 0), edit_page ? "  page 2/2" : "  page 1/2");
    rowf(1, COL_DIM, edit_page ? "K5 sustain K6 bend K7 mod" : edit_line < 2u ? "K2 patch K3/K4 range K5 vol" : "K1 line K2 patch K5 vol", 0, 0, 0);
    rowf(2, COL_ACC, "K1 Line: ", LN[edit_line], 0, 0);
    if (!edit_page) {
        rowf(3, COL_ACC, "K2 Patch: ", l->s.patch ? nb(l->s.patch, 3) : "---", " ", l->s.patch ? pname(l->s.patch) : "");
        if (edit_line < 2u) {
            rowf(4, COL_ACC, "K3 Low octave: ", oct_l(l->lo), 0, 0);
            rowf(5, COL_ACC, "K4 High octave: ", oct_l(l->hi), 0, 0);
        } else rowf(4, COL_DIM, "Solo: whole keyboard", 0, 0, 0);
        rowf(6, COL_ACC, "K5 Volume: ", nb(l->vol, 0), 0, 0);
    } else {
        rowf(3, COL_ACC, "K5 Sustain: ", l->flags & LF_SUSTAIN ? "YES" : "NO", 0, 0);
        rowf(4, COL_ACC, "K6 Bend: ", l->flags & LF_BEND ? "YES" : "NO", 0, 0);
        rowf(5, COL_ACC, "K7 Mod: ", MODN[l->flags >> LF_MOD_SHIFT & 7u], 0, 0);
    }
    for (i = 0; i < LGS_NLINE; i++) {
        char *o = row_t[7u + i];
        o[0] = 0;
        scat(o, i == edit_line ? "> " : "  ");
        line_text(o + 2, i, &cur.line[i]);
        row_c[7u + i] = i == edit_line ? COL_WHITE : COL_DIM;
    }
}

static void screen_popup(void)
{
    uint32_t n = pop_count(), first, i;
    rowf(0, COL_WHITE, "SEARCH  ", LGS_CAT_NAME[pop_cat], " (", nb(n, 0));
    scat(row_t[0], ")");
    first = pop_idx < 2u ? 0u : pop_idx - 2u;
    if (n > 7u && first > n - 7u) first = n - 7u;
    for (i = 0; i < 7u && first + i < n; i++) {
        uint32_t k = first + i;
        char *o = row_t[2u + i];
        o[0] = 0;
        scat(o, k == pop_idx ? "> " : "  ");
        if (pop_cat == 1u) { uint32_t f = pop_fav_at(k); scat(o, "F"); scat(o, nb(f + 1u, 3)); scat(o, " "); scat(o, pname(db.fav[f].patch)); }
        else { scat(o, nb(LGS_PATCH[cat_first[pop_cat] + k].id, 3)); scat(o, " "); scat(o, LGS_PATCH[cat_first[pop_cat] + k].name); }
        row_c[2u + i] = k == pop_idx ? COL_WHITE : COL_DIM;
    }
    if (!n) rowf(3, COL_DIM, "(empty)", 0, 0, 0);
    rowf(10, COL_ACC, "</>: category  K1: scroll", 0, 0, 0);
    rowf(11, COL_ACC, "Pad1 OK    Pad2 cancel", 0, 0, 0);
}

static void screen_sc(void)
{
    char *o;
    rowf(0, COL_WHITE, mode == M_PATCH ? "SAVE FAVORITE" : "SAVE PERFORMANCE", 0, 0, 0);
    rowf(2, COL_ACC, "K6 Bank: ", bk(sc_bank), 0, 0);
    rowf(3, COL_ACC, "K7 Slot: ", nb(sc_slot + 1u, 0), 0, 0);
    o = row_t[5];
    o[0] = 0;
    row_c[5] = COL_ORG;
    if (mode == M_PATCH) {
        const sound_t *d = &db.fav[sc_bank * 16u + sc_slot];
        if (d->patch) { scat(o, "Overwrites: "); scat(o, pname(d->patch)); } else scat(o, "Empty slot");
    } else {
        const perf_t *d = &db.perf[sc_bank * 14u + sc_slot];
        if (perf_filled(d)) { scat(o, "Overwrites: "); scat(o, pname(d->line[0].s.patch)); scat(o, " ..."); } else scat(o, "Empty slot");
    }
    rowf(10, COL_ACC, "Pad1 OK    Pad2 cancel", 0, 0, 0);
}

static void screen_exit(void)
{
    rowf(2, COL_WHITE, "Leave EDIT?", 0, 0, 0);
    rowf(4, COL_ACC, "Pad 1 (blue): SAVE", 0, 0, 0);
    rowf(5, COL_RED, "Pad 2 (red): DISCARD", 0, 0, 0);
    rowf(7, COL_DIM, "(10 s: stay in EDIT)", 0, 0, 0);
}

static void screen_favsave(void)
{
    uint32_t f = kb_bank * 16u + fs_pad;
    const lgs_patch_t *old = db.fav[f].patch ? pat_by_id(db.fav[f].patch) : 0, *now = pat_by_id(pat.patch);
    rowf(1, COL_WHITE, old ? "Overwrite favorite " : "Save to favorite ", nb(f + 1u, 0), "?", 0);
    if (old) rowf(3, COL_DIM, "now:  ", pshow(old), 0, 0);
    rowf(4, COL_ACC, "with: ", now ? pshow(now) : "-", 0, 0);
    rowf(6, COL_WHITE, "Press the same pad to save", 0, 0, 0);
    rowf(7, COL_DIM, "(5 s: cancel)", 0, 0, 0);
}

#ifndef OBV_HOST
/* Stack high-water marks for the panel check: the unused part of each stack is painted at start-up (interrupts off, below the
 * live stack pointer) and the lowest word that no longer holds the pattern is the deepest use. */
extern char _ustack_top[];                      /* app.ld (fm1_guard.h declares the others) */
#define STACK_PAT 0xA5A5A5A5u
#define STACK_KEEP 1024u                       /* the top of the stack is not painted: it may be in use right now */
static void stack_paint(void)
{
    char here;
    uint32_t *p;
    fm1_irq_off();
    for (p = (uint32_t *)(void *)_sstack_lo; (char *)p < _sstack_top - STACK_KEEP; p++) *p = STACK_PAT;
    for (p = (uint32_t *)(void *)_ustack_lo; (char *)p < &here - STACK_KEEP; p++) *p = STACK_PAT;
    fm1_irq_on();
}
static uint32_t stack_used(const char *lo, const char *top)    /* bytes from the top to the deepest word touched */
{
    const uint32_t *p = (const uint32_t *)(const void *)lo;
    while ((const char *)p < top && *p == STACK_PAT) p++;
    return (uint32_t)(top - (const char *)p);
}
#endif

static const char *hex(uint32_t v)
{
    static char b[12];
    uint32_t i;
    for (i = 0; i < 8u; i++) b[i] = "0123456789ABCDEF"[v >> (28u - 4u * i) & 15u];
    b[8] = 0;
    return b + (v > 0xFFFFu ? 0 : 4);
}

static void screen_check(void)
{
    uint32_t i, n = 0;
    char *o = row_t[2];
    rowf(0, COL_WHITE, "PANEL CHECK", 0, 0, 0);
    o[0] = 0;
    scat(o, "Faders");
    for (i = 0; i < 4u; i++) { scat(o, " "); scat(o, nb(fm1_in.fader[i], 0)); }
    row_c[2] = COL_ACC;
    rowf(3, COL_ACC, "Wheels ", nb(fm1_in.wheel[0], 0), " / ", nb(fm1_in.wheel[1], 0));
    rowf(4, COL_ACC, "Pedal ", nb(fm1_in.pedal, 0), pedal_down ? "  DOWN" : "  up", 0);
    rowf(5, COL_ACC, "Buttons 0x", hex(fm1_in.buttons), 0, 0);
    for (i = 0; i < NV; i++) n += vc[i].gate || vc[i].ae.stage != E_IDLE;
    rowf(6, COL_ACC, "Voices ", nb(n, 0), "/8   tempo ", nb(ctl_tempo, 0));
#ifndef OBV_HOST
    rowf(7, COL_ACC, "Render ", nb(ui_render_us, 0), " us, peak ", nb(ui_render_peak_us, 0));
    rowf(8, ui_render_overruns ? COL_RED : COL_ACC, "Overruns ", nb(ui_render_overruns, 0), flash_ok ? "   flash OK" : "   flash --", 0);
#else
    rowf(7, COL_DIM, "Render: device only", 0, 0, 0);
#endif
    rowf(9, COL_DIM, "Master ", nb((uint32_t)smk_master, 0), "/1023  fw ", APP_VER);
#ifndef OBV_HOST
    rowf(10, COL_DIM, "Stack irq ", nb(stack_used(_sstack_lo, _sstack_top), 0), " main ", nb(stack_used(_ustack_lo, _ustack_top), 0));
#endif
    rowf(11, COL_DIM, "Release KNOB BANK to leave", 0, 0, 0);
}

static void screen_page(void)
{
    uint32_t pg = page_id(), k;
    const char *title = pg == PGX_DRIVE ? "DRIVE (global)" : PAGE[pg].title;
    char *o;
    rowf(0, COL_WHITE, mode == M_PATCH ? "[PATCH] " : "[PERFORM] ", title, 0, 0);
    if (page_is_fx(pg)) rowf(1, COL_DIM, "effect unit shared by all parts", 0, 0, 0);
    for (k = 0; k < 4u; k++) {
        int p = page_param(pg, k);
        o = row_t[3u + k];
        o[0] = 0;
        row_c[3u + k] = COL_ACC;
        if (p < 0) continue;
        scat(o, "K"); scat(o, nb(k + 1u, 0)); scat(o, " ");
        scat(o, p == 100 ? "TYPE" : p == 101 ? "AMOUNT" : p == 102 ? "TONE" : PDEF[p].name);
        scat(o, "  ");
        page_value_text(o + slen(o), p, page_get(p));
    }
    rowf(8, COL_DIM, "Press the button again: next page", 0, 0, 0);
    rowf(11, COL_DIM, engine_label(sound_engine(ctx_s())), 0, 0, 0);
    if (msg[0]) rowf(9, msg_c, msg, 0, 0, 0);
}

static void screen_draw(void)
{
    scr_clear();
    if (ov == OV_CHECK) screen_check();
    else if (ov == OV_POP) screen_popup();
    else if (ov == OV_SC) screen_sc();
    else if (ov == OV_EXIT) screen_exit();
    else if (ov == OV_FAVSAVE) screen_favsave();
    else {
        if (page_btn >= 0 && mode != M_EDIT) screen_page();
        else if (mode == M_PATCH) screen_patch();
        else if (mode == M_PERF) screen_perf();
        else screen_edit();
        if (mode != M_EDIT && page_btn < 0) rows_footer();
        else if (msg[0]) rowf(11, msg_c, msg, 0, 0, 0);
        else if (bar[0]) rowf(11, COL_ACC, bar, 0, 0, 0);
    }
    scr_flush();
}

/* ---- LEDs --------------------------------------------------------------------------------------- */
static const uint8_t CAT_RGB[17][3] = {
    {0, 0, 0}, {255, 200, 40}, {255, 255, 255}, {255, 120, 60}, {200, 80, 255}, {60, 200, 120}, {80, 220, 220}, {255, 230, 140},
    {255, 90, 40}, {255, 150, 80}, {60, 120, 255}, {120, 90, 255}, {255, 60, 160}, {160, 255, 80}, {255, 100, 100}, {150, 150, 150}, {255, 255, 120}};

static void pad_rgb(uint32_t p, uint32_t r, uint32_t g, uint32_t b, uint32_t pct)
{
    padled_set(p, (uint8_t)(r * pct / 100u), (uint8_t)(g * pct / 100u), (uint8_t)(b * pct / 100u));
}

static uint32_t blink_pct(void) { return (fm1_ms >> 8) & 1u ? 100u : 10u; }      /* the selected pad: about 2 Hz */

static void leds_update(void)
{
    uint32_t p;
    for (p = 0; p < 16u; p++) {
        if (ov == OV_POP || ov == OV_SC || ov == OV_EXIT) {
            if (p == 0u) pad_rgb(p, 40, 90, 255, 90);
            else if (p == 1u) pad_rgb(p, 255, 40, 30, 90);
            else pad_rgb(p, 0, 0, 0, 0);
        } else if (ov == OV_FAVSAVE) {
            if (p == fs_pad) pad_rgb(p, 255, 255, 255, (fm1_ms >> 8) & 1u ? 100u : 20u); else pad_rgb(p, 0, 0, 0, 0);
        } else if (mode == M_PATCH && drum_pads) {
            uint32_t hit = drum_stamp[0][p & 7u] && eng_t + 1u - drum_stamp[0][p & 7u] < FS / 10u;
            if (hit) pad_rgb(p, 255, 255, 255, 100); else pad_rgb(p, 166, 66, 255, 70);
        } else if (mode == M_PATCH) {
            uint32_t idx = kb_bank * 16u + p;
            if (!db.fav[idx].patch) pad_rgb(p, 0, 0, 0, 0);                  /* nothing assigned: dark */
            else if (pat_cat == 1u && pat_idx == idx) pad_rgb(p, 255, 255, 255, blink_pct());   /* selected: white, blinking */
            else { const uint8_t *c = CAT_RGB[patch_cat_of(db.fav[idx].patch)]; pad_rgb(p, c[0], c[1], c[2], 40); }
        } else if (mode == M_PERF) {
            if (p == 7u || p == 15u) {
                uint32_t s = p == 7u ? 1u : 2u;
                if (!cur.line[1u + s].s.patch) pad_rgb(p, 0, 0, 0, 0);
                else pad_rgb(p, 255, 140, 20, solo == s ? blink_pct() : 45u);        /* both solos orange, the active one blinks */
            } else {
                uint32_t slot = p < 7u ? p : p - 1u;
                uint32_t idx = fb_bank * 14u + slot;
                uint32_t on_empty = !perf_filled(&db.perf[fb_bank * 14u + ps_slot]);     /* the cursor is on an empty slot: nothing new is loaded */
                if (!perf_filled(&db.perf[idx])) { if (on_empty && slot == ps_slot) pad_rgb(p, 255, 60, 50, blink_pct()); else pad_rgb(p, 0, 0, 0, 0); }
                else if (cur_slot == (int32_t)idx) pad_rgb(p, 80, 255, 120, solo ? 25u : on_empty ? 70u : blink_pct());   /* selected: green, blinking (steady while a solo or an empty slot holds the cursor) */
                else pad_rgb(p, 60, 140, 255, 40);
            }
        } else {
            if (p < 16u) {
                uint32_t same = (p + 1u) == patch_cat_of(cur.line[edit_line].s.patch);
                pad_rgb(p, CAT_RGB[p + 1u][0], CAT_RGB[p + 1u][1], CAT_RGB[p + 1u][2], same ? 100u : 25u);
            } else pad_rgb(p, 0, 0, 0, 0);
        }
    }
    padled_flush();
    fm1_led_key(BTN_KNOB_BANK, mode == M_PATCH);
    fm1_led_key(BTN_FADER_BANK, mode == M_PERF);
    fm1_led_key(BTN_EDIT, mode == M_EDIT);
    fm1_led_key(BTN_SEQ, theme);
    for (p = 0; p < sizeof PGBTN / sizeof PGBTN[0]; p++) fm1_led_key(PGBTN[p].btn, page_btn == (int8_t)p);
    fm1_led_key(BTN_SAVE, ov == OV_SC);
    fm1_led_key(BTN_LEFT, ov == OV_POP);
    fm1_led_key(BTN_RIGHT, ov == OV_POP);
}

#include "lgs_midi.h"

/* ---- pedal, wheels, per-line sustain / bend / mod ----------------------------------------------- */
static struct { uint32_t since, ready, rest_high, cand; } ped;

static void pedal_poll(void)                    /* the pedal's rest polarity is the level seen at power-on (smk_sustain.h) */
{
    uint32_t raw = fm1_in.pedal, high;
    if (fm1_in.frames < 16u) return;
    if (!ped.ready) {
        high = raw >= 512u;
        if (!ped.since || high != ped.rest_high) { ped.rest_high = high; ped.since = fm1_ms + 1u; }
        if (fm1_ms + 1u - ped.since >= 100u) ped.ready = 1;
        return;
    }
    high = ped.rest_high ? raw < 256u : raw > 768u;
    if (pedal_down) high = ped.rest_high ? raw < 768u : raw > 256u;
    if (high != ped.cand) { ped.cand = high; ped.since = fm1_ms; }
    if (fm1_ms - ped.since >= 12u) pedal_down = high;
}

static void wheels_poll(void)                   /* as obv_smk.h smk_wheels, without the MIDI output */
{
    enum { DEAD = 12, MOD_LO = 485, MOD_HI = 615 };
    static int32_t mlo, mhi;
    int32_t m = fm1_in.wheel[1], mw, b;
    if (fm1_in.frames < 300u) return;
    if (!mhi) { mlo = m < MOD_LO ? m : MOD_LO; mhi = m > MOD_HI ? m : MOD_HI; }
    if (m < mlo) mlo = m;
    if (m > mhi) mhi = m;
    mw = mhi - m <= DEAD ? 0 : (mhi - m - DEAD) * 127 / (mhi - mlo - DEAD > 1 ? mhi - mlo - DEAD : 1);
    mod_w = (uint8_t)clampi(mw > midi_mod ? mw : midi_mod, 0, 127);
    ctl_vib = mod_w;
    b = smk_pw_bend(&smk_pitch_calibration, fm1_in.wheel[0], fm1_ms) * 8192 / (SMK_PW_ST * 4096);
    ctl_wheel_bend = clampi(b, -8192, 8191);
}

static void flags_apply(void)                   /* the lines' sustain / bend / mod settings -> engine routing */
{
    uint32_t p, sus = 0, bend_off = 0, vib_off = 0, key = mod_w;
    for (p = 0; p < LGS_NLINE; p++) {
        uint32_t f = mode == M_PATCH ? (uint32_t)(LF_SUSTAIN | LF_BEND) : cur.line[p].flags;
        uint32_t act = f >> LF_MOD_SHIFT & 7u;
        if (f & LF_SUSTAIN) sus |= 1u << p;
        if (!(f & LF_BEND)) bend_off |= 1u << p;
        if (act != MOD_DEFAULT) vib_off |= 1u << p;
        mod_act[p] = (uint8_t)act;
        key |= act << (8u + 3u * p);
    }
    ctl_bend_off = bend_off;
    ctl_vib_off = vib_off;
    ctl_sustain = 0;
    ctl_hold = (pedal_down || midi_sus) ? sus : 0u;
    if (key != mod_key) {                       /* the wheel or an action changed: levels and cutoff offsets follow */
        mod_key = key;
        for (p = 0; p < LGS_NLINE; p++) {
            if (mode == M_PATCH) { if (p == 0u) part_levels(0, &pat, 99); }
            else if (cur.line[p].s.patch) part_levels(p, &cur.line[p].s, cur.line[p].vol);
        }
    }
}

/* ---- the frame ---------------------------------------------------------------------------------- */
static uint32_t keys_prev;

static void lgs_ui_init(void)
{
    uint32_t i;
    cat_init();
    obv_engines[OBV_FM2] = obv_engines[OBV_ORG] = obv_engines[OBV_SP] = obv_engines[OBV_FM6] = &lgs_clipper_ops;
    lgs_fx_init();
    if (!lgs_store_load()) db_defaults();
    theme = (uint8_t)db.theme;
    for (i = 0; i < 4u; i++) fd_last[i] = fm1_in.fader[i];
    smk_pad_events = 1;
#ifndef OBV_HOST
    stack_paint();
#endif
    pat_cat = 1;
    pat_idx = 0;
    pat_select();
    part_apply(0, &pat, 99);
    fd_reset();
}

static void lgs_frame(void)
{
    uint32_t keys = fm1_in.notes, down = keys & ~keys_prev, up = keys_prev & ~keys, k;
    int32_t e;
    for (k = 0; k < 27u; k++) {
        if (down >> k & 1u) kb_note(1, KEY0 + k, KEY_VEL(k));
        if (up >> k & 1u) kb_note(0, KEY0 + k, 0);
    }
    keys_prev = keys;
    while ((e = smk_extkey_take()) >= 0) {                      /* the keys beyond the 27 note keys */
        uint32_t kk = ((uint32_t)e >> 8) % FM1_NKEY, vel = (uint32_t)e & 0xFFu;
        kb_note(vel ? 1u : 0u, KEY0 + kk - SMK_KEY_F3, vel);
    }
    while ((e = smk_pad_take()) >= 0) {
        uint32_t p = ((uint32_t)e >> 8) & 15u;
        pad_vel = (uint32_t)e & 255u;
        if (pad_vel) pad_press(p); else pad_release(p);
    }
    pads_hold_poll();
    buttons_poll();
    knobs_poll();
    faders_poll();
    midi_poll();
    pedal_poll();
    wheels_poll();
    flags_apply();
    ov_timers();
    if (pop_audition && ov != OV_POP) pop_restore();       /* the popup closed without choosing: the stored sound returns */
    meters_poll();
    if (save_pending) {
        int rc = lgs_store_commit();
        if (rc) {
            save_pending = 0;
            if (rc > 0) say(save_ok, COL_GRN, 2000);
            else say("Save failed", COL_RED, 3000);
        }
    }
    if (ui_dirty) { ui_dirty = 0; screen_draw(); }
    leds_update();
}
