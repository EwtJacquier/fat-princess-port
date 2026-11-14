// Menu mod (FP_MOD_MENU=0 keeps the original): plainer names for the menus' options, and "< VALUE >"
// on the options that change with left/right.
//
// The game's text comes from string tables ("BRTS" files, strings/<name><language>.sf): entries
// sorted by the CRC-32 (without the final inversion) of the text's id, "str:<id>", each pointing at
// a UTF-16 string. FUN_089fdb34(table, hash) looks one up (a binary search) and returns the
// string's address; FUN_089fe1d4(manager, hash) asks every loaded table, then the texts made at run
// time, and else shows "Missing Text".
//
// Names: a renamed text is handed back from a copy of ours in guest memory, only when the table's own
// text is the English one (other languages keep theirs).
//
// Left/right options: mods/scripts.c makes the menu script ask for their value as "_mid" .. name
// instead of "str:" .. name. That id is in no table; as the hash is a CRC, the "str:" one follows
// from it and the name's length alone (a CRC over the same bytes after two different starts differs
// by the starts' difference carried through those bytes), so the lengths 1 to 64 are tried and the
// one found in a table is the value, shown as "< VALUE >" (the box's width is measured from it too).
#include <stdlib.h>
#include <string.h>

#include "funcs.h"
#include "rt.h"

typedef struct {
	uint32_t hash;      // ~crc32("str:<id>")
	const char *orig;   // the English text
	const char *text;   // ours
	uint32_t addr;      // our copy (UTF-16) in guest memory
	bool players;       // the players mod's (mods/players.c), not the menu mod's
} Rename;

static Rename renames[] = {
	{0x5421a011u, "PLAY WITH YOURSELF", "PLAY OFFLINE", 0},               // str:play_with_yourself
	{0x72934f96u, "PLAY WITH OTHERS", "PLAY ONLINE", 0},                  // str:play
	{0xeeeccab3u, "TWIDDLY KNOBS", "CUSTOMIZE", 0},                       // str:twiddly_knobs
	{0x398a453fu, "LEGEND OF THE FAT PRINCESS", "STORY MODE", 0},         // str:legend
	{0x830f7275u, "MESS ABOUT", "CASUAL MODE", 0},                        // str:mess_about
	{0x9034e6f6u, "GLADIATE", "ARENA MODE", 0},                           // str:gladiate
	{0x9699a2c7u, "GET FABULOUS", "CHANGE LOOK", 0},                      // str:get_fabulous
	{0x8fb39f9bu, "How To Play!", "How To Play", 0},                      // str:confused
	{0x7de8fd8bu, "WHO'S YOUR DADDY?", "CREDITS", 0},                     // str:option_credits_button
	{0x7d97bd39u, "CONTROL FREAKS", "HOST A GAME", 0},                    // str:control_freaks
	// players mod: an Ad Hoc match takes as many people as it has places
	{0x0214efd0u, "10 PLAYERS (8 HUMANS)", "10 PLAYERS", 0, true},        // str:player_10_8h
	{0xa81d275bu, "12 PLAYERS (8 HUMANS)", "12 PLAYERS", 0, true},        // str:player_12_8h
	{0x8d767887u, "14 PLAYERS (8 HUMANS)", "14 PLAYERS", 0, true},        // str:player_14_8h
	{0x277fb00cu, "16 PLAYERS (8 HUMANS)", "16 PLAYERS", 0, true},        // str:player_16_8h
};

bool mod_players_on(void);  // mods/players.c

static bool enabled(void) {
	static int on = -1;
	if (on < 0) {
		const char *e = getenv("FP_MOD_MENU");
		on = !e || atoi(e) != 0;
	}
	return on;
}

// ---- our texts in guest memory (one block, never freed: the game may keep the pointers) ----------
static uint32_t pool, pool_used, pool_size;

static uint32_t pool_take(uint32_t bytes) {
	bytes = (bytes + 3) & ~3u;
	if (!pool) {
		pool_size = 0x8000;
		pool = rt_alloc_user(pool_size, true, "mod texts");
		if (!pool) return 0;
	}
	if (pool_used + bytes > pool_size) return 0;
	uint32_t a = pool + pool_used;
	pool_used += bytes;
	return a;
}

static bool same_text(uint32_t addr, const char *s) {  // a UTF-16 string in guest memory vs ASCII
	for (;; addr += 2, s++) {
		uint16_t ch = M16(addr);
		if (ch != (uint8_t)*s) return false;
		if (!ch) return true;
	}
}

static uint32_t our_copy(Rename *r) {
	if (!r->addr) {
		size_t n = strlen(r->text) + 1;
		uint32_t a = pool_take((uint32_t)(n * 2));
		if (!a) return 0;
		for (size_t i = 0; i < n; i++) M16(a + 2 * i) = (uint8_t)r->text[i];
		r->addr = a;
	}
	return r->addr;
}

