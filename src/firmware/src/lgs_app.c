/* SPDX-License-Identifier: GPL-3.0-only */
/* LiveGomezSynth application shell (walking skeleton, T0.7a).
 * Replaces obervave.c as the application unit: the voice/DSP modules of the OberVave code base are reused, the
 * panel/screen/storage glue is new. Contract with platform.c: app_audio() (audio ISR) and app_main() (never returns).
 * No step sequencer (D-50): its five hooks are stubbed in lgs_seq_stub.h. */
static volatile unsigned smk_controller_active;
#define SMK_CONTROLLER_ACTIVE smk_controller_active
static void app_idle(void);
#define LCD_IDLE() app_idle()
#include "platform.c"
#include "obv_dsp.h"
#include "smk_midi_mode.h"
#include "obv_params.h"
#include "obv_fx.h"
#include "smk_midi_routes.h"
#include "obv_engine.h"
#include "lgs_seq_stub.h"
#include "obv_patches.h"

static void fmt_u(char *s, uint32_t v)          /* decimal, no leading zeros */
{
    char b[12];
    uint32_t i = 0, j = 0;
    do { b[i++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (i) s[j++] = b[--i];
    s[j] = 0;
}
static uint32_t slen(const char *s) { uint32_t n = 0; while (s[n]) n++; return n; }
static void scat(char *d, const char *s) { d += slen(d); while ((*d++ = *s++)) ; }

static void app_audio(int32_t *out, uint32_t n)
{
#ifndef OBV_HOST
    uint32_t t0 = fm1_ticks();
#endif
#if OBV_VOICE_HW && !defined(OBV_HOST)
    if (b2_latched) { memset(out, 0, n * 2u * sizeof *out); return; }
#endif
    engine_audio(out, n);
#ifndef OBV_HOST
    ui_render_us = (fm1_ticks() - t0) / FM1_TICKS_PER_US;
    if (ui_render_us > ui_render_peak_us) ui_render_peak_us = ui_render_us;
    if (ui_render_us > n * 1000000u / FS) ui_render_overruns++;
#endif
}

static void app_idle(void) {}
#ifdef OBV_HOST
static void host_dump(void) {}                  /* (tests/host.c: the script's DUMP event) */
#endif

/* The SMKSYNTH bank backup/restore session (smk_backup.h) never starts in this build: it belongs to the old data layout. */
static int b1_can_begin(void) { return 0; }

#define KEY0 53u                                /* keybed key 0 = F3 */
#define KEY_VEL(k) fm1_in.key_vel[SMK_KEY_F3 + (k)]
#include "lgs_input.h"

#include "lgs_flavors.h"
#include "lgs_engine_adapter.h"
#include "lgs_eng_fm2.h"
#include "lgs_eng_org.h"
#include "lgs_eng_sp.h"
#include "lgs_eng_fm6.h"
#include "lgs_eng_voc.h"
#include "lgs_ui.h"

static void app_main(void)
{
    engine_init();
    lgs_ui_init();
    for (;;) {
        platform_poll();
        lgs_frame();
    }
}
