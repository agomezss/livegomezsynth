# SPDX-License-Identifier: GPL-3.0-only
"""The ICONIC category (16): iconic timbres of well-known songs, as recipes for the engines we have. Names hint at the song
without using titles of instruments or products (D-42). Everything here is a first draft made from parameters, to be judged by ear
(DOCs/notes/repertoire-coverage.md). Consumed by tools/gen_famous.py.

SYN recipes: dict of parameter suffix -> value (an int or a C enum token); P_<KEY> in the firmware.
FM2 / ORG / SP / FM6 recipes: tuples as the engines' preset tables (see the comments at each list)."""

SAW2 = dict(O1_WAVE="W_SAW", O2_WAVE="W_SAW", O2_FINE=72)


def m(*dicts, **kw):
    r = {}
    for d in dicts:
        r.update(d)
    r.update(kw)
    return r


def brass(cut=55, fenv=100, fa=30, fd=70, fs=60, aa=20, ar=45, spread=80, vint=75, cho=35, rev=22, **kw):
    return m(SAW2, dict(FTYPE="F_LP24", CUTOFF=cut, RESO=12, FENV_AMT=fenv, FA=fa, FD=fd, FS=fs, AA=aa, AS=110, AR=ar, VINTAGE=vint,
             SPREAD=spread, CHO_MIX=cho, CHO_RATE=28, REV_SIZE=85, REV_MIX=rev, ), kw)


def strings(aa=70, ar=95, cut=60, cho=60, rev=40, **kw):
    return m(SAW2, dict(FTYPE="F_LP24", CUTOFF=cut, RESO=6, FENV_AMT=66, FA=60, AA=aa, AD=90, AS=120, AR=ar, VINTAGE=70, SPREAD=115,
             LFO_PITCH=5, LFO_DELAY=80, LFO_RATE=70, CHO_MIX=cho, CHO_RATE=26, REV_SIZE=110, REV_MIX=rev, ), kw)


def pad(aa=85, ar=100, cut=52, **kw):
    return m(dict(O1_WAVE="W_BOTH", O1_PW=70, O2_WAVE="W_PLS", O2_PW=90, O2_FINE=75, LFO_PWM=45, LFO_RATE=30, FTYPE="F_LP24", CUTOFF=cut,
             RESO=14, FENV_AMT=80, FA=90, FD=100, FS=60, AA=aa, AR=ar, VINTAGE=80, SPREAD=120, CHO_MIX=50, REV_SIZE=115, REV_MIX=45, ), kw)


def lead(cut=75, reso=22, glide=18, delay=30, **kw):
    return m(dict(MODE="M_MONO", O1_WAVE="W_SAW", O2_WAVE="W_PLS", O2_PW=60, O2_FINE=68, FTYPE="F_LP24", CUTOFF=cut, RESO=reso, FENV_AMT=80,
             FD=65, FS=55, GLIDE=glide, AA=3, AR=40, LFO_PITCH=6, LFO_DELAY=60, DLY_TIME="DD_8", DLY_FB=35, DLY_MIX=delay,
             REV_SIZE=75, REV_MIX=20, ), kw)


def bass(cut=40, reso=30, fd=55, fs=15, **kw):
    return m(dict(MODE="M_MONO", O1_WAVE="W_SAW", O2_WAVE="W_PLS", O2_PW=50, O2_SEMI=12, O2_LVL=80, FTYPE="F_LP24", CUTOFF=cut, RESO=reso,
             FENV_AMT=100, FD=fd, FS=fs, AA=1, AD=55, AS=80, AR=22, DRIVE=25, KEYTRK=75, ), kw)


def pluck(cut=60, fd=45, ad=60, **kw):
    return m(SAW2, dict(FTYPE="F_LP24", CUTOFF=cut, RESO=25, FENV_AMT=105, FD=fd, FS=0, AA=1, AD=ad, AS=0, AR=35, CHO_MIX=25, REV_SIZE=75,
             REV_MIX=20, ), kw)


