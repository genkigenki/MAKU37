/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Firmware save flows against NOR flash: deferred settings, retries, and failed-save rollback. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e) { (void)e; return 0; }
static uint32_t irq_start_race, irq_races;
static void fm1_irq_off(void)
{
    if (irq_start_race) {                         /* audio serviced PLAY immediately before main masks it */
        irq_start_race = 0;
        events_block(CTL);
        irq_races++;
    }
}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_power(uint32_t s) { (void)s; }   /* (MENU > SCREEN OFF: lcd.c) */
static void lcd_wake_now(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#define FELUCCA_FLASH 1
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
#include "../firmware/src/menu_items.c"
static void panel_setup(void) {}

static uint8_t nor[0x100000], flash_ok = 1;
static int erase_error, fail_after = -1;
/* erases / as_erases count the SAVES (a commit record programmed: storage.c writes it last), not the sector erases: a save
 * into a sector that reads blank skips its erase (storage.c st_blank, st_tidy), so the erases are counted on their own
 * (real_erases) where a test is about them. as_erases: the autosave's two sectors, 0xE5000 / 0xE6000 */
static uint32_t erases, as_erases, real_erases;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    real_erases++;
    if (erase_error) return -8;
    memset(nor + off, 0xFF, 4096);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    if (erase_error) return -8;                      /* (a failing part fails its programs too: a save into a blank sector has no erase to fail) */
    if (fail_after == 0) return -9;
    if (fail_after > 0) fail_after--;
    for (i = 0; i < n; i++) nor[off + i] &= s[i];
    if ((off & 0xFFFu) == 0u && n == 32u) {          /* the commit record: the save is done */
        erases++;
        as_erases += off >= 0xE5000u && off < 0xE7000u;
    }
    return 0;
}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0; }
static void fl_plain_window_init(void) {}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
#include "../firmware/src/project.c"


