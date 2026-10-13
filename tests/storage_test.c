/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Host test of firmware/src/storage.c against a simulated NOR flash:
 * erase -> 0xFF, program can only clear bits, page writes must not wrap. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint8_t nor[0x100000];
static int fail_after = -1;            /* torn-write injection: stop after N programs */
static int fail_bytes = -1;            /* power loss after any programmed byte */
static int erase_error, write_protected;
static uint32_t io_calls, erase_calls;

static int st_read(uint32_t off, void *dst, uint32_t n) { io_calls++; memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off)
{
    io_calls++;
    erase_calls++;
    if (erase_error) return -8;
    if (!write_protected) memset(nor + off, 0xFF, 4096);
    return 0;
}
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    io_calls++;
    if (fail_after == 0)
        return -9;
    if (fail_after > 0)
        fail_after--;
    if ((off & 0xFFu) + n > 256u) {
        printf("page wrap at %#x\n", off);
        exit(1);
    }
    for (i = 0; i < n; i++) {
        if (fail_bytes == 0) return -9;
        if (fail_bytes > 0) fail_bytes--;
        if (!write_protected) nor[off + i] &= s[i];
    }
    return 0;
}
#include "../firmware/src/storage.c"

static int check(const char *what, int ok)
{
    printf("%-46s %s\n", what, ok ? "ok" : "FAIL");
    return ok ? 0 : 1;
}