# ---- SYN (subtractive voice core) ---------------------------------------------------------------------------------------
SYN = [
    ("TAKE ON SYNTH", m(O1_WAVE="W_PLS", O1_PW=42, O2_WAVE="W_SAW", O2_FINE=70, FTYPE="F_LP24", CUTOFF=82, RESO=20, FENV_AMT=70, FD=55, FS=35,
                        AA=1, AD=82, AS=95, AR=38, VINTAGE=55, CHO_MIX=40, CHO_RATE=30, DLY_TIME="DD_8D", DLY_FB=32, DLY_MIX=26, REV_SIZE=80, REV_MIX=20)),
    ("TAKE ON BASS", bass(cut=46, reso=25, fd=50, fs=35)),
    ("FREE SYNTH LEAD", lead(cut=82, reso=24, glide=24, delay=28)),
    ("FREE SYNTH BASS", bass(cut=42, reso=40, fd=60, fs=20)),
    ("NIGHT STRINGS", strings(aa=82, ar=100, rev=48)),
    ("NIGHT LEAD", lead(cut=92, reso=18, glide=8, delay=34, O2_WAVE="W_SAW", O2_PW=64)),
    ("DOOR ORGAN", m(O1_WAVE="W_PLS", O1_PW=64, O2_WAVE="W_PLS", O2_PW=64, O2_SEMI=36, O2_LVL=60, FTYPE="F_LP12", CUTOFF=100, RESO=5, FENV_AMT=64,
                     VEL_AMP=0, VEL_FLT=0, AA=0, AD=40, AS=127, AR=20, LFO_PITCH=4, LFO_RATE=62, CHO_MIX=25, REV_SIZE=60, REV_MIX=14)),
    ("FIRE ORGAN", m(O1_WAVE="W_PLS", O1_PW=40, O2_WAVE="W_SAW", O2_SEMI=36, O2_LVL=55, FTYPE="F_LP12", CUTOFF=95, RESO=8, FENV_AMT=64, VEL_AMP=0,
                     AA=0, AS=127, AR=18, LFO_AMP=18, LFO_RATE=75, CHO_MIX=30, REV_SIZE=60, REV_MIX=14)),
    ("RAVE STAB", m(SAW2, O2_SEMI=36, O2_LVL=90, FTYPE="F_LP12", CUTOFF=70, RESO=12, FENV_AMT=92, FD=42, FS=20, AA=0, AD=66, AS=0, AR=30,
                    SPREAD=90, CHO_MIX=30, REV_SIZE=70, REV_MIX=18)),
    ("HOUSE BASS", bass(cut=38, reso=35, fd=45, fs=40)),
    ("SWEET DREAM SYN", m(MODE="M_MONO", O1_WAVE="W_SAW", O2_WAVE="W_SAW", O2_FINE=67, O2_SEMI=0, O2_LVL=85, FTYPE="F_LP24", CUTOFF=58,
                          RESO=34, FENV_AMT=72, FA=6, FD=62, FS=72, AA=0, AD=80, AS=115, AR=26, KEYTRK=70, VINTAGE=60, DRIVE=18,
                          CHO_MIX=20, DLY_MIX=0, REV_SIZE=40, REV_MIX=8)),
    ("SWEET DREAM BAS", bass(cut=36, reso=45, fd=38, fs=0, AD=40)),
    ("ENJOY LEAD", lead(cut=78, reso=30, glide=12, delay=26, AA=18, O2_WAVE="W_SAW", O2_PW=64, O2_FINE=71)),
    ("ENOUGH SQUARE", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=64, O2_WAVE="W_PLS", O2_PW=64, O2_FINE=69, FTYPE="F_LP12", CUTOFF=88, RESO=14,
                        FENV_AMT=70, FD=60, FS=70, AA=0, AR=30, GLIDE=5, DLY_TIME="DD_8", DLY_FB=28, DLY_MIX=20, REV_SIZE=60, REV_MIX=14)),
    ("MODE BASS", bass(cut=44, reso=38, fd=50, fs=25, O2_PW=70)),
    ("TAINTED BLIP", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=50, O2_LVL=0, FTYPE="F_LP24", CUTOFF=30, RESO=55, FENV_AMT=110, FD=32, FS=0,
                       AA=0, AD=38, AS=0, AR=18, DRIVE=15)),
    ("WANT ME SYNTH", m(O1_WAVE="W_PLS", O1_PW=55, O2_WAVE="W_SAW", O2_FINE=68, FTYPE="F_LP12", CUTOFF=85, RESO=12, FENV_AMT=64, AA=8, AS=110, AR=45,
                        LFO_PWM=40, LFO_RATE=40, SPREAD=100, CHO_MIX=45, REV_SIZE=90, REV_MIX=30)),
    ("CARS SYNTH", m(O1_WAVE="W_SAW", O2_WAVE="W_PLS", O2_PW=70, O2_FINE=71, O2_SEMI=12, FTYPE="F_LP24", CUTOFF=66, RESO=22, FENV_AMT=75, FA=20,
                     FD=60, FS=65, AA=6, AS=110, AR=40, LFO_PWM=55, LFO_RATE=34, VINTAGE=80, SPREAD=110, CHO_MIX=55, REV_SIZE=80, REV_MIX=24, DRIVE=20)),
    ("RIO SYNTH", m(SAW2, FTYPE="F_LP24", CUTOFF=60, RESO=30, FENV_AMT=95, FD=45, FS=0, AA=1, AD=62, AS=0, AR=30, ARP_MODE="A_UP", ARP_DIV="D_16", ARP_OCT=2,
                    ARP_GATE=48, DLY_TIME="DD_8D", DLY_FB=40, DLY_MIX=30, CHO_MIX=35, REV_SIZE=85, REV_MIX=22)),
    ("WEST BASS", bass(cut=50, reso=20, fd=40, fs=0, O2_LVL=40)),
    ("FOREVER PAD", strings(aa=88, ar=105, cut=58, cho=70, rev=55)),
    ("PIRATE LEAD", lead(cut=88, reso=20, glide=10, delay=24, O2_PW=58)),
    ("PIRATE POLY", pad(aa=60, ar=90, cut=60, CHO_MIX=60)),
    ("JUMP SYNTH", brass(cut=62, fenv=100, fa=10, fd=60, fs=70, aa=12, ar=40, spread=95, vint=85, cho=45)),
    ("JUMP STAB", brass(cut=70, fenv=95, fa=0, fd=40, fs=20, aa=0, ar=28, spread=95, vint=80, cho=40, AD=68, AS=0)),
    ("JUMP LEAD", lead(cut=84, reso=26, glide=16, delay=30, O2_WAVE="W_SAW", O2_FINE=72, DRIVE=15)),
    ("RESPECT SYNTH", m(MODE="M_MONO", O1_LVL=0, O2_LVL=120, SYNC=1, ENV_O2=100, O2_SEMI=31, O2_WAVE="W_SAW", FTYPE="F_LP24", CUTOFF=92, RESO=10, FD=75, FS=30,
                        AA=1, AR=40, DLY_TIME="DD_8D", DLY_FB=40, DLY_MIX=34, REV_SIZE=70, REV_MIX=22)),
    ("RESPECT BASS", bass(cut=44, reso=32, fd=48, fs=60, AD=60)),
    ("BEAT BRASS", brass(cut=64, fenv=95, fa=0, fd=45, fs=30, aa=0, ar=28, spread=60, vint=60, cho=22, AD=55, AS=40)),
    ("BILLIE BASS", bass(cut=36, reso=40, fd=42, fs=25, O2_PW=45, O2_LVL=70, DRIVE=30)),
    ("THRILLER BASS", bass(cut=34, reso=50, fd=36, fs=5, O2_WAVE="W_SAW", O2_LVL=60, DRIVE=35)),
    ("COUNTDOWN SYNTH", brass(cut=58, fenv=95, fa=30, fd=70, fs=75, aa=26, ar=55, spread=80, vint=70, cho=35, rev=32, LFO_PITCH=9, LFO_DELAY=70, LFO_RATE=72)),
    ("COUNTDOWN BRASS", brass(cut=52, fenv=105, fa=45, fd=80, fs=70, aa=34, ar=60, spread=70, vint=70, cho=30, rev=30)),
    ("COUNTDOWN PAD", strings(aa=75, ar=100, cut=56, cho=55, rev=50)),
    ("PRAYER SYNTH", brass(cut=60, fenv=95, fa=15, fd=65, fs=60, aa=14, ar=40, spread=85, vint=70, cho=40, rev=24)),
    ("RUNAWAY SYNTH", m(O1_WAVE="W_PLS", O1_PW=45, O2_WAVE="W_SAW", O2_FINE=71, FTYPE="F_LP24", CUTOFF=80, RESO=18, FENV_AMT=64, AA=5, AS=115, AR=40, LFO_PWM=50,
                        LFO_RATE=45, VINTAGE=60, SPREAD=100, CHO_MIX=65, CHO_RATE=32, DLY_TIME="DD_8D", DLY_FB=28, DLY_MIX=20, REV_SIZE=85, REV_MIX=26)),
    ("ARENA BRASS", brass(cut=66, fenv=90, fa=20, fd=70, fs=70, aa=18, ar=50, spread=100, vint=75, cho=45, rev=30)),
    ("ARENA PAD", pad(aa=95, ar=105, cut=56)),
    ("BALLAD STRINGS", strings(aa=80, ar=100, cut=56, cho=65, rev=52)),
    ("BALLAD PAD", pad(aa=98, ar=108, cut=48, CHO_MIX=60, REV_MIX=55)),
    ("LATIN STRINGS", strings(aa=60, ar=90, cut=64, cho=50, rev=36)),
    ("TRANS SYNTH", m(SAW2, FTYPE="F_LP24", CUTOFF=55, RESO=42, FENV_AMT=90, FD=40, FS=0, AA=1, AD=60, AS=0, AR=28, ARP_MODE="A_UP", ARP_DIV="D_16", ARP_OCT=2,
                      ARP_GATE=40, CHO_MIX=30, DLY_TIME="DD_8D", DLY_FB=35, DLY_MIX=24, REV_SIZE=80, REV_MIX=22)),
    ("OXYGEN LEAD", m(MODE="M_MONO", O1_WAVE="W_SAW", O2_WAVE="W_SAW", O2_FINE=70, FTYPE="F_LP24", CUTOFF=40, RESO=70, FENV_AMT=64, LFO_FLT=70, LFO_RATE=55,
                      LFO_SHAPE="L_TRI", AA=10, AS=127, AR=50, GLIDE=30, DLY_TIME="DD_4", DLY_FB=45, DLY_MIX=40, REV_SIZE=105, REV_MIX=36)),
    ("OXYGEN WIND", m(O1_WAVE="W_OFF", O2_WAVE="W_OFF", NOISE=100, FTYPE="F_BP12", RESO=40, CUTOFF=55, LFO_FLT=70, LFO_RATE=18, LFO_SHAPE="L_RND", AA=95, AR=105,
                      REV_SIZE=120, REV_MIX=55)),
    ("BLADE BRASS", brass(cut=34, fenv=118, fa=85, fd=95, fs=75, aa=60, ar=80, spread=110, vint=90, cho=55, rev=45, LFO_PITCH=7, LFO_DELAY=80)),
    ("SHINE PAD", m(SAW2, O2_SEMI=36, O2_LVL=60, FTYPE="F_LP24", CUTOFF=45, RESO=18, FENV_AMT=90, FA=100, FD=105, FS=70, AA=100, AR=110, SPREAD=127, VINTAGE=90,
                    LFO_FLT=20, LFO_RATE=20, CHO_MIX=60, REV_SIZE=120, REV_MIX=55)),
    ("PORTA LEAD", lead(cut=70, reso=35, glide=70, delay=18, O2_WAVE="W_SAW", O2_PW=64, O2_FINE=69, DRIVE=20)),
    ("MELLO STRINGS", m(O1_WAVE="W_SAW", O2_WAVE="W_PLS", O2_PW=60, O2_FINE=74, NOISE=10, FTYPE="F_LP12", CUTOFF=48, RESO=4, FENV_AMT=64, AA=50, AS=120, AR=70,
                       LFO_PITCH=8, LFO_RATE=62, VINTAGE=100, SPREAD=100, CHO_MIX=40, REV_SIZE=80, REV_MIX=25)),
    ("POPCORN SYNTH", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=30, O2_LVL=0, FTYPE="F_LP12", CUTOFF=80, RESO=30, FENV_AMT=76, FD=46, FS=10, AA=0, AD=60, AS=10, AR=20,
                        DLY_TIME="DD_8", DLY_FB=22, DLY_MIX=14)),
    ("AXEL LEAD", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=48, O2_WAVE="W_SAW", O2_FINE=69, FTYPE="F_LP24", CUTOFF=70, RESO=28, FENV_AMT=88, FD=44, FS=35, AA=1,
                    AD=60, AS=60, AR=28, DLY_TIME="DD_8D", DLY_FB=32, DLY_MIX=28, CHO_MIX=30, REV_SIZE=70, REV_MIX=18)),
    ("CHIP LEAD", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=32, O2_LVL=0, FTYPE="F_LP12", CUTOFF=127, RESO=0, FENV_AMT=64, VEL_AMP=0, AA=0, AS=127, AR=6, GLIDE=0)),
    ("CHIP BASS", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=64, O2_LVL=0, O1_LVL=110, FTYPE="F_LP12", CUTOFF=60, RESO=0, FENV_AMT=64, VEL_AMP=0, AA=0, AD=45, AS=100, AR=8)),
    ("DISCO STRINGS", strings(aa=45, ar=80, cut=66, cho=70, rev=30)),
    ("DISCO BRASS", brass(cut=68, fenv=95, fa=5, fd=55, fs=60, aa=6, ar=34, spread=70, vint=40, cho=30, rev=18)),
    ("SAWYER LEAD", lead(cut=72, reso=40, glide=34, delay=20, O2_WAVE="W_SAW", O2_PW=64, O2_FINE=68, DRIVE=25, SYNC=0)),
    ("KASHMIR ORCH", m(SAW2, O2_SEMI=12, O2_LVL=80, FTYPE="F_LP24", CUTOFF=44, RESO=10, FENV_AMT=90, FA=75, FD=100, FS=80, AA=60, AS=120, AR=70, SPREAD=100, VINTAGE=80,
                       CHO_MIX=40, REV_SIZE=110, REV_MIX=45)),
    ("ORCH HIT SYNTH", m(SAW2, O2_SEMI=36, O2_LVL=90, NOISE=40, FTYPE="F_LP12", CUTOFF=78, RESO=10, FENV_AMT=90, FD=50, FS=0, AA=0, AD=70, AS=0, AR=34, SPREAD=100,
                         REV_SIZE=95, REV_MIX=40)),
    ("TRIUMPH BRASS", brass(cut=70, fenv=90, fa=20, fd=70, fs=75, aa=16, ar=45, spread=60, vint=50, cho=15, rev=40)),
    ("SYNTH CHOIR", m(O1_WAVE="W_PLS", O1_PW=60, O2_WAVE="W_SAW", O2_FINE=73, FTYPE="F_BP12", CUTOFF=60, RESO=28, FENV_AMT=64, AA=75, AS=120, AR=95, LFO_PITCH=6,
                      LFO_DELAY=70, LFO_RATE=64, SPREAD=100, VINTAGE=80, CHO_MIX=70, REV_SIZE=115, REV_MIX=50)),
    ("DREAM ARP", m(SAW2, FTYPE="F_LP24", CUTOFF=58, RESO=20, FENV_AMT=95, FD=50, FS=0, AA=1, AD=66, AS=0, AR=34, ARP_MODE="A_UPDN", ARP_DIV="D_8T", ARP_OCT=2,
                    ARP_GATE=60, DLY_TIME="DD_4D", DLY_FB=45, DLY_MIX=38, CHO_MIX=40, REV_SIZE=105, REV_MIX=40)),
    ("SWEEP BRASS", brass(cut=30, fenv=127, fa=100, fd=110, fs=90, aa=40, ar=60, spread=90, vint=80, cho=40, rev=34)),
    ("DANCE BELLS SYN", m(O1_WAVE="W_PLS", O1_PW=30, O2_WAVE="W_PLS", O2_SEMI=43, O2_LVL=70, XMOD=60, FTYPE="F_LP24", CUTOFF=100, RESO=5, FENV_AMT=94, FD=85, FS=0,
                          AA=0, AD=92, AS=0, AR=80, DLY_TIME="DD_8", DLY_FB=40, DLY_MIX=36, REV_SIZE=105, REV_MIX=42)),
]

