// The game's flow lives in compiled scripts (CSL V0.7.1 by Game Toolworks: "FITG"/"MLSC" images in
// FP.wad's scripts archive; re/tools/csldis.py disassembles them). Two mods change a few
// instructions of them as each image is loaded, same length (nop = 0x09), each patch only where
// the original bytes are found:
//
// Boot (FP_MOD_BOOT=0 keeps the original):
//  - startup_screen_sequence goes straight to play_intro_movie: no epilepsy warning, ESRB and Sony
//    screens (the movie itself is not played by this port);
//  - the boot memory stick dialog is answered Yes (util.LoadSaveData: the save is loaded and
//    autosave stays on; on a memory stick without one, mods/save.c has the game make it);
//  - Start on the title accepts the online agreement (accept_eula: agreed_to_eula, then the
//    menus) instead of showing it.
// Menu (FP_MOD_MENU=0 keeps the original; the new names are mods/strings.c):
//  - the main menu has three options: no LOAD GAME (the save loads at boot);
//  - PLAY ONLINE goes straight to ad hoc (no Infrastructure / Ad-Hoc menu), and a network error
//    returns to the main menu;
//  - the PLAY ONLINE menu: FIND A GAME and HOST A GAME (no JUMP IN!);
//  - options changed with left/right show their value as "< VALUE >";
//  - no copyright line under the menus.
// Players (FP_MOD_PLAYERS=0 keeps the original; the rest is mods/players.c):
//  - an Ad Hoc match takes as many people as it has places (up to 16), not 8.
//
// FUN_08a8963c finishes loading an image: when it says yes, *(this + 0x14) is the "MLSC" block in
// memory (u32 size at +4); the patches' offsets count from there (the file offset minus 8).
#include <stdlib.h>
#include <string.h>

#include "funcs.h"
#include "rt.h"

enum { BOOT, MENU, PLAYERS };

typedef struct {
	int mod;
	const char *script;  // the image's source name (…\scripts\NAME.csl)
	uint32_t at, len;
	const char *old, *new;
} Patch;

static const Patch patches[] = {
	// startup_screen_sequence: show_epilepsy_screen -> play_intro_movie
	{BOOT, "startup", 0x11b8, 2, "\x95\x07", "\x95\x09"},
	// pressed_start, agreement not accepted yet: show_eula -> accept_eula
	{BOOT, "startup", 0x2228, 2, "\x95\x12", "\x95\x0a"},
	// ShowBootupMemoryStickWarning: util.LoadSaveData() and return (what Yes does through
	// load_save_data, without its wait for the system dialog: called here, out of a script thread,
	// that loop can't wait)
	{BOOT, "fp_ui_messages", 0x0f14, 9, "\x90\x2c\x00\x94\x01\x0d\x52\x00\x4d", "\x95\x31\x82\x64\x96\x00\x0f\x11\x0a"},
	// ReturnToSelectNetworkTypeMenu: ShowMenu(select_network_type) -> ShowMenu(main_menu)
	{MENU, "fp_ui_messages", 0x01f9, 2, "\x93\x48", "\x93\x32"},
	// get_second_option_function: OpenSelectNetworkTypeMenu -> SelectAdHocNetworkType
	{MENU, "mainmenu", 0x0cb9, 2, "\x95\x0a", "\x95\x04"},
	// Create: the labels and the handlers, 3 of each (no str:load_game, no LoadGame)
	{MENU, "mainmenu", 0x0dfe, 2, "\x96\x04", "\x96\x03"},
	{MENU, "mainmenu", 0x0e12, 4, "\xb3\x00\x85\x10", "\x09\x09\x09\x09"},
	{MENU, "mainmenu", 0x0e17, 2, "\x96\x04", "\x96\x03"},
	{MENU, "mainmenu", 0x0e29, 3, "\x95\x0d\x10", "\x09\x09\x09"},
	// Create: no SetOptionChooseable(4, 1)
	{MENU, "mainmenu", 0x0e52, 9, "\x95\x1d\x82\x41\x96\x04\x96\x01\x0f", "\x09\x09\x09\x09\x09\x09\x09\x09\x09"},
	// EnableLoadGame / DisableLoadGame: always the early return (option 4 is gone)
	{MENU, "mainmenu", 0x0986, 2, "\x85\x03", "\x0c\x09"},
	{MENU, "mainmenu", 0x09de, 2, "\x85\x03", "\x0c\x09"},
	// CreateMainMenuScreen: jump over the MenuNewElement of the copyright line under the menus
	// (the picture UI/fe_titlescreen_scea)
	{MENU, "mainmenu", 0x0ba9, 2, "\x95\x17", "\x84\x4b"},
	// menu_play Create (the PLAY ONLINE menu): 2 labels and 2 handlers, no str:jump_in / JumpIn
	{MENU, "menu_play", 0x027e, 2, "\x96\x03", "\x96\x02"},
	{MENU, "menu_play", 0x0283, 3, "\x93\x22\x10", "\x09\x09\x09"},
	{MENU, "menu_play", 0x028d, 2, "\x96\x03", "\x96\x02"},
	{MENU, "menu_play", 0x0292, 3, "\x95\x01\x10", "\x09\x09\x09"},
	// menu_play Create: no SetOptionChooseable(3, 1)
	{MENU, "menu_play", 0x02b4, 9, "\x95\x08\x82\x25\x96\x03\x96\x01\x0f", "\x09\x09\x09\x09\x09\x09\x09\x09\x09"},
	// The vmenu's left/right options (a list of texts: SetOptionTextList, Prev/NextOptionText,
	// SetCurrentOptionText[Index], and the box width measured from it): their text's id is
	// "_mid" .. name instead of "str:" .. name, which mods/strings.c shows as "< TEXT >"
	{MENU, "menu_fp_vmenu", 0x249a, 3, "\xb3\x00\xa3", "\xb3\x00\xc4"},
	{MENU, "menu_fp_vmenu", 0x24ac, 3, "\xb3\x00\xa3", "\xb3\x00\xc4"},
	{MENU, "menu_fp_vmenu", 0x27e1, 3, "\xb3\x00\xa3", "\xb3\x00\xc4"},
	{MENU, "menu_fp_vmenu", 0x2959, 3, "\xb3\x00\xa3", "\xb3\x00\xc4"},
	{MENU, "menu_fp_vmenu", 0x2a8c, 3, "\xb3\x00\xa3", "\xb3\x00\xc4"},
	{MENU, "menu_fp_vmenu", 0x2b90, 3, "\xb3\x00\xa3", "\xb3\x00\xc4"},
	// menu_server_settings StartServer, Ad Hoc: num_humans = 8 (above 8 places) -> = max_players
	{PLAYERS, "menu_server_settings", 0x06e5, 8, "\x96\x03\x96\x24\x96\x08\x54\x4d", "\x96\x03\x96\x24\x14\x02\x54\x4d"},
};

