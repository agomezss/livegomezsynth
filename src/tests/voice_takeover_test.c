/* SPDX-License-Identifier: GPL-3.0-only */
/* First start over SMKSYNTH's private bank (smk_voice_store.h): the owner tag of another SMK37DATA family is taken over,
 * only the owner, settings and program / favourite sectors are cleared, everything else is left alone, a cut at any step
 * is recovered on the next start, and a tag that is not SMK37DATA stays refused. Host test, a fake 1 MiB flash.
 *   cc -O1 -DBOARD_SMK37=1 -Ifirmware/hal -Ifirmware/src -o /tmp/vt tests/voice_takeover_test.c && /tmp/vt */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fm1_flash_map.h"

static uint8_t flash[0x100000];
static int ops, cut_at, bad_access;
static int voice_store_read(uint32_t o, void *p, uint32_t n) { if (o + n > sizeof flash) return -1; memcpy(p, flash + o, n); return 0; }
#define WRITE_OK(off, n) (FL_IN(off, n, 0x55000u, 0x57000u) || FL_IN(off, n, 0x98000u, 0x99000u) || FL_IN(off, n, 0x99000u, 0x9B000u) || \
                          FL_IN(off, n, 0x9B000u, 0x9D000u) || FL_IN(off, n, 0x57000u, 0x58000u) || FL_IN(off, n, 0x58000u, 0x98000u))
static int voice_store_write(uint32_t o, const void *p, uint32_t n)
{
    if (!WRITE_OK(o, n)) { bad_access++; return -1; }
    if (cut_at && ++ops >= cut_at) return -9;
    for (uint32_t i = 0; i < n; i++) flash[o + i] &= ((const uint8_t *)p)[i];
    return 0;
}
static int voice_store_erase(uint32_t o)
{
    if (!WRITE_OK(o, 4096u) || (o & 0xFFFu)) { bad_access++; return -1; }
    if (cut_at && ++ops >= cut_at) return -9;
    memset(flash + o, 0xFF, 4096);
    return 0;
}
static void voice_store_feed(void) {}
#define section(x)                              /* (the .pool section attribute is for the device) */
#define OBV_VOICE_STORE_IMPLEMENTATION 1
#include "smk_voice_store.h"

static int fails;
#define CHECK(c, m) do { if (c) printf("  ok    %s\n", m); else { printf("  FAIL  %s\n", m); fails++; } } while (0)
static const uint8_t LGS[16] = {'S','M','K','3','7','D','A','T','A','L','I','V','E','G','M','Z'};
static const uint8_t OBV[16] = {'S','M','K','3','7','D','A','T','A','O','B','E','R','V','A','V'};

static void smksynth_flash(void)               /* a keyboard that ran SMKSYNTH 1.2 */
{
    memset(flash, 0xFF, sizeof flash);
    memcpy(flash + VOICE_OWNER_AT, OBV, 16);
    memset(flash + VOICE_SETTINGS_AT, 0x11, 4096);
    memset(flash + VOICE_PSECT_AT, 0x22, 8192);
    memset(flash + VOICE_PFAV_AT, 0x33, 8192);
    memset(flash + VOICE_SLOTS_AT, 0x44, 4096 * 4);
    memset(flash + VOICE_STABLE_SETTINGS_AT, 0x55, 4096);
    memcpy(flash + VOICE_STABLE_OWNER_AT, OBV, 16);
    memset(flash + 0xC1000, 0x66, 0x1000);       /* calibration */
    memset(flash + 0x10000, 0x77, 0x1000);       /* the app */
}
static int untouched(void)
{
    for (uint32_t i = 0; i < 4096u * 4u; i++) if (flash[VOICE_SLOTS_AT + i] != 0x44) return 0;
    for (uint32_t i = 0; i < 4096u; i++) if (flash[VOICE_STABLE_SETTINGS_AT + i] != 0x55 || flash[0xC1000 + i] != 0x66 || flash[0x10000 + i] != 0x77) return 0;
    return !memcmp(flash + VOICE_STABLE_OWNER_AT, OBV, 16);
}

int main(void)
{
    int rc;
    smksynth_flash();
    CHECK(voice_store_prepare(LGS) == -8, "prepare refuses SMKSYNTH's tag (the reason the takeover exists)");
    rc = voice_store_takeover(LGS);
    CHECK(rc == 1, "takeover succeeds");
    CHECK(!memcmp(flash + VOICE_OWNER_AT, LGS, 16), "our tag is written");
    CHECK(flash[VOICE_SETTINGS_AT] == 0xFF && flash[VOICE_PSECT_AT + 8191] == 0xFF && flash[VOICE_PFAV_AT + 8191] == 0xFF, "settings, programs and favourites are cleared");
    CHECK(untouched(), "pattern slots, the stable store, the app and the calibration are untouched");
    CHECK(bad_access == 0, "no erase or write outside the allowed windows");
    CHECK(voice_store_prepare(LGS) == 0, "the next start finds the store ready");

    smksynth_flash();
    memcpy(flash + VOICE_OWNER_AT, "NOTOURSATALL1234", 16);
    CHECK(voice_store_takeover(LGS) == -8, "a tag that is not SMK37DATA stays refused");
    CHECK(flash[VOICE_SETTINGS_AT] == 0x11 && flash[VOICE_PSECT_AT] == 0x22, "and nothing is erased");

    int worst = 0;
    for (int c = 1; c <= 14; c++) {              /* a power cut at every erase / write step */
        smksynth_flash(); ops = 0; cut_at = c;
        rc = voice_store_takeover(LGS);
        cut_at = 0;
        int tag_ours = !memcmp(flash + VOICE_OWNER_AT, LGS, 16), blank = 1;
        for (int i = 0; i < 16; i++) blank &= flash[VOICE_OWNER_AT + i] == 0xFF;
        if (rc == 1 && !tag_ours) worst++;
        if (rc != 1) {                           /* next start: whatever state, prepare / takeover reach a store that works */
            int r2 = voice_store_prepare(LGS);
            if (r2 == -8) r2 = voice_store_takeover(LGS);
            if (r2 < 0 || memcmp(flash + VOICE_OWNER_AT, LGS, 16) || (!blank && !untouched())) worst++;   /* a blank tag is SMKSYNTH's own first-clone path */
        }
    }
    CHECK(worst == 0, "a cut at any step is recovered on the next start, nothing else is touched");
    printf(fails ? "voice takeover: FAILED\n" : "voice takeover: all checks passed\n");
    return fails ? 1 : 0;
}