int main(void)
{
    char a[600], b[600], got[600];
    int bad = 0, n;
    memset(nor, 0xFF, sizeof nor);
    memset(a, 'A', sizeof a);
    memset(b, 'B', sizeof b);
    bad += check("empty flash loads nothing", st_load(OBJ_PROJECT0, got, sizeof got) < 0);
    bad += check("save A", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns A", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    bad += check("save B", st_save(OBJ_PROJECT0, b, sizeof b) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns B (newer seq)", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    fail_after = 1;                                  /* payload page 1 written, the rest torn */
    st_save(OBJ_PROJECT0, a, sizeof a);
    fail_after = -1;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("torn save keeps B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    fail_after = 3;                                  /* all payload pages, header torn */
    st_save(OBJ_PROJECT0, a, sizeof a);
    fail_after = -1;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("header not written keeps B", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    bad += check("save A again", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("load returns A", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    nor[st_sector(OBJ_PROJECT0, 0) + ST_PAYLOAD_OFF + 10] ^= 0x01;   /* bit rot in one copy */
    nor[st_sector(OBJ_PROJECT0, 1) + ST_PAYLOAD_OFF + 10] ^= 0x01;
    n = st_load(OBJ_PROJECT0, got, sizeof got);
    bad += check("both copies corrupt -> nothing", n < 0);
    bad += check("other objects untouched", st_load(OBJ_PROJECT0 + 1, got, sizeof got) < 0);
    bad += check("settings save/load",
                 st_save(OBJ_SETTINGS, "hello", 5) == 0 && st_load(OBJ_SETTINGS, got, 5) == 5 && !memcmp(got, "hello", 5));
    bad += check("CRC-32 is zlib's (check value 0xCBF43926)", st_crc32("123456789", 9) == 0xCBF43926u);
    memset(nor, 0xFF, sizeof nor);
    st_save(OBJ_PROJECT0 + 2, a, sizeof a);
    st_save(OBJ_PROJECT0 + 2, b, sizeof b);              /* B is newer, in the other copy */
    {
        st_hdr_t h;
        int cur = st_current(OBJ_PROJECT0 + 2, &h);
        nor[st_sector(OBJ_PROJECT0 + 2, (uint32_t)cur) + ST_PAYLOAD_OFF + 300] ^= 0x10;   /* rot in the newer copy */
    }
    n = st_load(OBJ_PROJECT0 + 2, got, sizeof got);
    bad += check("newer copy rotten -> the older one loads", n == (int)sizeof a && !memcmp(got, a, sizeof a));
    bad += check("save over the rotten copy", st_save(OBJ_PROJECT0 + 2, b, sizeof b) == 0);
    n = st_load(OBJ_PROJECT0 + 2, got, sizeof got);
    bad += check("... loads the new data", n == (int)sizeof b && !memcmp(got, b, sizeof b));
    nor[st_sector(OBJ_PROJECT0 + 2, 0) + 8] ^= 0x01;    /* both headers broken */
    nor[st_sector(OBJ_PROJECT0 + 2, 1) + 8] ^= 0x01;
    bad += check("both headers broken -> nothing", st_load(OBJ_PROJECT0 + 2, got, sizeof got) < 0);
    bad += check("data stays in the Felucca regions",
                 st_sector(OBJ_SETTINGS, 1) + 4096 <= 0xFF000 && st_sector(OBJ_PROJECT0 + 3, 1) + 4096 <= 0xE0000 &&
                     st_sector(OBJ_UPRESET0, 0) >= 0xDC000 && st_sector(OBJ_UPRESET0 + 1, 1) + 4096 <= 0xE0000);
    /* the user presets' FM6 patches (1.0.3) in the two sectors of the retired FM6 bank: 0x9F000 (after the projects)
     * and 0xFE000 (after the settings); the two objects share them, told apart by the commit record's type */
    bad += check("FM6 user preset patches (and the retired bank) in the sectors 0x9F000 / 0xFE000",
                 OBJ_UPFM6 == OBJ_COUNT - 3 && OBJ_FM6BANK == OBJ_UPFM6 - 1 && st_sector(OBJ_UPFM6, 0) == 0x9F000u &&
                     st_sector(OBJ_PROJECT0 + 3, 1) + 4096 == 0x9F000u && st_sector(OBJ_UPFM6, 1) == 0xFE000u &&
                     st_sector(OBJ_FM6BANK, 0) == 0x9F000u && st_sector(OBJ_FM6BANK, 1) == 0xFE000u &&
                     st_sector(OBJ_SETTINGS, 1) + 4096 == 0xFE000u);
    /* the autosave (1.2, project.c): its own pair 0xE5000 / 0xE6000, above the OTA staging, apart from every other
     * object; a save there touches no other object */
    {
        uint32_t o, c, ok = OBJ_AUTOSAVE == OBJ_COUNT - 2 && st_sector(OBJ_AUTOSAVE, 0) == 0xE5000u &&
                               st_sector(OBJ_AUTOSAVE, 1) == 0xE6000u;
        static uint8_t snap[0x100000];
        for (o = 0; o < OBJ_AUTOSAVE; o++)
            for (c = 0; c < 2u; c++)
                ok &= st_sector(o, c) + 4096u <= 0xE5000u || st_sector(o, c) >= 0xE7000u;
        memcpy(snap, nor, sizeof nor);
        ok &= st_save(OBJ_AUTOSAVE, a, sizeof a) == 0 && st_load(OBJ_AUTOSAVE, got, sizeof got) == (int)sizeof a &&
              !memcmp(got, a, sizeof a) && !memcmp(snap, nor, 0xE5000u) && !memcmp(snap + 0xE7000u, nor + 0xE7000u, 0x19000u);
        bad += check("the autosave in its own sectors 0xE5000 / 0xE6000, nothing else touched", ok);
    }
    /* MAKU's likes (maku_fav.c): the autosave's pair under a type of their own (MAKU writes no autosave: a Felucca
     * autosave left there is written over); the two are told apart by the commit record's type, and a save of the
     * likes touches nothing outside the pair */
    {
        static uint8_t snap[0x100000];
        uint32_t ok = OBJ_MAKUFAV == OBJ_COUNT - 1 && st_sector(OBJ_MAKUFAV, 0) == 0xE5000u && st_sector(OBJ_MAKUFAV, 1) == 0xE6000u;
        memset(nor, 0xFF, sizeof nor);
        memcpy(snap, nor, sizeof nor);
        ok &= st_save(OBJ_MAKUFAV, b, sizeof b) == 0 && st_load(OBJ_MAKUFAV, got, sizeof got) == (int)sizeof b &&
              !memcmp(got, b, sizeof b) && st_load(OBJ_AUTOSAVE, got, sizeof got) < 0 &&
              !memcmp(snap, nor, 0xE5000u) && !memcmp(snap + 0xE7000u, nor + 0xE7000u, 0x19000u);
        memset(nor, 0xFF, sizeof nor);
        ok &= st_save(OBJ_AUTOSAVE, a, sizeof a) == 0 && st_load(OBJ_MAKUFAV, got, sizeof got) < 0;
        bad += check("MAKU's likes in the autosave's pair under their own type (neither reads the other), nothing else touched", ok);
    }
    {
        static uint8_t bank[3472], back[3728], tab[3728];
        uint32_t i;
        memset(nor, 0xFF, sizeof nor);
        for (i = 0; i < sizeof bank; i++) bank[i] = (uint8_t)(i * 7u);
        for (i = 0; i < sizeof tab; i++) tab[i] = (uint8_t)(i * 3u);
        st_save(OBJ_FM6BANK, bank, sizeof bank);         /* A (seq 1) */
        bank[0] ^= 1;
        st_save(OBJ_FM6BANK, bank, sizeof bank);         /* B (seq 2): the newest */
        bad += check("a bank copy does not read as the patches object (its type)", st_load(OBJ_UPFM6, back, sizeof back) < 0);
        bad += check("st_save_to: the first patches write into A, the bank's older copy; the bank's B still loads",
                     st_save_to(OBJ_UPFM6, tab, sizeof tab, 0) == 0 &&
                     st_load(OBJ_UPFM6, back, sizeof back) == (int)sizeof tab && !memcmp(back, tab, sizeof tab) &&
                     st_load(OBJ_FM6BANK, back, sizeof back) == (int)sizeof bank && !memcmp(back, bank, sizeof bank));
        tab[0] ^= 1;
        bad += check("the next save goes to B (over the bank), A/B as any object",
                     st_save(OBJ_UPFM6, tab, sizeof tab) == 0 && st_load(OBJ_FM6BANK, back, sizeof back) < 0 &&
                     st_load(OBJ_UPFM6, back, sizeof back) == (int)sizeof tab && back[0] == tab[0]);
        {
            int erased = 1;
            for (i = 256u + sizeof tab; i < 4096u; i++) erased &= nor[0x9F000u + i] == 0xFFu && nor[0xFE000u + i] == 0xFFu;
            bad += check("FM6 patches: the sector tails stay erased", erased);
        }
    }
    {
        uint8_t copies[2 * ST_SECTOR];
        uint32_t off = st_sector(OBJ_PROJECT0, 0), cut;
        int ok = 1;
        memset(nor, 0xFF, sizeof nor);
        st_save(OBJ_PROJECT0, a, sizeof a);
        st_save(OBJ_PROJECT0, b, sizeof b);
        memcpy(copies, nor + off, sizeof copies);
        for (cut = 0; cut < sizeof a + sizeof(st_hdr_t); cut++) {
            memcpy(nor + off, copies, sizeof copies);
            fail_bytes = (int)cut;
            ok &= st_save(OBJ_PROJECT0, a, sizeof a) != 0;
            fail_bytes = -1;
            ok &= st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof b && !memcmp(got, b, sizeof b);
        }
        bad += check("every payload/header byte cut keeps old data", ok);
        erase_error = 1;
        ok = st_save(OBJ_PROJECT0, a, sizeof a) != 0;
        erase_error = 0;
        bad += check("erase failure keeps old data", ok && st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof b &&
                                                      !memcmp(got, b, sizeof b));
        write_protected = 1;
        ok = st_save(OBJ_PROJECT0, a, sizeof a) != 0;
        write_protected = 0;
        bad += check("write protection is a save error", ok && st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof b &&
                                                            !memcmp(got, b, sizeof b));
    }
    {
        st_hdr_t h;
        uint32_t copy;
        for (copy = 0; copy < 2u; copy++) {
            uint32_t off = st_sector(OBJ_PROJECT0, copy);
            memcpy(&h, nor + off, sizeof h);
            h.seq = 0xFFFFFFFEu + copy;
            h.hcrc = st_crc32(&h, sizeof h - 4u);
            memcpy(nor + off, &h, sizeof h);
        }
        bad += check("save sequence wraps", st_save(OBJ_PROJECT0, a, sizeof a) == 0);
        n = st_load(OBJ_PROJECT0, got, sizeof got);
        bad += check("wrapped sequence loads new data", n == (int)sizeof a && !memcmp(got, a, sizeof a));
        bad += check("save after sequence wrap", st_save(OBJ_PROJECT0, b, sizeof b) == 0 &&
                                                st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof b &&
                                                !memcmp(got, b, sizeof b));
    }
    {   /* a save into a sector that reads blank has no erase; st_tidy erases the stale copy ahead of the next save */
        char c[600], d[600];
        uint32_t A = st_sector(OBJ_PROJECT0, 0), B = st_sector(OBJ_PROJECT0, 1);
        memset(c, 'C', sizeof c);
        memset(d, 'D', sizeof d);
        memset(nor, 0xFF, sizeof nor);
        erase_calls = 0;
        bad += check("saves into blank sectors (A, then B): no erase, both load in turn",
                     st_save(OBJ_PROJECT0, a, sizeof a) == 0 && st_save(OBJ_PROJECT0, b, sizeof b) == 0 && erase_calls == 0 &&
                     st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof b && !memcmp(got, b, sizeof b));
        bad += check("the third save goes over A's first: one erase",
                     st_save(OBJ_PROJECT0, c, sizeof c) == 0 && erase_calls == 1);
        bad += check("st_tidy: the stale copy (B) erased, the current (A) untouched and loading",
                     st_tidy(OBJ_PROJECT0) == 1 && erase_calls == 2 && nor[B] == 0xFFu && nor[A] != 0xFFu &&
                     st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof c && !memcmp(got, c, sizeof c));
        bad += check("  a second tidy does nothing (the other copy is blank)", st_tidy(OBJ_PROJECT0) == 0 && erase_calls == 2);
        bad += check("  the next save goes to the tidied copy with no erase",
                     st_save(OBJ_PROJECT0, d, sizeof d) == 0 && erase_calls == 2 &&
                     st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof d && !memcmp(got, d, sizeof d));
        memset(nor, 0xFF, sizeof nor);
        erase_calls = 0;
        bad += check("st_tidy with no valid copy does nothing", st_tidy(OBJ_PROJECT0) == 0 && erase_calls == 0);
        st_save(OBJ_PROJECT0, a, sizeof a);
        nor[B + 4095u] = 0x7Fu;
        erase_calls = 0;
        bad += check("a stray byte at the very end of a sector: not blank, the save erases it",
                     st_save(OBJ_PROJECT0, b, sizeof b) == 0 && erase_calls == 1 && nor[B + 4095u] == 0xFFu);
        memset(nor, 0xFF, sizeof nor);
        st_save(OBJ_PROJECT0, a, sizeof a);
        st_save(OBJ_PROJECT0, b, sizeof b);
        st_save(OBJ_PROJECT0, c, sizeof c);                /* A current, B stale */
        erase_error = 1;
        bad += check("st_tidy: a failing erase is reported, the current copy still loads",
                     st_tidy(OBJ_PROJECT0) < 0 && st_load(OBJ_PROJECT0, got, sizeof got) == (int)sizeof c && !memcmp(got, c, sizeof c));
        erase_error = 0;
        memset(nor, 0xFF, sizeof nor);
        st_save(OBJ_AUTOSAVE, a, sizeof a);                /* the pair shared with MAKU's likes: A holds an autosave */
        st_save_to(OBJ_MAKUFAV, b, sizeof b, 1);           /* .. the likes in B */
        erase_calls = 0;
        bad += check("st_tidy leaves another object's copy in the shared pair alone",
                     st_tidy(OBJ_MAKUFAV) == 0 && erase_calls == 0 && st_load(OBJ_AUTOSAVE, got, sizeof got) == (int)sizeof a);
    }
    {
        uint32_t calls = io_calls;
        bad += check("invalid object load does no flash access", st_load(OBJ_COUNT, got, sizeof got) < 0 && io_calls == calls);
        bad += check("invalid object save does no flash access", st_save(OBJ_COUNT, a, sizeof a) < 0 && io_calls == calls);
        bad += check("wrapped object rejected", st_save(0xFFFFFFFFu, a, sizeof a) < 0 && io_calls == calls);
        memset(got, 'X', sizeof got);
        bad += check("small destination rejects whole object", st_load(OBJ_PROJECT0, got, sizeof got - 1u) < 0 && got[0] == 'X');
        bad += check("oversized save rejected", st_save(OBJ_PROJECT0, a, ST_PAYLOAD_MAX + 1u) < 0);
    }
    printf("%s\n", bad ? "STORAGE TEST FAILED" : "storage test passed");
    return bad != 0;
}
