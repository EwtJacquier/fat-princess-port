// Online mod (FP_MOD_ONLINE=0 keeps the original): Ad Hoc matches over the internet ride out a
// connection that stops for a few seconds.
//
// When a network game starts (FUN_0890173c, FUN_0890872c) the game sets how long a peer may stay
// silent before its connection counts as lost (FUN_089aa070(session, ms): the global 0x08b08f78
// and each peer's; FUN_089065cc then shows "ConnectionLost_General"): 20 s on the PSN
// (Infrastructure), 4 s on Ad Hoc, where the PSPs were a room apart. Here Ad Hoc runs over the
// internet (LiveKit): a browser's LiveKit connection that times out reconnects by itself in 2-3 s,
// a Wi-Fi stalls a moment, and 4 s dropped those players. Ad Hoc gets 15 s.
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

#define ADHOC_PEER_TIMEOUT 4000u
#define ONLINE_PEER_TIMEOUT 15000u

static bool online_on(void) {
	static int on = -1;
	if (on < 0) {
		const char *e = getenv("FP_MOD_ONLINE");
		on = !e || atoi(e) != 0;
	}
	return on;
}

void f_089aa070(CPU *c) {
	if (online_on() && c->r[5] == ADHOC_PEER_TIMEOUT) c->r[5] = ONLINE_PEER_TIMEOUT;
	f_089aa070_orig(c);
}