static bool mod_on(int mod) {
	static const char *const env[] = {"FP_MOD_BOOT", "FP_MOD_MENU", "FP_MOD_PLAYERS"};
	static int on[3] = {-1, -1, -1};
	if (on[mod] < 0) {
		const char *e = getenv(env[mod]);
		on[mod] = !e || atoi(e) != 0;
	}
	return on[mod];
}

// The image's script name, from its source path (…\scripts\NAME.csl), or NULL.
static bool is_script(const uint8_t *img, uint32_t size, const char *name) {
	char want[64];
	snprintf(want, sizeof want, "\\scripts\\%s.csl", name);
	size_t n = strlen(want) + 1;  // with the NUL
	for (uint32_t i = 0; i + n <= size; i++) {
		if (img[i] == '\\' && memcmp(img + i, want, n) == 0) return true;
	}
	return false;
}

void f_08a8963c(CPU *c) {
	uint32_t self = c->r[4], ok = c->r[5];
	f_08a8963c_orig(c);
	if (!M32(ok)) return;
	uint32_t base = M32(self + 0x14);
	if (!base || M32(base) != 0x43534c4du) return;
	uint32_t size = M32(base + 4);
	uint8_t *img = (uint8_t *)MP(base);
	const char *last = NULL;
	bool match = false;
	for (size_t i = 0; i < sizeof patches / sizeof *patches; i++) {
		const Patch *p = &patches[i];
		if (!mod_on(p->mod)) continue;
		if (p->script != last) {
			last = p->script;
			match = is_script(img, size, p->script);
		}
		if (!match || p->at + p->len > size) continue;
		if (memcmp(img + p->at, p->old, p->len) == 0) {
			memcpy(img + p->at, p->new, p->len);
			rt_log("mod: %s.csl patched at 0x%x (%s)", p->script, p->at, p->mod == BOOT ? "boot" : p->mod == MENU ? "menu" : "players");
		} else if (memcmp(img + p->at, p->new, p->len) != 0) {
			rt_log("mod: %s.csl at 0x%x is not the expected code; left alone", p->script, p->at);
		}
	}
}
