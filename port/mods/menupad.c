// Menu mod (FP_MOD_MENU=0 keeps the original): the analog stick moves through the menus too.
//
// The menus' input (FUN_089c8654) turns the controller into one event per frame, the first of:
// the d-pad repeating (logical buttons 0xe, 0xf, 0xc, 0xd: "up", "down", "left", "right"), the
// analog stick's virtual buttons repeating (0x13, 0x14, 0x11, 0x12: "analog_up" ...), then the
// other buttons. The menus (menu_fp_vmenu.csl) only listen to the d-pad's events; only the online
// agreement, which the boot mod skips, used the analog ones. FUN_08981964 (is a logical button
// repeating: held, then the auto-repeat) is called by that function alone: asked for a d-pad
// direction, it answers for the analog stick's too, so the stick sends "up" ... with the same
// repeat. In a match the stick and the d-pad stay apart (the d-pad calls helpers and taunts).
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

static bool enabled(void) {
	static int on = -1;
	if (on < 0) {
		const char *e = getenv("FP_MOD_MENU");
		on = !e || atoi(e) != 0;
	}
	return on;
}

void f_08981964(CPU *c) {
	uint32_t pad = c->r[4], button = c->r[5];
	f_08981964_orig(c);
	if (c->r[2] || !enabled()) return;
	uint32_t stick;
	switch (button) {
	case 0xe: stick = 0x13; break;  // up
	case 0xf: stick = 0x14; break;  // down
	case 0xc: stick = 0x11; break;  // left
	case 0xd: stick = 0x12; break;  // right
	default: return;
	}
	c->r[4] = pad;
	c->r[5] = stick;
	f_08981964_orig(c);
}