void f_089fdb34(CPU *c) {
	uint32_t hash = c->r[5];
	f_089fdb34_orig(c);
	if (!c->r[2]) return;
	for (size_t i = 0; i < sizeof renames / sizeof *renames; i++) {
		Rename *r = &renames[i];
		if (r->hash != hash) continue;
		if ((r->players ? mod_players_on() : enabled()) && same_text(c->r[2], r->orig)) {
			uint32_t a = our_copy(r);
			if (a) c->r[2] = a;
		}
		return;
	}
}

// ---- left/right options: "< VALUE >" --------------------------------------------------------------
static uint32_t crc_table[256];

static uint32_t id_hash(const char *s) {  // the game's: CRC-32 without the final inversion
	uint32_t r = 0xffffffffu;
	for (; *s; s++) r = crc_table[(r ^ (uint8_t)*s) & 0xff] ^ (r >> 8);
	return r;
}

static uint32_t call2(CPU *c, uint32_t func, uint32_t a0, uint32_t a1) {  // a guest function's result
	CPU saved = *c;
	c->r[4] = a0;
	c->r[5] = a1;
	c->r[31] = 0;
	rt_call(c, func);
	uint32_t v = c->r[2];
	*c = saved;
	return v;
}

// What FUN_089fe1d4 finds for a hash before "Missing Text": the tables (through the renames), then
// the texts made at run time. 0 when none has it.
static uint32_t find_text(CPU *c, uint32_t mgr, uint32_t hash) {
	for (uint32_t n = M32(mgr); n != mgr; n = M32(n)) {
		uint32_t table = M32(n + 4);
		if (!table) break;
		uint32_t s = call2(c, 0x089fdb34u, table, hash);
		if (s) return s;
	}
	for (uint32_t n = M32(mgr + 12); n != mgr + 12; n = M32(n)) {
		uint32_t e = M32(n + 4);
		if (!e) break;
		if (M32(e + 0xc) == hash) return M32(e + 0x10);
	}
	return 0;
}

// hashes seen: 0 = an ordinary id (or one with no text), else our "< VALUE >" for a marked one
enum { SEEN_SIZE = 4096 };
static struct { uint32_t hash, text; bool used; } seen[SEEN_SIZE];

static int seen_slot(uint32_t hash) {
	uint32_t i = (hash * 2654435761u) % SEEN_SIZE;
	for (int k = 0; k < SEEN_SIZE; k++, i = (i + 1) % SEEN_SIZE) {
		if (!seen[i].used || seen[i].hash == hash) return (int)i;
	}
	return -1;
}

static uint32_t bracketed(uint32_t text) {  // "< " + text + " >" (UTF-16) in our pool
	uint32_t n = 0;
	while (M16(text + 2 * n) && n < 200) n++;
	uint32_t a = pool_take((n + 5) * 2);
	if (!a) return 0;
	M16(a) = '<';
	M16(a + 2) = ' ';
	for (uint32_t i = 0; i < n; i++) M16(a + 4 + 2 * i) = M16(text + 2 * i);
	M16(a + 4 + 2 * n) = ' ';
	M16(a + 6 + 2 * n) = '>';
	M16(a + 8 + 2 * n) = 0;
	return a;
}

static uint32_t marked_value(CPU *c, uint32_t mgr, uint32_t hash) {
	static uint32_t diff;  // id_hash("_mid") ^ id_hash("str:")
	if (!crc_table[1]) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t r = i;
			for (int k = 0; k < 8; k++) r = (r >> 1) ^ (r & 1 ? 0xedb88320u : 0);
			crc_table[i] = r;
		}
		diff = id_hash("_mid") ^ id_hash("str:");
	}
	uint32_t d = diff;
	for (int len = 1; len <= 64; len++) {
		d = crc_table[d & 0xff] ^ (d >> 8);  // the difference carried through one more byte
		uint32_t text = find_text(c, mgr, hash ^ d);
		if (text) return bracketed(text);
	}
	return 0;
}

void f_089fe1d4(CPU *c) {
	uint32_t mgr = c->r[4], hash = c->r[5];
	if (enabled()) {
		int i = seen_slot(hash);
		if (i >= 0 && seen[i].used) {
			if (seen[i].text) {
				c->r[2] = seen[i].text;
				return;
			}
		} else if (i >= 0) {
			// an id with a text: ordinary; else maybe a marked one (a hash without either, or with
			// its table not loaded yet, is asked again next time)
			bool ordinary = find_text(c, mgr, hash) != 0;
			uint32_t text = ordinary ? 0 : marked_value(c, mgr, hash);
			if (ordinary || text) {
				seen[i].used = true;
				seen[i].hash = hash;
				seen[i].text = text;
			}
			if (text) {
				c->r[2] = text;
				return;
			}
		}
	}
	f_089fe1d4_orig(c);
}
