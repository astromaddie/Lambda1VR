// Steam Frame controllers: what the runtime says about the physical buttons this frame, and
// the code that turns it into what the game reads. The map itself is VrFrameMap.c.
#ifndef VRINPUTFRAME_H
#define VRINPUTFRAME_H

#include <stdbool.h>

typedef struct {
	float triggerLeft, triggerRight, gripLeft, gripRight;
	bool bumperLeft, bumperRight;
	bool stickClickLeft, stickClickRight;
	bool a, b, x, y, menu, view;
	bool dpadUp, dpadDown, dpadLeft, dpadRight;
	float stickLeftX, stickLeftY, stickRightX, stickRightY;
} frameRaw_t;

// Called once a frame after the Quest-shaped controller states have been read, when the runtime
// is using the Frame's controller profile. Rewrites those states and sends the commands.
void VrInputFrame_Apply(const frameRaw_t* raw);

#endif