# ---- FM2 (4-operator FM): (name, [ALG, R2, R3, R4, IDX, MDEC, FB, -], env A D S R, fx DIST CHORUS DELAY REVERB) -------------------
FM2 = [
    ("NIGHT HARPSI", [3, 1, 4, 6, 92, 20, 5, 0], [0, 72, 0, 30], (0, 8, 8, 40)),
    ("BAROQUE HARPSI", [4, 2, 3, 5, 100, 16, 0, 0], [0, 66, 0, 24], (0, 4, 6, 30)),
    ("NIGHT BELLS", [4, 6, 1, 13, 45, 92, 0, 0], [0, 100, 0, 100], (0, 20, 30, 70)),
    ("NIGHT FANTASY", [4, 3, 1, 6, 55, 98, 6, 0], [45, 100, 127, 110], (0, 45, 25, 72)),
    ("NIGHT CELESTA", [4, 2, 1, 4, 30, 55, 0, 0], [0, 96, 0, 90], (0, 12, 22, 60)),
    ("NIGHT HARP", [4, 1, 1, 2, 45, 30, 10, 0], [0, 90, 0, 80], (0, 18, 20, 56)),
    ("STORM EP", [4, 1, 1, 14, 30, 32, 0, 0], [0, 80, 35, 60], (0, 60, 10, 20)),
    ("DOOR KEY BASS", [4, 1, 1, 2, 38, 26, 6, 0], [0, 60, 20, 30], (0, 8, 0, 8)),
    ("HOUSE PIANO 91", [4, 1, 2, 13, 70, 26, 12, 0], [0, 84, 0, 55], (0, 16, 10, 22)),
    ("ITALO PIANO", [4, 1, 2, 12, 74, 30, 12, 0], [0, 86, 0, 58], (0, 35, 12, 24)),
    ("SHEPHERD PAN", [4, 1, 1, 1, 10, 70, 18, 0], [25, 90, 127, 60], (0, 0, 20, 60)),
    ("MODE METAL", [4, 7, 1, 11, 70, 40, 0, 0], [0, 70, 0, 45], (0, 10, 18, 26)),
    ("WEST STAB", [5, 1, 2, 3, 110, 15, 8, 0], [0, 70, 0, 30], (0, 8, 10, 30)),
    ("FOREVER BELL", [4, 3, 1, 6, 45, 60, 0, 0], [0, 88, 0, 75], (0, 14, 34, 50)),
    ("RESPECT BELLS", [4, 5, 1, 8, 50, 45, 0, 0], [0, 80, 0, 66], (0, 10, 30, 36)),
    ("BEAT HIT", [4, 8, 1, 4, 98, 28, 0, 0], [0, 68, 0, 34], (0, 10, 14, 16)),
    ("BEAT METAL", [4, 9, 2, 12, 80, 34, 4, 0], [0, 74, 0, 40], (0, 8, 12, 20)),
    ("ORCH HIT 80S", [5, 1, 2, 3, 120, 14, 10, 0], [0, 72, 0, 34], (0, 6, 8, 40)),
    ("BALLAD EP", [4, 1, 1, 14, 22, 40, 0, 0], [0, 90, 30, 70], (0, 45, 25, 35)),
    ("LATIN EP", [4, 1, 1, 14, 34, 34, 2, 0], [0, 82, 30, 55], (0, 25, 14, 22)),
    ("SLOW DANCE EP", [4, 1, 1, 13, 18, 44, 0, 0], [0, 92, 30, 75], (0, 50, 22, 40)),
    ("FANTASY BELL", [4, 5, 1, 8, 40, 95, 0, 0], [55, 100, 127, 105], (0, 40, 30, 66)),
    ("LATIN NYLON", [4, 1, 1, 2, 35, 28, 8, 0], [0, 72, 0, 50], (0, 14, 14, 24)),
    ("XMAS BELLS", [4, 6, 1, 9, 40, 70, 0, 0], [0, 90, 0, 80], (0, 12, 26, 50)),
    ("AFRICA KALIMBA", [4, 4, 1, 6, 52, 24, 0, 0], [0, 78, 0, 58], (0, 0, 26, 38)),
]

