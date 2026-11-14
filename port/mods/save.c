// Boot mod, the save (FP_MOD_BOOT=0 keeps the original): the game makes its save at boot when
// the memory stick has none, so autosave is on from the start.
//
// The boot load (util.LoadSaveData, asked for by mods/scripts.c in place of the memory stick
// dialog) ends in FUN_089257d0 (util.HandleAutoLoadResult): with "no data" (0x80110307 / 0x80110309)
// during the boot (DAT_08b01868 == 1) the game goes on to the title and saves nothing until a menu
// calls util.AutoSave (settings, customization, pause). Right after it, this does what
// util.AutoSave (FUN_0892536c) does: when no save or load is running (FUN_089239ec), with the save
// system up (DAT_08b09210) and no error pending (DAT_08b01879), it starts the save (FUN_08923dd4).
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

#define LOAD_RESULT 0x08b09928u  // the last load's result
#define BOOTING     0x08b01868u  // 1 during the boot load
#define SAVE_SYSTEM 0x08b09210u
#define SAVE_ERROR  0x08b01879u
#define SAVE_FLAGS  0x08b05b38u
#define SAVE_FLAG2  0x08b01878u
#define DIALOG_UP   0x08b08becu  // FUN_0898c900: a system dialog is up (FUN_0898c8e4 closes it)

static bool enabled(void) {
	static int on = -1;
	if (on < 0) {
		const char *e = getenv("FP_MOD_BOOT");
		on = !e || atoi(e) != 0;
	}
	return on;
}

static uint32_t call(CPU *c, uint32_t func) {  // a guest function without arguments: its result
	CPU saved = *c;
	c->r[31] = 0;
	rt_call(c, func);
	uint32_t v = c->r[2];
	*c = saved;
	return v;
}

void f_089257d0(CPU *c) {
	uint32_t result = M32(LOAD_RESULT);
	bool booting = M8(BOOTING) == 1;
	f_089257d0_orig(c);
	if (!enabled() || !booting || (result != 0x80110307u && result != 0x80110309u)) return;
	if (!call(c, 0x089239ecu) || !M32(SAVE_SYSTEM) || M8(SAVE_ERROR)) {
		rt_log("mod: no save on the memory stick, and the game can't save now");
		return;
	}
	if (M32(DIALOG_UP)) call(c, 0x0898c8e4u);
	M8(SAVE_FLAGS) &= 0xfd;
	M8(SAVE_FLAG2) = 0;
	call(c, 0x08923dd4u);
	rt_log("mod: no save on the memory stick: the game makes one (autosave on)");
}
