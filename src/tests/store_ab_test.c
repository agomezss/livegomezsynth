/* SPDX-License-Identifier: GPL-3.0-only */
/* A/B storage logic against a fake flash: a power cut at every erase / data write / header write step must leave
 * either the previous or the new image, never a mix; corrupted copies are ignored.
 *   cc -O1 -Wall -Ifirmware/src -o build/store_ab_test tests/store_ab_test.c && build/store_ab_test */
#include <stdio.h>
#include <string.h>
#include "lgs_ab.h"

#define SECT 4096u
#define SIZE 7820u
static uint8_t flash[4][SECT];
static int cut_after = -1, ops;                  /* power cut: fail every primitive after this many */
static int fails;

/* sector_save/sector_load as platform.c: [magic][len][crc] header, written last */
#define HM 0x59474E44u
static int prim(void) { return cut_after >= 0 && ops++ >= cut_after; }
static uint8_t *sec(uint32_t at) { return flash[(at - 0x99000u) / SECT == 0 ? 0 : (at - 0x99000u) / SECT == 1 ? 1 : (at - 0x99000u) / SECT == 2 ? 2 : 3]; }
static int fsave(uint32_t at, const void *p, uint32_t n)
{
    uint8_t *s = sec(at);
    uint32_t h[3] = {HM, n, ab_crc32(p, n)};
    if (prim()) return -1;
    memset(s, 0xFF, SECT);
    if (prim()) return -1;
    memcpy(s + 12, p, n);
    if (prim()) return -1;
    memcpy(s, h, 12);
    return 0;
}
static int fload(uint32_t at, void *p, uint32_t n)
{
    uint8_t *s = sec(at);
    uint32_t h[3];
    memcpy(h, s, 12);
    if (h[0] != HM || h[1] != n || ab_crc32(s + 12, n) != h[2]) return -1;
    memcpy(p, s + 12, n);
    return 0;
}
static const ab_io_t io = {fload, fsave, {0x99000u, 0x9B000u}, 0x1000u, 0x4C47530Au, 1u};

static void fill(uint8_t *img, uint8_t v)
{
    memset(img, v, SIZE);
    ab_seal(&io, img, SIZE, 0);
}
static int check(const char *n, int ok) { printf("  %s  %s\n", ok ? "ok  " : "FAIL", n); if (!ok) fails++; return ok; }

int main(void)
{
    static uint8_t img[SIZE], scr[SIZE], out[SIZE];
    ab_state_t st;
    int step, i;

    memset(flash, 0xFF, sizeof flash);
    check("empty flash: nothing to load", !ab_load(&io, &st, out, scr, SIZE));
    fill(img, 1);
    check("first commit", !ab_commit(&io, &st, img, SIZE));
    check("load after first commit", ab_load(&io, &st, out, scr, SIZE) && out[100] == 1 && st.gen == 1);
    fill(img, 2);
    check("second commit goes to the other copy", !ab_commit(&io, &st, img, SIZE) && st.cur == 1 && st.gen == 2);
    check("newest generation wins", ab_load(&io, &st, out, scr, SIZE) && out[100] == 2 && st.gen == 2);

    /* a power cut at every step of the third commit */
    for (step = 0; step < 6; step++) {
        uint8_t save[4][SECT];
        ab_state_t s2;
        memcpy(save, flash, sizeof flash);
        ab_load(&io, &s2, out, scr, SIZE);
        fill(img, 3);
        cut_after = step; ops = 0;
        int rc = ab_commit(&io, &s2, img, SIZE);
        cut_after = -1;
        {
            ab_state_t s3;
            int got = ab_load(&io, &s3, out, scr, SIZE);
            char nm[64];
            snprintf(nm, sizeof nm, "cut at step %d: %s, image intact", step, rc ? "commit failed" : "commit done");
            check(nm, got && (out[100] == 2 || out[100] == 3) && out[SIZE / 2] == out[100] && out[SIZE - 5] == out[100] && (rc ? out[100] == 2 : out[100] == 3));
        }
        memcpy(flash, save, sizeof flash);
    }
    /* a full commit survives the next boot; a corrupted newest copy falls back to the older one */
    fill(img, 4);
    ab_load(&io, &st, out, scr, SIZE);
    ab_commit(&io, &st, img, SIZE);
    check("third commit ok", ab_load(&io, &st, out, scr, SIZE) && out[100] == 4 && st.gen == 3);
    flash[st.cur == 0 ? 1 : 3][200] ^= 0x55;     /* damage the second sector of the newest copy */
    check("damaged newest copy: older one used", ab_load(&io, &st, out, scr, SIZE) && out[100] == 2 && st.gen == 2);
    memset(flash[0], 0, SECT); memset(flash[1], 0, SECT);
    memset(flash[2], 0, SECT); memset(flash[3], 0, SECT);
    check("both copies damaged: nothing", !ab_load(&io, &st, out, scr, SIZE));
    (void)i;
    printf(fails ? "FAILED\n" : "store A/B: all checks passed\n");
    return fails ? 1 : 0;
}