# ---- ORG (drawbar organ): (name, [REG, SUB, BODY, TOP, PERC, CLICK, DRIVE, ROTR], env, fx) ------------------------------------------
ORG = [
    ("HIGHWAY ORGAN", [7, 0, 0, 0, 0, 60, 100, 2], [0, 64, 127, 35], (30, 0, 6, 16)),
    ("SMOKE ORGAN", [5, 0, 0, 1, 1, 70, 80, 2], [0, 64, 127, 36], (30, 0, 6, 18)),
    ("BURN ORGAN", [8, 0, 0, 0, 0, 80, 110, 2], [0, 64, 127, 32], (40, 0, 6, 16)),
    ("CHILD ORGAN", [12, 0, 0, 0, 0, 10, 10, 1], [25, 64, 127, 70], (0, 8, 0, 44)),
    ("PALE ORGAN", [4, 0, 0, 0, 0, 25, 20, 1], [8, 64, 127, 60], (0, 10, 0, 50)),
    ("MAGIC ORGAN", [6, 0, 0, 1, 1, 60, 50, 2], [0, 64, 127, 40], (20, 0, 6, 24)),
    ("TOCCATA ORGAN", [15, 0, 0, 0, 0, 10, 15, 0], [5, 64, 127, 70], (0, 0, 0, 76)),
    ("CATHEDRAL ORGAN", [12, 0, 0, 0, 0, 5, 0, 0], [20, 64, 127, 90], (0, 0, 0, 90)),
    ("PIRATE ORGAN", [5, 0, 0, 0, 1, 60, 40, 1], [0, 64, 127, 40], (10, 0, 6, 20)),
    ("LATIN ORGAN", [2, 0, 0, 0, 0, 40, 15, 1], [0, 64, 127, 36], (0, 0, 6, 22)),
]

# ---- SP (sample sets PIANO HORNS STRGS FLUTE VOX GLASS PAD AIR): (name, set, CUT, DRV, BITS, env, mono, fx) -------------------
SP = [
    ("RHAPSODY PIANO", "PIANO", 127, 0, 0, [0, 110, 0, 70], 0, (0, 6, 6, 40)),
    ("NIGHT PIANO", "PIANO", 120, 0, 0, [0, 110, 0, 75], 0, (0, 10, 8, 55)),
    ("CLASSIC PIANO", "PIANO", 110, 0, 0, [0, 100, 0, 55], 0, (0, 4, 4, 26)),
    ("MOON PIANO", "PIANO", 60, 0, 0, [0, 115, 0, 80], 0, (0, 8, 8, 50)),
    ("CONCERT PIANO", "PIANO", 127, 8, 0, [0, 105, 0, 60], 0, (0, 4, 4, 34)),
    ("LATIN PIANO", "PIANO", 110, 0, 0, [0, 105, 0, 45], 0, (0, 15, 6, 20)),
    ("ROMANCE PIANO", "PIANO", 85, 0, 0, [0, 112, 0, 70], 0, (0, 30, 10, 40)),
    ("SONG PIANO", "PIANO", 100, 0, 0, [0, 108, 0, 60], 0, (0, 8, 4, 28)),
    ("BELIEVE PIANO", "PIANO", 120, 6, 0, [0, 95, 0, 40], 0, (0, 12, 6, 22)),
    ("DANCING PIANO", "PIANO", 127, 0, 0, [0, 100, 0, 45], 0, (0, 20, 10, 24)),
    ("BAR PIANO", "PIANO", 90, 40, 0, [0, 90, 0, 40], 0, (0, 0, 0, 18)),
    ("NIGHT CHOIR", "VOX", 105, 0, 0, [70, 100, 127, 100], 0, (0, 40, 14, 66)),
    ("NIGHT OPERA", "AIR", 100, 0, 0, [55, 100, 127, 100], 0, (0, 30, 16, 62)),
    ("RHAPSODY CHOIR", "VOX", 110, 0, 0, [60, 100, 127, 96], 0, (0, 36, 12, 60)),
    ("MELLO CHOIR", "VOX", 90, 0, 30, [50, 100, 127, 80], 0, (0, 30, 12, 40)),
    ("MELLO FLUTE", "FLUTE", 85, 0, 28, [25, 90, 127, 55], 0, (0, 12, 14, 30)),
    ("FLUTE SOLO", "FLUTE", 100, 0, 0, [22, 90, 127, 55], 0, (0, 8, 10, 34)),
    ("ROCK FLUTE", "FLUTE", 110, 45, 0, [18, 90, 127, 45], 0, (0, 0, 14, 22)),
    ("NIGHT FLUTE", "FLUTE", 100, 0, 0, [25, 90, 127, 60], 0, (0, 10, 16, 52)),
    ("ANDES PAN FLUTE", "FLUTE", 75, 0, 0, [12, 90, 127, 40], 0, (0, 0, 14, 34)),
    ("NIGHT HORNS", "HORNS", 100, 0, 0, [25, 100, 127, 50], 0, (0, 14, 10, 44)),
    ("STAR BRASS", "HORNS", 127, 10, 0, [8, 100, 127, 40], 0, (0, 8, 8, 34)),
    ("AFRICA CHOIR", "GLASS", 100, 0, 0, [60, 100, 127, 90], 0, (0, 30, 20, 56)),
    ("NIGHT ORCH PAD", "STRGS", 95, 0, 0, [40, 100, 127, 80], 0, (0, 24, 10, 50)),
]

# ---- FM6 (six-operator FM, Clipper's eight voices): (name, [ALG FB MLVL MRAT MEG VMOD DTUN PTCH], env, fx) ------------------------
# PTCH: 0 TINE EP, 1 GLASS BELL, 2 ROUND BASS, 3 BRASS SECT, 4 SOFT PAD, 5 WOOD BARS, 6 DRAWBARS, 7 NYLON PICK
FM6 = [
    ("AFRICA MARIMBA", [0, 0, 0, 0, -20, 0, 0, 5], [0, 64, 127, 110], (0, 0, 28, 44)),
    ("SLOW EP", [0, 0, -10, 0, 10, 0, 10, 0], [0, 64, 127, 110], (0, 50, 24, 40)),
    ("TRIUMPH HORNS", [0, 0, 10, 0, -10, 0, 0, 3], [0, 64, 127, 100], (0, 18, 14, 40)),
    ("CRYSTAL CHIME", [0, 0, 8, 0, 10, 0, 0, 1], [0, 64, 127, 110], (0, 10, 30, 70)),
]


