// Boot mod (FP_MOD_BOOT=0 keeps the original): the game's tips start off on every boot (with or
// without a save); Settings > Game Settings can still turn them on for the session.
//
// game_settings_man.LoadGameSettings (FUN_088f23dc) fills the settings from the save; the boolean
// ones are bits of the u16 at the start of the settings object (*DAT_08afb5b8; FUN_088f0804 sets
// them): bit 0 blood, bit 1 tips (menu_settings.csl: INDEX_TIPS = 1). Cleared right after it.
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

#define SETTINGS_PTR 0x08afb5b8u
#define TIPS_BIT 0x2u

void f_088f23dc(CPU *c) {
	f_088f23dc_orig(c);
	const char *e = getenv("FP_MOD_BOOT");
	if (e && atoi(e) == 0) return;
	uint32_t s = M32(SETTINGS_PTR);
	if (s) M16(s) &= (uint16_t)~TIPS_BIT;
}
