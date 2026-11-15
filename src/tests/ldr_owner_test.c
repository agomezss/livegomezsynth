/* SPDX-License-Identifier: GPL-3.0-only */
/* The update loader's owner gate: compiles the real firmware/loader/ldr_core.c with stubbed hooks and checks which owner tags
 * of the user bank (0x57000) let an update through. The bank must be kept when it is SMKSYNTH's or this build's own family
 * (a LiveGomezSynth keyboard must be updatable more than once), accepted when erased, refused for anything else. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define BOARD_SMK37 1
#define OBV_VOICE_HW 1
#ifndef OBV_DATA_FAMILY
#define OBV_DATA_FAMILY "LIVEGMZ"
#endif

static void ota_jl_enc(uint8_t *p, uint32_t n) { (void)p; (void)n; }
static uint32_t ota_crc16(const uint8_t *p, uint32_t n, uint32_t c) { (void)p; (void)n; return c; }
static uint32_t ota_rd16(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8; }
static uint32_t ota_rd32(const uint8_t *p) { return ota_rd16(p) | ota_rd16(p + 2) << 16; }
static int ota_memeq(const uint8_t *a, const uint8_t *b, uint32_t n) { return memcmp(a, b, n) == 0; }
static int ota_read(uint32_t off, void *p, uint32_t n) { (void)off; (void)p; (void)n; return -1; }
static int ota_ufw(uint8_t *h, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d, uint32_t *e) { (void)h; (void)a; (void)b; (void)c; (void)d; (void)e; return -1; }
static int ldr_fread(uint32_t off, void *p, uint32_t n) { (void)off; (void)p; (void)n; return -1; }
static int ldr_erase(uint32_t off) { (void)off; return -1; }
static int ldr_prog(uint32_t off, const void *p, uint32_t n) { (void)off; (void)p; (void)n; return -1; }
static void ldr_record_clear(void) {}
static int ldr_flash_known(void) { return 1; }
static void ldr_progress(uint32_t done, uint32_t total) { (void)done; (void)total; }

#include "../firmware/loader/ldr_core.c"

static int fail;
static void check(const char *name, int ok)
{
    printf("  %s    %s\n", ok ? "ok  " : "FAIL", name);
    if (!ok) fail = 1;
}

static int st(const char *tag16)
{
    uint8_t o[16];
    memcpy(o, tag16, 16);
    return ldr_owner_state(o);
}

int main(void)
{
    uint8_t blank[16], own[16];
    memset(blank, 0xFF, sizeof blank);
    memset(own, 0, sizeof own);
    memcpy(own, "SMK37DATA" OBV_DATA_FAMILY, 9 + sizeof OBV_DATA_FAMILY - 1u);
    check("SMKSYNTH's bank is kept", st("SMK37DATASMK16P1") == 1);
    check("this build's own bank (SMK37DATA + " OBV_DATA_FAMILY ") is kept", ldr_owner_state(own) == 1);
    check("an erased tag is accepted, the bank is written", ldr_owner_state(blank) == 0);
    check("another family's bank is refused", st("SMK37DATAOBERVAV") == -1);
    check("a tag one character off ours is refused", (own[15] ^= 1, ldr_owner_state(own) == -1));
    check("one byte off SMKSYNTH's is refused", st("SMK37DATASMK16P2") == -1);
    check("a half-erased tag is refused", (blank[3] = 0, ldr_owner_state(blank) == -1));
    check("zeros are refused", st("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0") == -1);
    printf(fail ? "ldr owner gate: FAILED\n" : "ldr owner gate: all checks passed\n");
    return fail;
}