# the order of the list in the instrument (grouped by what you reach for: keys, organs, bells, voices, pads, brass, leads, basses)
ORDER = """RHAPSODY PIANO|NIGHT PIANO|CLASSIC PIANO|MOON PIANO|CONCERT PIANO|ROMANCE PIANO|SONG PIANO|LATIN PIANO|BELIEVE PIANO|DANCING PIANO|BAR PIANO|
HOUSE PIANO 91|ITALO PIANO|SLOW EP|BALLAD EP|SLOW DANCE EP|LATIN EP|STORM EP|
HIGHWAY ORGAN|SMOKE ORGAN|BURN ORGAN|DOOR ORGAN|FIRE ORGAN|MAGIC ORGAN|PIRATE ORGAN|LATIN ORGAN|LATIN NYLON|CHILD ORGAN|PALE ORGAN|TOCCATA ORGAN|CATHEDRAL ORGAN|DOOR KEY BASS|
NIGHT HARPSI|BAROQUE HARPSI|NIGHT BELLS|NIGHT CELESTA|NIGHT HARP|NIGHT FANTASY|FANTASY BELL|FOREVER BELL|RESPECT BELLS|XMAS BELLS|CRYSTAL CHIME|AFRICA KALIMBA|
AFRICA MARIMBA|DANCE BELLS SYN|
NIGHT CHOIR|NIGHT OPERA|RHAPSODY CHOIR|MELLO CHOIR|AFRICA CHOIR|SYNTH CHOIR|FLUTE SOLO|MELLO FLUTE|ROCK FLUTE|NIGHT FLUTE|ANDES PAN FLUTE|SHEPHERD PAN|
NIGHT STRINGS|NIGHT ORCH PAD|KASHMIR ORCH|MELLO STRINGS|FOREVER PAD|BALLAD STRINGS|BALLAD PAD|LATIN STRINGS|DISCO STRINGS|COUNTDOWN PAD|ARENA PAD|SHINE PAD|
PIRATE POLY|OXYGEN WIND|
JUMP SYNTH|JUMP STAB|COUNTDOWN SYNTH|COUNTDOWN BRASS|ARENA BRASS|BEAT BRASS|TRIUMPH BRASS|TRIUMPH HORNS|PRAYER SYNTH|BLADE BRASS|SWEEP BRASS|DISCO BRASS|
NIGHT HORNS|STAR BRASS|ORCH HIT 80S|ORCH HIT SYNTH|WEST STAB|RAVE STAB|BEAT HIT|BEAT METAL|
TAKE ON SYNTH|FREE SYNTH LEAD|NIGHT LEAD|NIGHT LEAD|JUMP LEAD|ENJOY LEAD|ENOUGH SQUARE|RESPECT SYNTH|PIRATE LEAD|AXEL LEAD|PORTA LEAD|SAWYER LEAD|OXYGEN LEAD|
POPCORN SYNTH|CHIP LEAD|TAINTED BLIP|RUNAWAY SYNTH|WANT ME SYNTH|CARS SYNTH|RIO SYNTH|SWEET DREAM SYN|TRANS SYNTH|DREAM ARP|MODE METAL|
TAKE ON BASS|FREE SYNTH BASS|HOUSE BASS|SWEET DREAM BAS|MODE BASS|WEST BASS|RESPECT BASS|BILLIE BASS|THRILLER BASS|CHIP BASS""".replace("\n", "").split("|")
ORDER = [x for i, x in enumerate(ORDER) if x not in ORDER[:i]]


