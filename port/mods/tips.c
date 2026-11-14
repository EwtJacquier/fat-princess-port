// Boot mod (FP_MOD_BOOT=0 keeps the original): the game's tips start off on every boot (with or
// without a save); Settings > Game Settings can still turn them on for the session.
//
// game_settings_man.LoadGameSettings (FUN_088f23dc -> FUN_088f1730) copies the settings from the
// save's table of values (*DAT_08b08f08, FUN_08ad1b88 reads a flag by its hash, FUN_08ad1bd8 writes
// it) into the settings object: the boolean ones are bits of the u16 at its start (*DAT_08afb5b8;
// FUN_088f0804 sets them): bit 0 blood, bit 1 tips (menu_settings.csl: INDEX_TIPS = 1), which is
// what the menu shows. The tips themselves (the balloons during a match: "Tap the attack button
// ...", each shown once per save) read the table's flag 0x2d0040e1 (FUN_0882f228, the queue that
// FUN_08832cc8 shows), which only SaveGameSettings (FUN_088f1590) writes back from the bit: both
// are cleared right after the load. Turning them on in the menu sets both again.
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

#define SETTINGS_PTR 0x08afb5b8u
#define TIPS_BIT 0x2u
#define VALUES_PTR 0x08b08f08u        // the save's table of values (a pointer to a pointer)
#define VALUES_TIPS_ON 0x2d0040e1u    // its flag "tips on"
#define VALUES_SET_FLAG 0x08ad1bd8u   // (table, hash, value)

void f_088f23dc(CPU *c) {
	f_088f23dc_orig(c);
	const char *e = getenv("FP_MOD_BOOT");
	if (e && atoi(e) == 0) return;
	uint32_t s = M32(SETTINGS_PTR);
	if (s) M16(s) &= (uint16_t)~TIPS_BIT;
	uint32_t values = M32(VALUES_PTR);
	if (values && M32(values)) {  // the tips (the balloons) off too
		CPU saved = *c;
		c->r[4] = M32(values);
		c->r[5] = VALUES_TIPS_ON;
		c->r[6] = 0;
		c->r[31] = 0;
		rt_call(c, VALUES_SET_FLAG);
		*c = saved;
	}
}
