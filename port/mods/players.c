// Players mod (FP_MOD_PLAYERS=0 keeps the original): an Ad Hoc match takes as many people as it has
// places, up to 16, instead of 8.
//
// The host picks the match's size in menu_server_settings.csl: 2, 4, 6, 8, then "10/12/14/16
// PLAYERS (8 HUMANS)" (Infrastructure, the PSN's mode, had "(6 HUMANS)" and "(4 HUMANS)"). Three
// things kept people at 8, none of them a table: the match has 16 places (the bots', which a person
// takes, FUN_0890f9b4), the connections are a list and the lobby shows two teams of 8.
//  - StartServer passes num_humans = min(max_players, 8) (mods/scripts.c makes it max_players) and
//    the native side (FUN_0890872c) clamps it to 8 again before keeping it in the network game's
//    object (*(0x08b09920) + 0xac), where the host's join handler (FUN_08909558) compares its
//    connections with it ("game full"): set again right after StartServer.
//  - The host lets those asking to join in one at a time, every 5 seconds, in two queues (below):
//    with people arriving together, whoever waited ~30 s gave up and went back to the search
//    ("GAME NOT FOUND"), so the eighth and later never got in, as if the limit were still 8. One a
//    second instead.
//  - The size options read "10 PLAYERS" ... "16 PLAYERS" (mods/strings.c).
// Tested with 16 copies on one machine: host + 15 arriving within 10 s, all of them in the match.
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

#define NETGAME_PTR 0x08b09920u  // the network game's object; +0: 1 on Ad Hoc, 0 on the PSN
#define NETGAME_MAX_HUMANS 0xacu

bool mod_players_on(void) {
	static int on = -1;
	if (on < 0) {
		const char *e = getenv("FP_MOD_PLAYERS");
		on = !e || atoi(e) != 0;
	}
	return on;
}

// FUN_0890872c(mode, map, max_players, num_humans): starts hosting.
void f_0890872c(CPU *c) {
	uint32_t places = c->r[6], humans = c->r[7];
	f_0890872c_orig(c);
	if (!mod_players_on()) return;
	uint32_t ng = M32(NETGAME_PTR);
	if (!ng || M32(ng) != 1) return;  // Ad Hoc only
	uint32_t want = humans < places ? humans : places;
	if (want > 16) want = 16;
	if (want > M32(ng + NETGAME_MAX_HUMANS)) {
		M32(ng + NETGAME_MAX_HUMANS) = want;
		rt_log("players mod: this match takes %u people", want);
	}
}

// The host lets the people asking to join in one at a time, in two queues of 24 places each
// worked through every 5 seconds (now + 5000 ms, FUN_0898c5b8's clock): FUN_0890c778 sends the next
// one the room (every player's 236-byte record, ~4 KB), FUN_0890cbf0 the level's state. Someone
// waiting too long gives up (~30 s) and goes back to the search ("GAME NOT FOUND"): with people
// arriving together the eighth and later never got in (two queues x 5 s each). One a second
// instead: 16 people arriving at once are all in within about 15 s (a few KB each, small for
// today's networks).
#define JOIN_ROOM_NEXT_AT 0x08afbbe8u
#define JOIN_LEVEL_NEXT_AT 0x08afbbe0u
#define JOIN_STEP_MS 1000u

static void quicker(uint32_t next_at, uint32_t before) {
	uint32_t after = M32(next_at);
	if (mod_players_on() && after != before && after > 5000u - JOIN_STEP_MS) M32(next_at) = after - (5000u - JOIN_STEP_MS);
}

void f_0890c778(CPU *c) {
	uint32_t before = M32(JOIN_ROOM_NEXT_AT);
	f_0890c778_orig(c);
	quicker(JOIN_ROOM_NEXT_AT, before);
}

void f_0890cbf0(CPU *c) {
	uint32_t before = M32(JOIN_LEVEL_NEXT_AT);
	f_0890cbf0_orig(c);
	quicker(JOIN_LEVEL_NEXT_AT, before);
}