# ---- second round: rock, pop rock, rock nacional, Latin rock (ids after the first round, so earlier ids stay put) ---------------------
SYN += [
    ("POWER CHORD", m(SAW2, O2_SEMI=31, O2_LVL=100, FTYPE="F_LP24", CUTOFF=70, RESO=8, FENV_AMT=64, AA=1, AS=127, AR=25, DRIVE=90, CHO_MIX=15, REV_SIZE=70, REV_MIX=14)),
    ("POWER LEAD", lead(cut=80, reso=15, glide=10, delay=22, DRIVE=70, AS=127)),
    ("SOLO SUSTAIN", lead(cut=70, reso=10, glide=22, delay=26, DRIVE=45, LFO_PITCH=8, LFO_DELAY=90, AS=127)),
    ("SMOOTH SOLO", lead(cut=60, reso=18, glide=40, delay=30, DRIVE=30, AS=127)),
    ("PROG LEAD", lead(cut=70, reso=30, glide=50, delay=10, O2_FINE=66, AS=127)),
    ("WAYS RIFF", brass(cut=68, fenv=95, fa=5, fd=60, fs=70, aa=6, ar=34, spread=100, vint=70, cho=40, rev=24)),
    ("TIGER SYNTH", brass(cut=66, fenv=100, fa=0, fd=50, fs=30, aa=0, ar=26, spread=90, vint=80, cho=30, AD=70, AS=50)),
    ("URGENT SYNTH", brass(cut=56, fenv=95, fa=10, fd=60, fs=65, aa=8, ar=36, spread=60, vint=60, cho=15)),
    ("SHOUT SYNTH", brass(cut=60, fenv=90, fa=25, fd=75, fs=70, aa=30, ar=60, spread=100, vint=75, cho=50, rev=34)),
    ("SUSSUDIO BRASS", brass(cut=70, fenv=98, fa=0, fd=45, fs=40, aa=0, ar=30, spread=70, vint=60, cho=20, AD=70, AS=55)),
    ("SKA BRASS", brass(cut=66, fenv=95, fa=0, fd=38, fs=0, aa=0, ar=22, spread=50, vint=50, cho=10, AD=70, AS=0)),
    ("FUNK BRASS", brass(cut=62, fenv=100, fa=0, fd=42, fs=10, aa=0, ar=24, spread=60, vint=55, cho=12, AD=70, AS=10)),
    ("SOFT BRASS", brass(cut=48, fenv=90, fa=30, fd=80, fs=80, aa=30, ar=60, spread=70, vint=60, cho=30, rev=36)),
    ("ROCK STRINGS", strings(aa=55, ar=90, cut=70, cho=40, rev=40)),
    ("RAIN STRINGS", strings(aa=90, ar=105, cut=54, cho=60, rev=60)),
    ("GOTHIC STRINGS", strings(aa=30, ar=70, cut=50, cho=40, rev=60, AD=50, AS=70)),
    ("VIVA STRINGS", strings(aa=2, ar=40, cut=66, cho=30, rev=36, AD=48, AS=0)),
    ("EPITAPH STRINGS", strings(aa=60, ar=95, cut=58, cho=55, rev=44)),
    ("ROMANTIC PAD", pad(aa=95, ar=105, cut=60, REV_MIX=50)),
    ("HEAVEN PAD", pad(aa=75, ar=100, cut=66, CHO_MIX=100, CHO_RATE=20, REV_MIX=50)),
    ("SPACE PAD", pad(aa=110, ar=112, cut=44, REV_MIX=60)),
    ("AIR TONIGHT PAD", pad(aa=70, ar=110, cut=36, REV_SIZE=127, REV_MIX=65)),
    ("PURPLE PAD", pad(aa=60, ar=95, cut=58, CHO_MIX=70)),
    ("WAVE SYNTH", pluck(cut=70, fd=70, ad=80, CHO_MIX=55, CHO_RATE=30, SPREAD=90)),
    ("SUMMER SYNTH", pluck(cut=74, fd=50, ad=70, DLY_TIME="DD_8D", DLY_FB=30, DLY_MIX=22, SPREAD=70)),
    ("NEW WAVE LEAD", lead(cut=84, reso=20, glide=0, delay=22, O2_PW=64)),
    ("LONDON LEAD", lead(cut=90, reso=14, glide=6, delay=30)),
    ("TITAN SYNTH", m(O1_WAVE="W_PLS", O1_PW=40, O2_WAVE="W_SAW", O2_FINE=70, FTYPE="F_LP24", CUTOFF=74, RESO=20, FENV_AMT=80, FD=55, FS=50, AA=2, AS=100, AR=34,
                      VINTAGE=60, SPREAD=90, CHO_MIX=40, DLY_TIME="DD_8", DLY_FB=28, DLY_MIX=20, REV_SIZE=70, REV_MIX=18)),
    ("CLOCKS ARP", m(SAW2, FTYPE="F_LP24", CUTOFF=84, RESO=10, FENV_AMT=80, FD=60, FS=20, AA=1, AD=70, AS=0, AR=40, ARP_MODE="A_UP", ARP_DIV="D_8", ARP_OCT=1,
                     ARP_GATE=70, DLY_TIME="DD_8D", DLY_FB=30, DLY_MIX=20, REV_SIZE=80, REV_MIX=24)),
    ("FAST ARP", m(SAW2, FTYPE="F_LP24", CUTOFF=60, RESO=40, FENV_AMT=90, FD=45, FS=0, AA=0, AD=60, AS=0, AR=26, DRIVE=40, ARP_MODE="A_UP", ARP_DIV="D_16", ARP_OCT=3,
                   ARP_GATE=50, REV_SIZE=70, REV_MIX=18)),
    ("ALIVE ARP", m(SAW2, FTYPE="F_LP24", CUTOFF=66, RESO=20, FENV_AMT=88, FD=55, FS=0, AA=1, AD=66, AS=0, AR=34, ARP_MODE="A_UPDN", ARP_DIV="D_16", ARP_OCT=2,
                    ARP_GATE=60, CHO_MIX=40, DLY_TIME="DD_8D", DLY_FB=38, DLY_MIX=30, REV_SIZE=90, REV_MIX=30)),
    ("ENGINEER ARP", pluck(cut=70, fd=48, ad=66, ARP_MODE="A_UP", ARP_DIV="D_8", ARP_OCT=2, ARP_GATE=60)),
    ("MONDAY BASS", bass(cut=44, reso=20, fd=50, fs=0, O2_PW=50, AD=48)),
    ("DRIVE BASS", bass(cut=60, reso=10, fd=40, fs=60, DRIVE=80, O2_LVL=70)),
    ("ROCK BASS", bass(cut=52, reso=8, fd=45, fs=50, DRIVE=45, O2_WAVE="W_SAW")),
    ("FUNK BASS", bass(cut=48, reso=35, fd=38, fs=0, AD=40)),
    ("LONDON BASS", bass(cut=48, reso=28, fd=48, fs=30)),
    ("WIND WHISTLE", lead(cut=55, reso=0, glide=14, delay=24, LFO_PITCH=10, LFO_DELAY=70, O2_LVL=0, O1_WAVE="W_BOTH", AA=14, AS=127)),
]
FM2 += [
    ("CLAV FUNK", [4, 1, 3, 4, 85, 14, 6, 0], [0, 40, 0, 20], (0, 6, 0, 10)),
    ("REED EP ROCK", [4, 1, 1, 14, 40, 30, 8, 0], [0, 78, 25, 50], (30, 20, 10, 20)),
    ("EDGE ECHO", [4, 1, 2, 3, 50, 30, 8, 0], [0, 66, 0, 40], (0, 14, 60, 36)),
    ("CLEAN CHORUS", [4, 1, 1, 2, 32, 26, 4, 0], [0, 74, 0, 50], (0, 70, 10, 30)),
    ("TWELVE STRING", [4, 1, 2, 3, 52, 30, 10, 0], [0, 76, 0, 56], (0, 60, 10, 28)),
    ("STEEL ARENA", [4, 1, 2, 3, 58, 34, 10, 0], [0, 80, 0, 60], (0, 30, 14, 40)),
    ("VIOLA CAIPIRA", [4, 1, 3, 3, 62, 26, 12, 0], [0, 70, 0, 48], (0, 40, 10, 26)),
    ("CHARANGO", [4, 2, 1, 3, 60, 24, 6, 0], [0, 58, 0, 36], (0, 30, 8, 20)),
    ("OVERDRIVE GTR", [4, 1, 2, 3, 56, 22, 14, 0], [0, 84, 40, 40], (40, 10, 14, 20)),
    ("POLICE GTR", [4, 1, 1, 2, 38, 22, 6, 0], [0, 60, 0, 40], (0, 60, 30, 26)),
    ("OFFBEAT KEYS", [4, 1, 1, 2, 48, 18, 4, 0], [0, 36, 0, 16], (0, 10, 18, 14)),
    ("AGOGO BELL", [4, 7, 1, 11, 70, 20, 0, 0], [0, 40, 0, 22], (0, 0, 8, 20)),
    ("FUNK SLAP", [4, 1, 2, 1, 70, 14, 10, 0], [0, 36, 0, 18], (0, 0, 0, 6)),
    ("PICK BASS", [4, 1, 1, 1, 40, 20, 6, 0], [0, 50, 10, 20], (10, 0, 0, 6)),
]
ORG += [
    ("BROTHERS ORGAN", [4, 0, 0, 0, 0, 25, 15, 1], [8, 64, 127, 50], (0, 10, 0, 36)),
    ("LATIN BLUES ORG", [6, 0, 0, 1, 2, 60, 40, 2], [0, 64, 127, 40], (15, 0, 6, 22)),
    ("SKA ORGAN", [3, 0, 0, 0, 2, 50, 10, 0], [0, 48, 0, 22], (0, 0, 10, 18)),
    ("BUBBLE ORGAN", [2, 0, 0, 0, 0, 60, 10, 0], [0, 56, 0, 20], (0, 0, 8, 14)),
    ("FORRO ACCORDION", [9, 0, 0, 1, 0, 8, 20, 0], [6, 64, 127, 35], (0, 45, 0, 24)),
    ("XOTE ACCORD", [10, 0, 2, 0, 0, 5, 10, 0], [8, 64, 127, 35], (0, 55, 0, 22)),
    ("PSYCH ORGAN", [10, 0, 0, 2, 0, 70, 60, 0], [0, 64, 127, 36], (25, 0, 6, 20)),
    ("BARON ORGAN", [7, 0, 0, 0, 1, 60, 60, 2], [0, 64, 127, 40], (25, 0, 6, 20)),
    ("NEW WAVE ORGAN", [13, 0, 0, 0, 1, 40, 10, 0], [0, 64, 127, 36], (0, 25, 10, 22)),
    ("STADIUM ORGAN", [15, 0, 0, 0, 0, 40, 60, 2], [0, 64, 127, 40], (30, 0, 6, 36)),
]
SP += [
    ("ROCK PIANO", "PIANO", 127, 12, 0, [0, 100, 0, 50], 0, (0, 8, 6, 22)),
    ("HONKY PIANO", "PIANO", 95, 55, 0, [0, 92, 0, 36], 0, (0, 35, 0, 16)),
    ("STADIUM PIANO", "PIANO", 127, 5, 0, [0, 108, 0, 70], 0, (0, 18, 10, 50)),
    ("POWER PIANO", "PIANO", 120, 10, 0, [0, 106, 0, 64], 0, (0, 12, 8, 40)),
    ("HOLD PIANO", "PIANO", 127, 15, 0, [0, 84, 0, 34], 0, (0, 6, 4, 18)),
    ("CLOCKS PIANO", "PIANO", 118, 0, 0, [0, 96, 0, 50], 0, (0, 8, 40, 36)),
    ("RAIN PIANO", "PIANO", 105, 0, 0, [0, 112, 0, 86], 0, (0, 10, 10, 64)),
    ("LEGION PIANO", "PIANO", 92, 0, 0, [0, 108, 0, 62], 0, (0, 12, 6, 36)),
    ("BARON PIANO", "PIANO", 127, 20, 0, [0, 104, 0, 50], 0, (0, 8, 4, 20)),
    ("HAWAII PIANO", "PIANO", 110, 0, 0, [0, 100, 0, 54], 0, (0, 22, 12, 28)),
    ("GOTHIC PIANO", "PIANO", 70, 0, 0, [0, 114, 0, 84], 0, (0, 10, 14, 72)),
    ("GOTHIC CHOIR", "VOX", 80, 0, 0, [75, 100, 127, 105], 0, (0, 30, 10, 72)),
    ("BALLAD CHOIR", "VOX", 100, 0, 0, [55, 100, 127, 95], 0, (0, 35, 14, 52)),
    ("RECORDER", "FLUTE", 127, 0, 0, [10, 90, 127, 40], 0, (0, 0, 6, 20)),
    ("TAPE FLUTE", "FLUTE", 80, 0, 0, [30, 90, 127, 60], 0, (0, 18, 16, 36)),
    ("QUENA FLUTE", "FLUTE", 90, 0, 0, [14, 90, 127, 45], 0, (0, 0, 12, 30)),
    ("SHAKU FLUTE", "AIR", 110, 0, 0, [20, 90, 127, 50], 1, (0, 0, 16, 34)),
    ("ROCK HORNS", "HORNS", 127, 45, 0, [6, 100, 127, 36], 0, (0, 8, 6, 24)),
    ("SOFT HORNS", "HORNS", 85, 0, 0, [30, 100, 127, 55], 0, (0, 16, 10, 40)),
    ("SOFT SECTION", "STRGS", 90, 0, 0, [50, 100, 127, 80], 0, (0, 28, 10, 46)),
]
FM6 += [
    ("NATURE EP", [0, 0, -6, 0, 5, 0, 10, 0], [0, 64, 127, 110], (0, 55, 22, 44)),
    ("SUPER EP", [0, 0, 6, 0, 0, 0, 0, 0], [0, 64, 127, 100], (20, 30, 10, 24)),
    ("FUNK HORNS", [0, 0, 15, 0, -5, 0, 0, 3], [0, 64, 127, 60], (0, 10, 10, 24)),
    ("ROCK FM BASS", [0, 0, 0, 0, 0, 0, 0, 2], [0, 64, 127, 70], (20, 0, 0, 6)),
]
# the second round in listing order: keys, guitars, organs, winds, brass, strings and pads, leads and arps, basses
ORDER += """ROCK PIANO|HONKY PIANO|STADIUM PIANO|POWER PIANO|HOLD PIANO|CLOCKS PIANO|RAIN PIANO|LEGION PIANO|BARON PIANO|HAWAII PIANO|GOTHIC PIANO|
NATURE EP|SUPER EP|REED EP ROCK|CLAV FUNK|OFFBEAT KEYS|
CLEAN CHORUS|POLICE GTR|EDGE ECHO|TWELVE STRING|STEEL ARENA|OVERDRIVE GTR|VIOLA CAIPIRA|CHARANGO|
BROTHERS ORGAN|LATIN BLUES ORG|BARON ORGAN|PSYCH ORGAN|NEW WAVE ORGAN|STADIUM ORGAN|SKA ORGAN|BUBBLE ORGAN|FORRO ACCORDION|XOTE ACCORD|
RECORDER|TAPE FLUTE|QUENA FLUTE|SHAKU FLUTE|WIND WHISTLE|AGOGO BELL|
ROCK HORNS|SOFT HORNS|FUNK HORNS|SKA BRASS|FUNK BRASS|SOFT BRASS|SUSSUDIO BRASS|URGENT SYNTH|SHOUT SYNTH|TIGER SYNTH|WAYS RIFF|
ROCK STRINGS|RAIN STRINGS|GOTHIC STRINGS|VIVA STRINGS|EPITAPH STRINGS|SOFT SECTION|ROMANTIC PAD|HEAVEN PAD|SPACE PAD|AIR TONIGHT PAD|PURPLE PAD|
GOTHIC CHOIR|BALLAD CHOIR|
POWER CHORD|POWER LEAD|SOLO SUSTAIN|SMOOTH SOLO|PROG LEAD|NEW WAVE LEAD|LONDON LEAD|TITAN SYNTH|WAVE SYNTH|SUMMER SYNTH|
CLOCKS ARP|FAST ARP|ALIVE ARP|ENGINEER ARP|
MONDAY BASS|DRIVE BASS|ROCK BASS|ROCK FM BASS|FUNK BASS|FUNK SLAP|PICK BASS|LONDON BASS""".replace("\n", "").split("|")
ORDER = [x for i, x in enumerate(ORDER) if x not in ORDER[:i]]


