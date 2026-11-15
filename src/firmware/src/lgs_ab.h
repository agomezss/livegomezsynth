/* SPDX-License-Identifier: GPL-3.0-only */
/* Power-cut-safe storage of one image in two copies (A/B), each copy two sectors written with sector_save (header last).
 * Image layout (all little-endian words): [0] magic, [1] version, [2] generation, ... , [size-4] CRC-32 of all before it.
 * A copy counts only when both sector chunks load AND the whole-image CRC matches, so a cut at any point leaves either
 * the old or the new image, never a mix. The newest valid generation wins. Pure logic: tests/store_ab_test.c runs it
 * against a fake flash with a power cut injected at every write step. */
#include <stdint.h>                              /* memcpy comes from the includer (the firmware's own, or string.h in the host test) */

typedef struct {
    int (*load)(uint32_t at, void *p, uint32_t n);            /* 0 = ok; the chunk's own header + CRC checked by it */
    int (*save)(uint32_t at, const void *p, uint32_t n);      /* 0 = ok */
    uint32_t at[2];                                           /* first sector of copy A and B */
    uint32_t step;                                            /* distance to a copy's second sector */
    uint32_t magic, ver;
} ab_io_t;
typedef struct { uint32_t gen; int8_t cur; } ab_state_t;      /* cur: copy holding the newest valid image, -1 none */

#define AB_GEN_OFF 8u

static uint32_t ab_crc32(const void *b, uint32_t n)
{
    const uint8_t *p = b;
    uint32_t c = 0xFFFFFFFFu, i, k;
    for (i = 0; i < n; i++) {
        c ^= p[i];
        for (k = 0; k < 8u; k++) c = c & 1u ? (c >> 1) ^ 0xEDB88320u : c >> 1;
    }
    return ~c;
}

static uint32_t ab_word(const void *img, uint32_t i) { uint32_t w; memcpy(&w, (const uint8_t *)img + 4u * i, 4); return w; }

static int ab_image_ok(const ab_io_t *io, const void *img, uint32_t size)
{
    return ab_word(img, 0) == io->magic && ab_word(img, 1) == io->ver && ab_crc32(img, size - 4u) == ab_word(img, (size - 4u) / 4u);
}

static void ab_seal(const ab_io_t *io, void *img, uint32_t size, uint32_t gen)
{
    uint8_t *b = img;
    uint32_t c;
    memcpy(b, &io->magic, 4);
    memcpy(b + 4, &io->ver, 4);
    memcpy(b + AB_GEN_OFF, &gen, 4);
    c = ab_crc32(b, size - 4u);
    memcpy(b + size - 4u, &c, 4);
}

static int ab_read_copy(const ab_io_t *io, uint32_t c, uint8_t *buf, uint32_t size)
{
    uint32_t n0 = (size + 1u) / 2u;
    if (io->load(io->at[c], buf, n0) || io->load(io->at[c] + io->step, buf + n0, size - n0)) return -1;
    return ab_image_ok(io, buf, size) ? 0 : -1;
}

static int ab_load(const ab_io_t *io, ab_state_t *st, uint8_t *img, uint8_t *scratch, uint32_t size)   /* 1: img holds the newest valid image */
{
    uint32_t c;
    st->cur = -1;
    st->gen = 0;
    for (c = 0; c < 2u; c++)
        if (!ab_read_copy(io, c, scratch, size)) {
            uint32_t g = ab_word(scratch, AB_GEN_OFF / 4u);
            if (st->cur < 0 || g > st->gen) { memcpy(img, scratch, size); st->gen = g; st->cur = (int8_t)c; }
        }
    return st->cur >= 0;
}

static int ab_commit(const ab_io_t *io, ab_state_t *st, uint8_t *img, uint32_t size)    /* 0: written; the other copy is overwritten */
{
    uint32_t n0 = (size + 1u) / 2u, c = st->cur == 0 ? 1u : 0u, g = st->gen + 1u;
    ab_seal(io, img, size, g);
    if (io->save(io->at[c], img, n0)) return -1;
    if (io->save(io->at[c] + io->step, img + n0, size - n0)) return -1;
    st->gen = g;
    st->cur = (int8_t)c;
    return 0;
}
