// The game's frame clock follows the frames on the screen (FP_FRAME_CLOCK=0: the real time, as
// before).
//
// FUN_089bc4dc ticks the game's clock once a frame: it reads the time in milliseconds
// (sceKernelGetSystemTimeWide), takes the difference from the last frame's (capped), scales it
// (slow motion) and adds it to the game's time; the simulation moves everything by that step.
// The main thread reads it at the end of a frame's work (~10-17 ms here, the swap thread's drawing
// included), just before it waits for the frame before to go to the screen. That moment wanders
// around the vblanks, while the frames reach the screen exactly a vblank apart (display.c's frame
// pacing): steps of 12, 21, 7, 27, 3, 30 ms for frames shown 16.7 ms apart, and the camera, which
// eases after the player, wobbled (the scenery moved 1, 8, 4, 7 pixels from frame to frame running
// straight) and the player shook on the screen, at any render scale.
//
// What is fixed in that loop is the order: each frame begins (FUN_08977864: the wait on the
// CpuGpuSync flag's 0x20, set by the swap thread after sceDisplaySetFrameBuf) once the frame before
// it is set on the screen, and the clock is read once between two beginnings. So the time read is
// that of the vblank the last frame was set at when this one began (rt_vblank_time_us: 16.683 ms
// each): a frame advances exactly by the vblanks between two frames on the screen (one, or two
// after a late one). The rest of the game (network, sound, timeouts) keeps the real time.
#include <stdlib.h>

#include "funcs.h"
#include "rt.h"

static uint64_t began_after;  // the vblank of the frame on the screen when the current frame began

static bool enabled(void) {
	static int on = -1;
	if (on < 0) on = !getenv("FP_FRAME_CLOCK") || atoi(getenv("FP_FRAME_CLOCK")) != 0;
	return on;
}

// The renderer's begin frame: waits until the frame before is set on the screen.
void f_08977864(CPU *c) {
	f_08977864_orig(c);
	began_after = rt_shown_vblank();
}

void f_089bc4dc(CPU *c) {
	rt_clock_vblanks = enabled() && began_after;
	rt_clock_vblank = began_after;
	f_089bc4dc_orig(c);
	rt_clock_vblanks = false;
}