# ---- third round: specific requests (Supertramp / Rita Lee EP, Europe and Heart ballads, Bonnie Tyler piano, Michael Jackson) -------------
SYN += [
    ("BILLIE SYNTH", m(SAW2, O2_WAVE="W_PLS", O2_PW=60, FTYPE="F_LP24", CUTOFF=58, RESO=26, FENV_AMT=96, FD=64, FS=20, AA=0, AD=70, AS=40, AR=40, VINTAGE=70,
                       SPREAD=80, CHO_MIX=45, CHO_RATE=26, DLY_TIME="DD_8D", DLY_FB=26, DLY_MIX=18, REV_SIZE=80, REV_MIX=22)),
    ("HUMAN PAD", pad(aa=80, ar=105, cut=70, CHO_MIX=65, REV_MIX=50)),
]
FM2 += [
    ("DREAMER EP", [4, 1, 1, 14, 55, 36, 14, 0], [0, 70, 20, 40], (20, 25, 8, 14)),
    ("PERFUME EP", [4, 1, 1, 14, 28, 34, 0, 0], [0, 86, 30, 60], (0, 55, 20, 30)),
    ("ALONE EP", [4, 1, 1, 13, 16, 40, 0, 0], [10, 90, 40, 80], (0, 70, 24, 55)),
    ("HUMAN PLUCK", [4, 1, 1, 6, 40, 40, 0, 0], [0, 70, 0, 60], (0, 30, 28, 40)),
]
SP += [
    ("ECLIPSE PIANO", "PIANO", 120, 6, 0, [0, 110, 0, 82], 0, (0, 14, 12, 70)),
]
FM6 += [
    ("CARRIE EP", [0, 0, -14, 0, 4, 0, 12, 0], [0, 64, 127, 110], (0, 60, 20, 56)),
]
ORDER += "DREAMER EP|PERFUME EP|CARRIE EP|ALONE EP|ECLIPSE PIANO|BILLIE SYNTH|HUMAN PAD|HUMAN PLUCK".split("|")