static int check(const char *what, int ok)
{
    printf("persist: %-66s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

static void reset(void)
{
    memset(nor, 0xFF, sizeof nor);
    memset(&song, 0, sizeof song);
    memset(trk, 0, sizeof trk);
    memset(&chain, 0, sizeof chain);
    chain_defaults(&chain_config);
    memset(proj_slot, 0, sizeof proj_slot);
    memset(up_bank, 0, sizeof up_bank);
    memset(&persist_saved, 0, sizeof persist_saved);
    memset(&settings, 0, sizeof settings);
    memset(&ui, 0, sizeof ui);
    memset(&favorites, 0, sizeof favorites);
    panel = PANEL_DEFAULT;
    host_tracks_init();
    fm1_ms = 0;
    transport_req = 0;
    persist_pending = 0;
    persist_retry_ms = 0;
    flash_ok = 1;
    erase_error = 0;
    fail_after = -1;
    up_gen = erases = real_erases = 0;
    irq_start_race = irq_races = 0;
}

/* ---- 1.0.3: the retired FM6 bank's patches move into the user presets on the first boot (up_fm6.c) ---- */
static uint8_t mig_pre[0x100000];
static fm6_bank_t mig_bank;

static void mig_rec(up_bank_t *b, uint32_t i, uint32_t engine, int16_t slot, char tag)
{
    up_rec_t *r = &b->r[i];
    uint32_t k;
    memset(r, 0, sizeof *r);
    r->used = UP_USED; r->ver = UP_VER; r->engine = (uint8_t)engine; r->np = P_COUNT;
    r->name[0] = tag;
    for (k = 0; k < P_COUNT; k++) up_set_value(r, k, param_desc_of(engine, k)->def);
    up_set_value(r, P_E7, slot);
    up_set_value(r, P_E0, (int16_t)(tag & 7));          /* (each record another sound) */
}

/* a 1.0.2 flash: user presets on B3 (slot 1), F2 (2), B5 empty (3), DRUM (4), B1 (18); the bank written twice */
static void mig_setup(void)
{
    static up_bank_t b[2];
    memset(nor, 0xFF, sizeof nor);
    memset(b, 0, sizeof b);
    for (uint32_t i = 0; i < 2u; i++) { b[i].magic = UP_BANK_MAGIC; b[i].rsize = sizeof(up_rec_t); b[i].nslot = UP_PER_BANK; }
    mig_rec(&b[0], 0, ENGI_FM6, FM6_NFACTORY + 2, 'A');
    mig_rec(&b[0], 1, ENGI_FM6, 1, 'B');
    mig_rec(&b[0], 2, ENGI_FM6, FM6_NFACTORY + 4, 'C');
    mig_rec(&b[0], 3, ENGI_DRUM, FM6_NFACTORY + 2, 'D');
    mig_rec(&b[1], 1, ENGI_FM6, FM6_NFACTORY + 0, 'E');
    st_save(OBJ_UPRESET0, &b[0], sizeof b[0]);
    st_save(OBJ_UPRESET0 + 1, &b[1], sizeof b[1]);
    memset(&mig_bank, 0, sizeof mig_bank);
    mig_bank.magic = FM6_BANK_MAGIC; mig_bank.ver = 1; mig_bank.nslot = FM6_BANK_N; mig_bank.used = 1u | 1u << 2;
    memcpy(mig_bank.v[2], FM6_FACTORY[4], FM6_PACKED); memcpy(mig_bank.v[2] + 118, "OLD BANK 1", 10);
    st_save(OBJ_FM6BANK, &mig_bank, sizeof mig_bank);   /* A: an older bank */
    memcpy(mig_bank.v[0], FM6_FACTORY[6], FM6_PACKED); memcpy(mig_bank.v[0] + 118, "NEW B1    ", 10);
    memcpy(mig_bank.v[2] + 118, "NEW B3    ", 10);
    st_save(OBJ_FM6BANK, &mig_bank, sizeof mig_bank);   /* B: the newest */
}

static void mig_boot(void)                               /* a power-on: RAM lost, the stores from flash */
{
    memset(up_bank, 0, sizeof up_bank);
    memset(&upf, 0, sizeof upf);
    up_boot();
}

/* the presets play what 1.0.2 played: slot 1 the newest B3, 18 B1, 2 F2, 3 init (an empty B5), DRUM none */
static int mig_ok(void)
{
    uint8_t pk[FM6_PACKED];
    return !upf_get(0, pk) && !memcmp(pk + 118, "NEW B3    ", 10) && !upf_get(17, pk) &&
           !memcmp(pk + 118, "NEW B1    ", 10) && upf_get(1, pk) && upf_get(2, pk) && upf_get(3, pk);
}

static int mig_test(void)
{
    int bad = 0, ok = 1;
    uint32_t cut, before, cuts = 0;
    uint8_t pk[FM6_PACKED];
    upf_t got;
    reset();
    mig_setup();
    memcpy(mig_pre, nor, sizeof nor);
    mig_boot();
    bad += check("FM6 bank -> user presets: B slots get the bank's newest patches", mig_ok());
    bad += check("  .. written into A (the bank's older copy); the bank's newest copy (B) still whole",
                 st_load(OBJ_UPFM6, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &upf, sizeof got) &&
                     ((st_hdr_t *)(nor + 0x9F000))->type == OBJ_UPFM6 && st_load(OBJ_FM6BANK, &mig_bank, sizeof mig_bank) > 0 &&
                     !memcmp(mig_bank.v[0] + 118, "NEW B1", 6));
    up_load(0);
    bad += check("  .. UP_LOAD 1 plays the bank's B3, SLOT OWN", !memcmp(fm6_patch[song.sel] + FP_NAME, "NEW B3    ", 10) &&
                 TSEL->p[P_E7] == FM6_OWN);
    up_load(1);
    bad += check("  .. UP_LOAD 2 (F2) plays F2, SLOT F2", !memcmp(fm6_patch[song.sel] + FP_NAME, FM6_FACTORY[1] + 118, 10) &&
                 TSEL->p[P_E7] == 1);
    up_load(2);
    bad += check("  .. UP_LOAD 3 (an empty B5) plays the init voice, as before",
                 !memcmp(fm6_patch[song.sel] + FP_NAME, "INIT VOICE", 10) && TSEL->p[P_E7] == FM6_OWN);
    before = erases;
    mig_boot();
    bad += check("  .. the next boot: done (no write), the same patches", mig_ok() && erases == before);
    /* a power cut at every write step of the migration: the next boot ends with the same result */
    for (cut = 0; cut < 64u && ok; cut++) {
        memcpy(nor, mig_pre, sizeof nor);
        fail_after = (int)cut;
        mig_boot();                                      /* (writes stop at the cut: the rest fails) */
        if (fail_after > 0) break;                       /* (this boot finished before the cut) */
        fail_after = -1;
        cuts++;
        mig_boot();
        ok &= mig_ok() && st_load(OBJ_UPFM6, &got, sizeof got) == (int)sizeof got;
    }
    memcpy(nor, mig_pre, sizeof nor);
    erase_error = 1;
    mig_boot();                                          /* a cut before the erase */
    erase_error = 0;
    mig_boot();
    ok &= mig_ok();
    bad += check("a power cut at any step of the migration loses nothing (the next boot redoes it)", ok && cuts >= 10u);
    /* after it: a preset save writes the other sector (the bank's last copy); a cut there keeps the first copy */
    memcpy(nor, mig_pre, sizeof nor);
    fail_after = -1;
    mig_boot();
    trk[song.sel].eng_req = ENGI_FM6;
    fm6_load_slot(song.sel, 3);
    fail_after = (int)((sizeof(up_bank_t) + 255u) / 256u + 1u + 3u);   /* the preset bank's write passes, the patches' fails */
    up_store(20, "CUT");
    fail_after = -1;
    mig_boot();
    bad += check("a cut while a preset's patch is written: the others kept, that preset without one",
                 mig_ok() && upf_get(20, pk));
    up_store(20, "SAVED");
    mig_boot();
    bad += check("saved again: its patch, and the bank's sectors now both hold the patches object",
                 mig_ok() && !upf_get(20, pk) && !memcmp(pk + 118, FM6_FACTORY[3] + 118, 10) &&
                     ((st_hdr_t *)(nor + 0x9F000))->type == OBJ_UPFM6 && ((st_hdr_t *)(nor + 0xFE000))->type == OBJ_UPFM6);
    memset(nor, 0xFF, sizeof nor);                        /* a device that never had a bank: nothing written */
    before = erases;
    mig_boot();
    bad += check("no bank: nothing to move, nothing written", erases == before && upf_valid(&upf));
    return bad;
}

/* ---- 1.2, Discussion #130: the autosave (project.c): written only when the music changed, stopped and idle 10 s, at
 * most one a minute; restored at power-on (RESTORE LAST ON, a clean boot); a damaged one ignored ---- */
static void as_run(uint32_t ms)                          /* the main loop for ms (autosave_poll checks every 250 ms) */
{
    uint32_t end = fm1_ms + ms;
    while ((int32_t)(end - fm1_ms) > 0) {
        fm1_ms += 50u;
        autosave_poll();
    }
}
static void as_power_on(void)                            /* RAM lost: the defaults, then main.c's restore */
{
    uint32_t k;
    memset(trk, 0, sizeof trk);
    memset(&motion, 0, sizeof motion);
    memset(motion_active, 0, sizeof motion_active);
    memset(&as, 0, sizeof as);
    memset(&ui, 0, sizeof ui);
    proj_name[0] = 0;
    chain_defaults(&chain_config);
    host_tracks_init();
    for (k = 0; k < NTRK; k++)
        trk[k].eng_req = trk[k].engine = (uint8_t)TRK_DEF[k][0];
}
static void as_edit(uint32_t i)                          /* a change a project holds: step i's note (always another) */
{
    static uint32_t n;
    track_t *t = &trk[i % NTRK];
    step_t *s = &t->step[i % 16u];
    s->time = ST_NOTE;
    s->n = 1;
    s->note[0] = (uint8_t)(20u + (s->note[0] + 1u + n++ % 50u) % 100u);
    s->vel = 100;
}
static int autosave_test(void)
{
    int bad = 0, ok;
    uint32_t w, k, b0, minute;
    step_t st[NSTEP];
    project_store_t raw;
    reset();
    as_power_on();
    as_erases = 0;
    ok = !autosave_boot(1) && !as.writes;
    as_run(120000u);
    bad += check("autosave: power-on with none: nothing restored; 2 min unchanged: no write", ok && !as.writes && !as_erases);
    as_edit(0);
    as_run(9000u);
    ok = !as.writes;
    as_run(1500u);
    bad += check("autosave: a change: no write within 10 s, one once idle 10 s (one erase)",
                 ok && as.writes == 1u && as_erases == 1u && st_load(OBJ_AUTOSAVE, &raw, sizeof raw) == (int)sizeof raw);
    as_run(600000u);
    bad += check("autosave: 10 min unchanged after it: no more writes", as.writes == 1u && as_erases == 1u);
    song.g[G_SLOT] = 3;                                  /* (the PROJECT page's pick, the selected track: not the music) */
    song.sel = 2;
    as_run(120000u);
    bad += check("autosave: the selected track / PROJECT's slot pick alone: no write", as.writes == 1u);
    song.g[G_BPM] = 133;
    song.playing = 1;
    as_run(300000u);
    ok = as.writes == 1u;
    song.playing = 0;
    as_run(9000u);
    ok &= as.writes == 1u;
    as_run(1500u);
    bad += check("autosave: changed while playing: no write until stopped and idle 10 s, then one", ok && as.writes == 2u);
    as_edit(1);
    fm1_in.notes = 1u << 7;                              /* a key held (playing the sound, stopped) */
    as_run(60000u);
    ok = as.writes == 2u;
    fm1_in.notes = 0;
    trk[1].v[0].active = 1;                              /* .. its release still sounding */
    as_run(20000u);
    ok &= as.writes == 2u;
    trk[1].v[0].active = 0;
    as_run(9000u);
    ok &= as.writes == 2u;
    as_run(1500u);
    bad += check("autosave: a key held, a voice sounding: no write (an erase stops the audio); after, idle 10 s: one",
                 ok && as.writes == 3u);
    as_run(60000u);
    as_edit(2);                                          /* written 10 s on; then edits 15 s apart: a minute's gap */
    as_run(11000u);
    w = as.writes;
    as_edit(3);
    as_run(15000u);
    ok = as.writes == w;
    as_edit(4);
    as_run(15000u);
    ok &= as.writes == w;
    as_run(40000u);
    bad += check("autosave: two writes at least 60 s apart", ok && as.writes == w + 1u);
    as_run(60000u);
    w = as.writes;
    as_edit(5);
    for (k = 0; k < 120u; k++) {                         /* an editor backup going on: holds it */
        as_run(500u);
        autosave_hold();
    }
    ok = as.writes == w;
    as_run(10500u);
    bad += check("autosave: none during an editor transfer (autosave_hold), one after", ok && as.writes == w + 1u);
    /* power-on: restored */
    as_edit(6);
    song.g[G_BPM] = 97;
    str_cpy(proj_name, "MY TUNE", sizeof proj_name);
    as_run(70000u);
    memcpy(st, trk[0].step, sizeof st);
    b0 = as_erases;
    as_power_on();
    ok = memcmp(st, trk[0].step, sizeof st) != 0 && song.g[G_BPM] != 97;
    ok &= autosave_boot(1) == 1 && !memcmp(st, trk[0].step, sizeof st) && song.g[G_BPM] == 97 && str_eq(proj_name, "MY TUNE") &&
          ui.msg_t && str_eq(ui.msg, "RESTORED");
    as_run(300000u);
    bad += check("autosave: power-on restores it (steps, tempo, name; RESTORED); no write after (unchanged)",
                 ok && !as.writes && as_erases == b0);
    as_power_on();
    ok = autosave_boot(0) == 0 && song.g[G_BPM] != 97;
    as_run(60000u);
    bad += check("autosave: after a crash / hang / failed boot: not restored, nothing written", ok && as_erases == b0);
    as_power_on();
    ui_prefs |= PREF_RESTORE_OFF;
    ok = autosave_boot(1) == 0 && song.g[G_BPM] != 97;
    as_edit(7);
    as_run(300000u);
    bad += check("autosave: RESTORE LAST OFF: none restored, none written", ok && !as.writes && as_erases == b0);
    ui_prefs &= (uint8_t)~PREF_RESTORE_OFF;
    {   /* a damaged copy: the other one (A/B); both damaged: none */
        st_hdr_t h;
        int cur, other;
        as_power_on();
        autosave_boot(1);
        song.g[G_BPM] = 111;                             /* a newer copy into the other sector */
        as_run(11000u);
        cur = st_current(OBJ_AUTOSAVE, &h);
        nor[st_sector(OBJ_AUTOSAVE, (uint32_t)cur) + ST_PAYLOAD_OFF + 100u] ^= 0x10u;   /* its payload: the CRC fails */
        other = st_current(OBJ_AUTOSAVE, &h);
        as_power_on();
        ok = cur >= 0 && other == !cur && autosave_boot(1) == 1 && song.g[G_BPM] == 97;
        bad += check("autosave: the newest copy damaged: the copy before it is restored", ok);
        nor[st_sector(OBJ_AUTOSAVE, (uint32_t)other) + 12u] ^= 0x01u;   /* the other one's header too */
        as_power_on();
        ok = autosave_boot(1) == 0 && song.g[G_BPM] != 97 && song.g[G_BPM] != 111;
        bad += check("autosave: both copies damaged: ignored, power-on as new", ok);
        memset(nor + 0xE5000u, 0xFF, 0x2000u);
        st_save(OBJ_AUTOSAVE, "NOT A PROJECT", 13u);     /* a valid record that is no project */
        as_power_on();
        bad += check("autosave: a record that is no project: ignored", autosave_boot(1) == 0 && song.g[G_BPM] != 97);
    }
    /* a power cut in the middle of a write: the copy before stays in charge */
    memset(nor + 0xE5000u, 0xFF, 0x2000u);
    as_power_on();
    autosave_boot(1);
    as_edit(8);
    as_run(11000u);                                      /* (written: copy A) */
    song.g[G_BPM] = 150;
    fail_after = 3;                                      /* the next write stops after 3 programs */
    as_run(70000u);
    fail_after = -1;
    ok = as.err;
    as_power_on();
    ok &= autosave_boot(1) == 1 && song.g[G_BPM] != 150 && trk[0].step[8].n == 1u;
    bad += check("autosave: a write cut short (power off): the copy before is restored", ok);
    /* wear: a 4-hour session: 3 h of editing (a change every 3 s), pauses and playing it back, then an hour away */
    as_power_on();
    autosave_boot(1);
    as_erases = 0;
    for (minute = 0; minute < 240u; minute++) {
        uint32_t s;
        if (minute >= 180u) {
            as_run(60000u);
            continue;
        }
        song.playing = minute % 10u >= 7u;               /* 3 minutes in 10 playing */
        for (s = 0; s < 60u; s += 3u) {
            if (minute % 5u != 4u)                       /* (every fifth minute a pause, no edits) */
                as_edit(minute * 20u + s);
            as_run(3000u);
        }
    }
    song.playing = 0;
    as_run(20000u);
    printf("persist:   a 4-hour session (3 h editing and playing, 1 h idle): %u autosave writes\n", (unsigned)as_erases);
    bad += check("autosave wear: a session's writes stay far below one a minute", as_erases > 0u && as_erases <= 60u);
    {   /* worst case: a change, then just enough idle, over and over, for 8 hours */
        uint32_t i;
        as_erases = 0;
        for (i = 0; i < 8u * 3600u / 11u; i++) {
            as_edit(i);
            as_run(11000u);
        }
        printf("persist:   worst case, 8 h of a change every 11 s: %u writes (%u per sector)\n", (unsigned)as_erases,
               (unsigned)(as_erases / 2u));
        bad += check("autosave wear, worst case: at most one write a minute (480 in 8 h)", as_erases <= 8u * 60u + 1u);
    }
    return bad;
}

/* MAKU's likes (maku_fav.c) in flash: OBJ_MAKUFAV, the autosave's pair under its own type, written only by REC
 * (maku_fav_store), read once at power-on (persist_boot calls maku_fav_boot); a damaged copy falls back, a damaged ring
 * starts empty */
static void fav_fill(uint32_t n, uint32_t salt)
{
    uint32_t i, k;
    maku_fav.n = 0;
    for (i = 0; i < n; i++) {
        maku_fav_t e;
        memset(&e, 0, sizeof e);
        e.seed = salt * 1000u + i;
        for (k = 0; k < 3u; k++)
            e.v[k] = (uint8_t)(1u + (i + k) % 5u);
        e.root = (uint8_t)(i % 12u);
        e.scale = 2;
        e.bpm = (uint8_t)(70u + i);
        for (k = 0; k < 12u; k++)
            e.m[k] = (uint8_t)((i * 7u + k * 3u) % 128u);
        e.it[0] = IT_PHASER;
        e.ia[0] = 20;
        e.ib[1] = 90;
        e.ic[1] = 40;
        maku_fav_push(&e);
    }
}
static int fav_test(void)
{
    int bad = 0;
    maku_fav_t keep[MAKU_FAV_MAX];
    uint32_t e0, a0, r0, n;
    reset();
    flash_ok = 1;                                   /* (persist_boot would ask the part's id: the stub says none) */
    maku_fav_boot();
    bad += check("MAKU likes: empty flash, the ring starts empty", maku_fav.n == 0u);
    fav_fill(5, 1);
    memcpy(keep, maku_fav.e, sizeof keep);
    e0 = erases;
    a0 = as_erases;
    r0 = real_erases;
    bad += check("  REC from empty flash: the ring written into the autosave's pair, no sector erase (nothing to stop the audio)",
                 maku_fav_store() == 0 && erases == e0 + 1u && as_erases == a0 + 1u && real_erases == r0);
    maku_fav.n = 0;
    maku_fav_boot();
    bad += check("  power-on: the five likes back, as they were", maku_fav.n == 5u && !memcmp(keep, maku_fav.e, 5u * sizeof keep[0]));
    bad += check("  .. and the power-on wrote and erased nothing (no stale copy yet)", erases == e0 + 1u && real_erases == r0);
    bad += check("  not an autosave: the autosave's loader finds none there", st_load(OBJ_AUTOSAVE, &proj_wire, sizeof proj_wire) < 0);
    fav_fill(16, 2);
    memcpy(keep, maku_fav.e, sizeof keep);
    bad += check("  a second REC goes to the other sector (A / B by turns), blank too: no erase", maku_fav_store() == 0 &&
                 erases == e0 + 2u && real_erases == r0 && nor[0xE5000u] != 0xFFu && nor[0xE6000u] != 0xFFu);
    {
        uint32_t i;
        for (i = 0; i < 300u; i++)                       /* new worlds, likes or not: no flash */
            maku_world_pref(i * 7919u + 1u);
        bad += check("  300 new worlds from the likes: no flash write", erases == e0 + 2u && real_erases == r0);
    }
    {   /* the newer copy rots before any power-on tidied the stale one: the older still loads */
        static uint8_t keep_nor[0x2000];
        uint32_t cur = st_sector(OBJ_MAKUFAV, nor[0xE6000u + 8] != 0xFFu && ((st_hdr_t *)(nor + 0xE6000u))->seq == 2u ? 1u : 0u);
        memcpy(keep_nor, nor + 0xE5000u, sizeof keep_nor);
        nor[cur + ST_PAYLOAD_OFF + 40u] ^= 0x10u;        /* the newer copy rots */
        maku_fav.n = 0;
        maku_fav_boot();
        bad += check("  the newer copy damaged: the one before it loads (5 likes)", maku_fav.n == 5u);
        memcpy(nor + 0xE5000u, keep_nor, sizeof keep_nor);
        r0 = real_erases;
    }
    maku_fav.n = 0;
    maku_fav_boot();
    n = maku_fav.n;
    bad += check("  .. power-on: the newer ring (16 likes)", n == 16u && !memcmp(keep, maku_fav.e, sizeof keep));
    bad += check("  .. and the power-on erased the stale older copy (the audio has not started), the newer is intact",
                 real_erases == r0 + 1u && (nor[0xE5000u] == 0xFFu) != (nor[0xE6000u] == 0xFFu) && st_load(OBJ_MAKUFAV, &proj_wire, sizeof proj_wire) > 0);
    nor[0xE5000u + ST_PAYLOAD_OFF + 41u] ^= 0x10u;
    nor[0xE6000u + ST_PAYLOAD_OFF + 41u] ^= 0x10u;
    maku_fav.n = 7;
    maku_fav_boot();
    bad += check("  both damaged: the ring starts empty", maku_fav.n == 0u);
    {   /* a ring whose own checksum fails (the storage CRC over it is right): empty */
        uint8_t b[MAKU_FAV_BYTES];
        fav_fill(3, 3);
        maku_fav_pack(b);
        b[MAKU_FAV_HEAD + 3u] ^= 1u;
        memset(nor + 0xE5000u, 0xFF, 0x2000u);
        st_save(OBJ_MAKUFAV, b, sizeof b);
        maku_fav.n = 3;
        maku_fav_boot();
        bad += check("  a ring with a bad checksum of its own: empty", maku_fav.n == 0u);
        fav_fill(3, 3);
        maku_fav_pack(b);
        b[4] = MAKU_FAV_VER + 1u;
        maku_put32(b + MAKU_FAV_BYTES - 4u, maku_fav_sum(b, MAKU_FAV_BYTES - 4u));
        st_save(OBJ_MAKUFAV, b, sizeof b);
        maku_fav_boot();
        bad += check("  a ring of another version: empty", maku_fav.n == 0u);
    }
    fav_fill(4, 4);
    e0 = erases;
    erase_error = 1;
    bad += check("  a failed write is reported (the likes stay in RAM)", maku_fav_store() != 0 && maku_fav.n == 4u);
    erase_error = 0;
    maku_fav.n = 0;
    bad += check("  the clearing is written too (an empty ring)", maku_fav_store() == 0 && erases == e0 + 1u);
    fav_fill(2, 5);
    maku_fav_boot();
    bad += check("  .. and power-on finds it empty", maku_fav.n == 0u);
    maku_fav.n = 0;
    /* the erase ahead of the press (storage.c st_tidy, maku_fav_tidy): a REC stops the audio only if it has to erase */
    reset();
    flash_ok = 1;
    maku_fav_stale = 0;
    fav_fill(3, 6);
    bad += check("  REC, REC: two blank sectors, no erase at all", maku_fav_store() == 0 && maku_fav_store() == 0 && real_erases == 0u);
    bad += check("  .. the stale copy is known (maku_fav_stale)", maku_fav_stale == 1u);
    maku_fav_tidy();
    bad += check("  a new world's tidy: the stale copy erased (one erase), the newer intact", real_erases == 1u && maku_fav_stale == 0u &&
                 st_load(OBJ_MAKUFAV, &proj_wire, sizeof proj_wire) > 0);
    maku_fav_tidy();
    bad += check("  .. and a second tidy does nothing", real_erases == 1u);
    bad += check("  the next REC writes into the tidied sector: no erase", maku_fav_store() == 0 && real_erases == 1u);
    bad += check("  REC again before any new world: the stale copy is erased on the press (one erase, the dip)",
                 maku_fav_store() == 0 && real_erases == 2u);
    {   /* a copy that is not a MAKU ring (the autosave's, the pair is shared) is not touched by the tidy */
        uint8_t b[16] = {1, 2, 3};
        reset();
        flash_ok = 1;
        maku_fav_stale = 0;
        fav_fill(2, 7);
        maku_fav_store();
        maku_fav_store();                                /* B is current, A stale */
        st_save_to(OBJ_AUTOSAVE, b, sizeof b, 0);        /* .. then A holds an autosave (erased first: not blank) */
        r0 = real_erases;
        maku_fav_stale = 1;
        maku_fav_tidy();
        bad += check("  a copy that is another object's (an autosave) is left alone by the tidy", real_erases == r0 &&
                     st_load(OBJ_AUTOSAVE, b, sizeof b) == (int)sizeof b);
    }
    maku_fav_stale = 0;
    maku_fav.n = 0;
    return bad;
}

int main(void)
{
    int bad = 0, ok;
    uint32_t before;
    persist_t p;
    project_store_t old, got;
    project_v5_t v5;
    up_bank_t bank;
    up_rec_t r;
    reset();
    settings_init();
    reset();
    settings.palette = 3;
    settings.lowcut = 2;
    settings.zoom = 1;
    persist_saved.bold = 1;                       /* (an earlier firmware's font weight: kept as saved) */
    favorites.factory[8][0] = 1;
    favorites.user = 1u << 31;
    song.playing = 1;
    settings_save();
    bad += check("playing: settings queued, no erase, runtime kept", persist_pending && !erases &&
                  settings.palette == 3u && settings.lowcut == 2u && settings.zoom == 1u);
    song.playing = 0;
    transport_req = 1;
    settings_poll();
    bad += check("pending PLAY: settings still queued, no erase", persist_pending && !erases);
    transport_req = 0;
    chain.armed = 1;
    settings_poll();
    bad += check("pending SONG: settings still queued, no erase", persist_pending && !erases);
    chain.armed = 0;
    settings.palette = 2;
    settings_poll();
    bad += check("STOP: latest settings saved and request cleared", !persist_pending && erases == 1u &&
                  st_load(OBJ_SETTINGS, &p, sizeof p) == (int)sizeof p && p.palette == palette_to_stored(2) &&
                  p.lowcut == 2u && p.zoom == 1u && p.bold == 1u &&
                  p.favorites.factory[8][0] == 1 && p.favorites.user == (1u << 31) &&
                  !memcmp(&p.panel, &panel, sizeof panel));
    settings_save();
    bad += check("unchanged settings do not erase", !persist_pending && erases == 1u);
    settings.palette = 1;
    erase_error = 1;
    fm1_ms = 0xFFFFFF00u;
    settings_save();
    before = erases;
    settings_poll();
    fm1_ms += 999u;
    settings_poll();
    bad += check("failed settings retry waits through clock wrap", persist_pending && erases == before &&
                  persist_saved.palette == palette_to_stored(2) && settings.palette == 1u);
    erase_error = 0;
    fm1_ms++;
    settings_poll();
    bad += check("settings retry succeeds after 1000 ms", !persist_pending && erases == before + 1u &&
                  st_load(OBJ_SETTINGS, &p, sizeof p) == (int)sizeof p && p.palette == palette_to_stored(1));

    reset();
    trk[0].step[0] = (step_t){{60}, 1, ST_NOTE, 0, 96, 0, 0};
    project_save(0);
    old = proj_slot[0];
    trk[0].step[0].note[0] = 72;
    fail_after = 1;
    project_save(0);
    fail_after = -1;
    bad += check("failed project save keeps RAM and old flash", !strcmp(ui.msg, "SAVE ERROR") &&
                  !memcmp(&proj_slot[0], &old, sizeof old) && st_load(OBJ_PROJECT0, &got, sizeof got) == (int)sizeof got &&
                  !memcmp(&got, &old, sizeof got));
    project_save(0);
    bad += check("successful project save publishes new RAM", !strcmp(ui.msg, "SAVED") &&
                  proj_import(&proj_scratch, &proj_slot[0], sizeof proj_slot[0]) &&
                  proj_scratch.t[0].step[0].note[0] == 72u && proj_ok(&proj_scratch));
    before = erases;
    transport_req = 1;
    project_save(1);
    bad += check("pending PLAY: project save leaves slot and flash", !project_used(1) && erases == before &&
                  !strcmp(ui.msg, "STOP TO SAVE"));
    transport_req = 0;
    erase_error = 1;
    project_save(2);
    bad += check("failed first project save leaves slot empty", !project_used(2));
    erase_error = 0;
    {   /* names: a project's through flash; a failed rename keeps the old one in RAM and flash */
        char n[16];
        project_store_t keep;
        step_t st0 = trk[0].step[0];
        project_save_as(0, "LOFI JAM");
        memset(proj_slot[0].raw, 0, 8);                       /* (the RAM copy lost: read it back from flash) */
        proj_fetch(0);
        bad += check("a project's name goes to flash and comes back", project_name(0, n) && !strcmp(n, "LOFI JAM") &&
                      !strcmp(proj_name, "LOFI JAM"));
        keep = proj_slot[0];
        fail_after = 1;
        bad += check("failed project rename: SAVE ERROR, the slot and flash as they were",
                      project_rename(0, "DUB") == 2 && !strcmp(ui.msg, "SAVE ERROR") && !memcmp(&proj_slot[0], &keep, sizeof keep) &&
                      st_load(OBJ_PROJECT0, &got, sizeof got) == (int)sizeof got && !memcmp(&got, &keep, sizeof got));
        fail_after = -1;
        before = erases;
        transport_req = 1;
        bad += check("pending PLAY: no rename, no erase", project_rename(0, "DUB") == 1 && erases == before &&
                      !strcmp(ui.msg, "STOP TO SAVE"));
        transport_req = 0;
        bad += check("project rename: flash has the new name, the music as it was", project_rename(0, "DUB") == 0 &&
                      !strcmp(ui.msg, "RENAMED") && (memset(proj_slot[0].raw, 0, 8), proj_fetch(0), project_name(0, n)) &&
                      !strcmp(n, "DUB") && proj_scratch.t[0].step[0].note[0] == st0.note[0]);
    }
    {   /* FUN7 itself refuses a step outside its fields: a damaged slot is empty, the tracks stay */
        step_t keep = trk[0].step[0];
        old = proj_slot[0];
        proj_slot[0].raw[68 + P_COUNT + 2 + 4] = 7;               /* track 1 step 1: n = 7 */
        bad += check("damaged FUN7 slot (hash ok, n > 4) is refused", (memcpy(proj_slot[0].raw + PROJ_STORE_SIZE - 4,
                      &(uint32_t){proj_hash(proj_slot[0].raw, PROJ_STORE_SIZE - 4)}, 4), !project_used(0)));
        st_save(OBJ_PROJECT0, &proj_slot[0], sizeof proj_slot[0]);
        ui.msg[0] = 0;
        project_load(0);
        bad += check("loading it says EMPTY SLOT and keeps the steps", !strcmp(ui.msg, "EMPTY SLOT") &&
                      !memcmp(&trk[0].step[0], &keep, sizeof keep));
        proj_slot[0] = old;
        st_save(OBJ_PROJECT0, &proj_slot[0], sizeof proj_slot[0]);
    }
    {   /* an older format (FUN5) with values FUN7 cannot pack still loads, bounded */
        uint32_t i;
        memset(&v5, 0, sizeof v5);
        v5.magic = PROJ_MAGIC_V5;
        v5.size = sizeof v5;
        for (i = 0; i < G_COUNT; i++)
            v5.g[i] = GP[i].def;
        for (i = 0; i < NTRK; i++) {
            v5.t[i].engine = trk[i].engine;
            memcpy(v5.t[i].p, trk[i].p, sizeof v5.t[i].p);
        }
        v5.t[0].p[P_LEVEL] = 900;                                  /* out of range */
        v5.t[0].step[0] = (step10_t){{255, 128, 160, 200}, 255, 255, 0, 96, 255, 5};
        v5.sum = proj_hash(&v5, sizeof v5 - 4u);
        st_save(OBJ_PROJECT0 + 3u, &v5, sizeof v5);
        memset(proj_slot[3].raw, 0, 4);
        project_load(3);
        bad += check("FUN5 load bounds steps and masks lane accents", trk[0].step[0].n == 4u &&
                      trk[0].step[0].time == ST_REST && trk[0].step[0].note[0] == 127u &&
                      !trk[0].step[0].note[1] && trk[0].step[0].acc == 5u && project_used(3));
        bad += check("FUN5 load clamps a parameter into its range", trk[0].p[P_LEVEL] == TP[P_LEVEL].max);
        for (i = 0; i < P_COUNT; i++) {
            uint32_t e;
            for (e = 0; e < NENGINES; e++)
                if (param_desc_of(e, i)->min < -64 || param_desc_of(e, i)->max > 127) {
                    printf("param %u engine %u: range %d..%d does not fit FUN7's byte\n", i, e,
                           param_desc_of(e, i)->min, param_desc_of(e, i)->max);
                    bad++;
                }
        }
    }
    chain_config.count = 1;
    chain_config.row[0] = (chain_row_t){3, 1};
    bad += check("SONG sources use the same step bounds", chain_prepare() == 0 &&
                  !memcmp(&chain.source[3].step[0][0], &trk[0].step[0], sizeof(step_t)));
    seq_stop();
    transport_req = 0;
    memset(&r, 0, sizeof r);
    r.used = UP_USED;
    r.ver = UP_VER;
    r.np = P_COUNT;
    memcpy(r.name, "First", 6);
    erase_error = 1;
    bank = up_bank[1];
    ok = up_put(17, &r) == 2;
    erase_error = 0;
    bad += check("failed first preset save restores empty bank", ok && !memcmp(&up_bank[1], &bank, sizeof bank));
    bad += check("preset initial save", up_put(3, &r) == 0 && up_used(3));
    bank = up_bank[0];
    before = up_gen;
    r.p[0] = 77;
    fail_after = 1;
    ok = up_put(3, &r) == 2;
    fail_after = -1;
    bad += check("failed preset save rolls back bank and generation", ok && up_gen == before &&
                  !memcmp(&up_bank[0], &bank, sizeof bank));
    favorite_set(NENGINES, 3, 1);
    fail_after = 1;
    ok = up_put(3, 0) == 2;
    fail_after = -1;
    bad += check("failed preset erasure retains both the sound and its favorite", ok && up_used(3) &&
                  favorite_has(NENGINES, 3) && !memcmp(&up_bank[0], &bank, sizeof bank));
    memset(&up_bank[0], 0, sizeof up_bank[0]);
    up_boot();
    bad += check("failed preset save keeps old flash", !memcmp(&up_bank[0], &bank, sizeof bank));
    trk[0].user = 4;
    erase_error = 1;
    ok = up_put(3, 0) == 2;
    erase_error = 0;
    bad += check("failed preset erase keeps record and loaded label", ok && trk[0].user == 4u && up_gen == before &&
                  !memcmp(&up_bank[0], &bank, sizeof bank));
    bad += check("successful preset erase clears loaded label", up_put(3, 0) == 0 && !up_used(3) && !trk[0].user);
    before = erases;
    transport_req = 1;
    bad += check("pending PLAY: preset save blocked", up_put(3, &r) == 2 && erases == before && !up_used(3));
    transport_req = 0;
    chain.armed = 1;
    bad += check("pending SONG: preset save blocked", up_put(3, &r) == 2 && erases == before && !up_used(3));
    chain.armed = 0;
    bad += check("preset invalid slot blocked before access", up_put(UP_SLOTS, &r) == 1 && erases == before);
    song.playing = 0; transport_req = 1; irq_start_race = 1;
    bad += check("PLAY consumed before the save snapshot still blocks a project write",
                  project_save(1) != 0 && irq_races == 1u && song.playing && !transport_req && erases == before);
    song.playing = 0; transport_req = 1; irq_start_race = 1;
    bad += check("PLAY consumed before the preset snapshot still blocks a bank write",
                  up_put(3, &r) == 2 && irq_races == 2u && song.playing && !transport_req && erases == before);
    song.playing = 0; transport_req = 1; irq_start_race = 1;
    settings.palette = 4;
    settings_save();
    bad += check("PLAY consumed before the settings snapshot defers the write",
                  irq_races == 3u && song.playing && !transport_req && persist_pending && erases == before);
    bad += mig_test();
    bad += autosave_test();
    bad += fav_test();
    printf("%s\n", bad ? "PERSISTENCE TEST FAILED" : "persistence test passed");
    return bad != 0;
}
