/* SPDX-License-Identifier: GPL-3.0-only */
/* MIDI input (T0.7b): notes (any channel) play like the keybed, minus the OCTAVE buttons; CC 1 mod, CC 7 volume, CC 64
 * sustain, CC 120 / 123 all notes off, pitch bend, and the MIDI clock for the arpeggiator. No MIDI output in this firmware. */
static uint8_t midi_mod, midi_sus;
static uint32_t clk_n, clk_t0, clk_last;

static void midi_poll(void)
{
    uint8_t st, d1, d2;
    while (midi_get(&st, &d1, &d2)) {
        uint32_t kind = st & 0xF0u;
        midi_input_active = 1;
        if (st == 0xF8u) {                              /* clock, 24 per quarter: the tempo from 4 quarters, rounded as quarter_len() */
            if (!clk_n || fm1_ms - clk_last > 250u) { clk_n = 0; clk_t0 = eng_t; }
            clk_last = fm1_ms;
            if (++clk_n == 97u) {
                uint32_t q = ((eng_t - clk_t0) / 4u + 24u) / 48u * 48u, d;
                if (q > FS / 5u && q < FS * 8u / 5u) {
                    d = q > ctl_ext_quarter ? q - ctl_ext_quarter : ctl_ext_quarter - q;
                    if (!ctl_ext_quarter || d * 1000u > 3u * q) ctl_ext_quarter = q;
                }
                clk_n = 1; clk_t0 = eng_t;
            }
        } else if (st == 0xFAu || st == 0xFBu) { ctl_arp_restart = 1; clk_n = 0; }
        else if (st >= 0xF0u) { /* other system messages: ignored */ }
        else if (kind == 0x90u && d2) { kb_ext = 1; kb_note(1, d1, d2); kb_ext = 0; }
        else if (kind == 0x80u || kind == 0x90u) { kb_ext = 1; kb_note(0, d1, 0); kb_ext = 0; }
        else if (kind == 0xE0u) ctl_bend = (int32_t)(d1 | d2 << 7) - 8192;
        else if (kind == 0xB0u) {
            if (d1 == 1u) midi_mod = d2;
            else if (d1 == 7u) ctl_cc7 = d2;
            else if (d1 == 64u) midi_sus = d2 >= 64u;
            else if (d1 == 120u || d1 == 123u) { keys_release_all(); ctl_panic = 1; }
        }
        midi_input_active = 0;
    }
    if (ctl_ext_quarter && fm1_ms - clk_last > 1000u) { ctl_ext_quarter = 0; clk_n = 0; }   /* the clock stopped: the internal tempo again */
}