# ---- fourth round: requests by song (Paralamas brass, Linkin Park, Shaman, 80s pop and new wave, Foreigner, sax) ----------------------------
SYN += [
    ("BRAZIL HORNS", brass(cut=64, fenv=98, fa=8, fd=64, fs=66, aa=10, ar=38, spread=70, vint=60, cho=25, rev=22)),
    ("BRAZIL STAB", brass(cut=68, fenv=96, fa=0, fd=60, fs=10, aa=0, ar=26, spread=60, vint=55, cho=15, AD=72, AS=15)),
    ("NUMB PAD", pad(aa=75, ar=100, cut=40, REV_MIX=50, CHO_MIX=40)),
    ("FAINT STRINGS", strings(aa=14, ar=50, cut=62, cho=30, rev=34, MODE="M_MONO", GLIDE=38, AD=70, AS=100)),
    ("FAINT CELLO", strings(aa=24, ar=60, cut=46, cho=25, rev=36, MODE="M_MONO", GLIDE=44, O2_SEMI=12, LFO_PITCH=10)),
    ("RITUAL SYNTH", m(O1_WAVE="W_SAW", O2_WAVE="W_PLS", O2_PW=70, O2_FINE=71, FTYPE="F_LP24", CUTOFF=40, RESO=38, FENV_AMT=96, FA=85, FD=100, FS=70, AA=80, AD=100, AS=110,
                       AR=95, LFO_FLT=40, LFO_RATE=24, VINTAGE=85, SPREAD=100, CHO_MIX=35, DLY_TIME="DD_4D", DLY_FB=45, DLY_MIX=34, REV_SIZE=120, REV_MIX=55)),
    ("FUN BASS", bass(cut=46, reso=32, fd=48, fs=40, O2_PW=45)),
    ("MANIAC SYNTH", m(O1_WAVE="W_PLS", O1_PW=36, O2_WAVE="W_SAW", O2_FINE=70, FTYPE="F_LP24", CUTOFF=72, RESO=34, FENV_AMT=90, FD=48, FS=0, AA=0, AD=60, AS=0, AR=26,
                       ARP_MODE="A_UP", ARP_DIV="D_16", ARP_OCT=2, ARP_GATE=50, DLY_TIME="DD_8D", DLY_FB=28, DLY_MIX=20, REV_SIZE=70, REV_MIX=18)),
    ("SILENCE SYNTH", lead(cut=78, reso=26, glide=0, delay=24, O2_WAVE="W_SAW", O2_PW=64, O2_FINE=71, AS=110)),
    ("SILENCE PAD", pad(aa=70, ar=100, cut=50, CHO_MIX=40)),
    ("SILENCE BASS", bass(cut=50, reso=22, fd=48, fs=40)),
    ("DRIVE PAD", pad(aa=95, ar=105, cut=56, CHO_MIX=55, REV_MIX=45, LFO_PWM=30)),
    ("ORDINARY PAD", pad(aa=85, ar=105, cut=52, DLY_TIME="DD_8D", DLY_FB=30, DLY_MIX=20, REV_MIX=50)),
    ("TEAR LEAD", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=30, O2_WAVE="W_PLS", O2_PW=40, O2_FINE=68, FTYPE="F_LP12", CUTOFF=92, RESO=20, FENV_AMT=64, AA=0, AS=127, AR=28,
                    DLY_TIME="DD_8", DLY_FB=30, DLY_MIX=24, REV_SIZE=60, REV_MIX=16)),
    ("KNOW PAD", pad(aa=105, ar=110, cut=60, CHO_MIX=70, REV_MIX=55)),
    ("WAITING PAD", m(SAW2, O2_FINE=70, FTYPE="F_LP24", CUTOFF=46, RESO=10, FENV_AMT=80, FA=100, FD=100, FS=70, AA=100, AS=120, AR=100, SPREAD=110, VINTAGE=90,
                      CHO_MIX=60, CHO_RATE=22, REV_SIZE=110, REV_MIX=48)),
    ("DANCER BRASS", brass(cut=64, fenv=100, fa=0, fd=55, fs=40, aa=0, ar=30, spread=80, vint=70, cho=30, AD=62, AS=50)),
    ("DANCER BASS", bass(cut=44, reso=36, fd=44, fs=30, O2_PW=48)),
    ("DANCER SYNTH", lead(cut=84, reso=22, glide=0, delay=26, O2_WAVE="W_SAW", O2_PW=64, O2_FINE=71)),
    ("SAX SYNTH", m(MODE="M_MONO", O1_WAVE="W_PLS", O1_PW=55, O2_WAVE="W_SAW", O2_FINE=66, FTYPE="F_LP24", CUTOFF=56, RESO=30, FENV_AMT=80, FA=20, FD=70, FS=80, AA=14, AS=110, AR=40,
                    LFO_PITCH=5, LFO_DELAY=70, GLIDE=6, DRIVE=25, REV_SIZE=60, REV_MIX=22)),
    ("HORN LEAD", lead(cut=56, reso=14, glide=10, delay=18, O2_WAVE="W_SAW", O2_PW=64, O2_FINE=66, AA=16, AS=115, DRIVE=20, LFO_PITCH=6)),
]
FM2 += [
    ("LOGICAL EP", [4, 1, 1, 13, 50, 28, 10, 0], [0, 74, 25, 46], (12, 35, 6, 16)),
    ("TIME CHIME", [4, 5, 1, 8, 32, 50, 0, 0], [0, 80, 0, 70], (0, 45, 26, 50)),
    ("ORDINARY PLUCK", [4, 1, 1, 2, 30, 28, 4, 0], [0, 70, 0, 60], (0, 35, 40, 40)),
    ("HOLD LINE EP", [4, 1, 1, 14, 60, 30, 6, 0], [0, 64, 0, 40], (15, 20, 6, 14)),
    ("HOOK BASS", [4, 1, 2, 1, 58, 24, 8, 0], [0, 56, 10, 24], (0, 20, 0, 8)),
    ("FLAME EP", [4, 1, 1, 14, 20, 38, 0, 0], [0, 88, 30, 70], (0, 60, 22, 46)),
]
ORG += [
    ("PRAYER ORGAN", [12, 0, 0, 0, 0, 10, 10, 1], [10, 64, 127, 60], (0, 10, 0, 60)),
    ("WALK ORGAN", [13, 0, 0, 1, 1, 40, 15, 0], [0, 64, 127, 30], (0, 20, 6, 22)),
]
SP += [
    ("BRASS SECTION", "HORNS", 127, 15, 0, [6, 100, 127, 40], 0, (0, 12, 8, 30)),
    ("NUMB PIANO", "PIANO", 80, 0, 0, [0, 104, 0, 70], 0, (0, 10, 36, 50)),
    ("PRAYER CHOIR", "VOX", 110, 0, 0, [45, 100, 127, 90], 0, (0, 30, 12, 56)),
    ("KNOW CHOIR", "VOX", 100, 0, 0, [70, 100, 127, 100], 0, (0, 34, 14, 62)),
]
ORDER += """BRASS SECTION|BRAZIL HORNS|BRAZIL STAB|SAX SYNTH|HORN LEAD|NUMB PIANO|NUMB PAD|FAINT STRINGS|FAINT CELLO|RITUAL SYNTH|LOGICAL EP|FUN BASS|TIME CHIME|MANIAC SYNTH|
SILENCE SYNTH|SILENCE PAD|SILENCE BASS|DRIVE PAD|PRAYER ORGAN|PRAYER CHOIR|ORDINARY PAD|ORDINARY PLUCK|WALK ORGAN|HOLD LINE EP|TEAR LEAD|HOOK BASS|KNOW PAD|KNOW CHOIR|
WAITING PAD|FLAME EP|DANCER BRASS|DANCER BASS|DANCER SYNTH""".replace("\n", "").split("|")


def row(name, e, env, fx, mono=0):
    """one lgs_preset_t initialiser line"""
    return '    {"%s", {%s}, {%s}, 0, %d, FX(%s)},\n' % (name, ", ".join(map(str, e)), ", ".join(map(str, env)), mono, ", ".join(map(str, fx)))


def rows(table):
    return "    /* ICONIC (tools/famous_defs.py) */\n" + "".join(row(n, e, env, fx) for n, e, env, fx in table)
