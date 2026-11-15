/* SPDX-License-Identifier: GPL-3.0-only */
/* Logical names for the eight encoders (D-03). Physical order is left to right = K1..K8 = fm1_enc_take() index 0..7
 * (hal/smk_input.h SMK_ENCC; SMKSYNTH 1.2.0 uses the same order, K8 = master since its PF12). All encoders are endless. */
enum {
    LGS_K1 = 0, LGS_K2, LGS_K3, LGS_K4,         /* the four patch parameters (CUTOFF, RESO, ENV AMT, RELEASE for subtractive engines) */
    LGS_K5, LGS_K6, LGS_K7,                     /* navigation: PATCH = tempo, category, patch (03-ux-spec.md section 1) */
    LGS_K8,                                     /* MASTER: handled in hal/smk_input.h as a virtual pot (SMK_ENC_MASTER == 7) */
    LGS_NKNOB
};
_Static_assert(LGS_K8 == 7u, "K8 must be the encoder the HAL uses as MASTER (SMK_ENC_MASTER)");
/* detents turned since the last call (+ = clockwise); never use LGS_K8 here: the HAL's master pot consumes it */
static int32_t lgs_knob(uint32_t k) { return k < LGS_K8 ? fm1_enc_take(k) : 0; }
