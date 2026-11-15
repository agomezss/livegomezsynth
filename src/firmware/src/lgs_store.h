/* SPDX-License-Identifier: GPL-3.0-only */
/* User data persistence (T1.9). A stored image is accepted only when magic, version, size and CRC match; its content is
 * then sanitized so that a stale or damaged image can never put the engine in an undefined state.
 * Simulator: the page keeps the image in localStorage and hands it over before the first frame (sim.c).
 * Device: two copies (A/B, lgs_ab.h) in the user-data windows 0x99000 and 0x9B000 through platform.c's sector_save, which
 * goes through the guarded flash primitives; written only when nothing sounds (flash erase masks the interrupts). */
#include "lgs_ab.h"

static void sound_sanitize(sound_t *s)
{
    uint32_t i;
    if (s->patch && !pat_by_id(s->patch)) { memset(s, 0, sizeof *s); return; }
    for (i = 0; i < 4u; i++) {
        if (s->tw[i] > 127u) s->tw[i] = 127;
        if (s->snd[i] > 127u) s->snd[i] = 127;
    }
}

static void db_sanitize(void)
{
    uint32_t i, l;
    if (db.theme > 1u) db.theme = 0;
    for (i = 0; i < LGS_NFAV; i++) sound_sanitize(&db.fav[i]);
    for (i = 0; i < LGS_NPERF; i++)
        for (l = 0; l < LGS_NLINE; l++) {
            lgline_t *x = &db.perf[i].line[l];
            sound_sanitize(&x->s);
            if (x->origin > LGS_NFAV) x->origin = 0;
            if (x->lo < 1u) x->lo = 1;
            if (x->hi > 7u) x->hi = 7;
            if (x->lo > 7u) x->lo = 7;
            if (x->hi < x->lo) x->hi = x->lo;
            if (x->vol > 99u) x->vol = 99;
            if ((x->flags >> LF_MOD_SHIFT & 7u) >= MOD_N) x->flags &= 3u;
        }
}

#ifdef OBV_WASM
static const ab_io_t store_io = {0, 0, {0, 0}, 0, LGS_DB_MAGIC, LGS_DB_VER};
static uint8_t store_stage[sizeof db] __attribute__((aligned(4)));
static uint32_t store_stage_len;
static int lgs_store_load(void)
{
    if (store_stage_len != sizeof db || !ab_image_ok(&store_io, store_stage, sizeof db)) return 0;
    memcpy(&db, store_stage, sizeof db);
    db_sanitize();
    return 1;
}
static void db_seal(void) { ab_seal(&store_io, &db, sizeof db, db.gen); }
static int lgs_store_commit(void) { return 1; }          /* the page stores the image (sim_db_export) */
#elif defined(OBV_HOST)
static int lgs_store_load(void) { return 0; }
static int lgs_store_commit(void) { return 1; }
#else
#define STORE_COPY_A 0x00099000u                          /* inside the user-data bank's allow-listed window (safety-audit.md) */
#define STORE_COPY_B 0x0009B000u
static const ab_io_t store_io = {sector_load, sector_save, {STORE_COPY_A, STORE_COPY_B}, 0x1000u, LGS_DB_MAGIC, LGS_DB_VER};
static ab_state_t store_st = {0, -1};
static uint8_t store_scratch[sizeof db] __attribute__((aligned(4)));

static int lgs_store_load(void)
{
    if (!flash_ok || !ab_load(&store_io, &store_st, (uint8_t *)&db, store_scratch, sizeof db)) return 0;
    db_sanitize();
    return 1;
}

static int lgs_store_commit(void)                       /* 1 done, 0 not now (something sounds), -1 failed */
{
    if (!flash_ok) return -1;
    if (!engine_quiet() || fm1_in.notes || fm1_in.keys) return 0;
    fm1_wdt_feed();
    return ab_commit(&store_io, &store_st, (uint8_t *)&db, sizeof db) ? -1 : 1;
}
#endif
