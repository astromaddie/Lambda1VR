// Steam Frame: the controller map as one table, so it is easy to change and can be checked on
// the host. It follows Half-Life's own Steam Input gamepad layout (the 25th anniversary update's
// xbox_controller_config_standard.vdf: sticks move and turn, RT fires, LT alt fires, A jump,
// B crouch, X use, Y reload, bumpers and D-pad pick weapons, Start pauses and, held, quick saves,
// Select is the flashlight) with the VR parts added: the grips, and the head for looking. The right
// stick's up and down do nothing in the game (it turns left and right), and the flat screen view is a
// setting (vr_flat_screen), not a button.
//
// The Frame's controllers are a split gamepad: the left one has the D-pad, View, a bumper, trigger,
// grip and stick, the right one has A B X Y, Menu, a bumper, trigger, grip and stick. Handedness
// decides which hand is the weapon hand (so which trigger fires and which grip grabs, and where the
// laser comes from); the buttons stay where they physically are.
#ifndef VRFRAMEMAP_H
#define VRFRAMEMAP_H

#include <stdbool.h>
#include <stdint.h>

// Physical inputs. The stick directions are the stick pushed over a threshold, View and Menu
// come as a short press (on release), and a hold that fires once while it's held.
enum {
	FI_L_TRIGGER, FI_R_TRIGGER, FI_L_GRIP, FI_R_GRIP, FI_L_BUMPER, FI_R_BUMPER,
	FI_L_STICK_CLICK, FI_R_STICK_CLICK,
	FI_A, FI_B, FI_X, FI_Y,
	FI_MENU_SHORT, FI_MENU_HOLD, FI_VIEW_SHORT, FI_VIEW_HOLD_1S, FI_VIEW_HOLD_3S,
	FI_DPAD_UP, FI_DPAD_DOWN, FI_DPAD_LEFT, FI_DPAD_RIGHT,
	FI_L_STICK_UP, FI_L_STICK_DOWN, FI_L_STICK_LEFT, FI_L_STICK_RIGHT,
	FI_R_STICK_UP, FI_R_STICK_DOWN, FI_R_STICK_LEFT, FI_R_STICK_RIGHT,
	FI_COUNT
};

// What can happen
enum {
	ACT_NONE,
	ACT_FIRE, ACT_ALTFIRE, ACT_USE, ACT_USE_OFF, ACT_RELOAD, ACT_JUMP, ACT_CROUCH_HOLD, ACT_CROUCH_TOGGLE,
	ACT_FLASHLIGHT, ACT_SCOREBOARD, ACT_LASER,
	ACT_MOVE, ACT_TURN,
	ACT_PREV_WEAPON, ACT_NEXT_WEAPON, ACT_LAST_WEAPON,
	ACT_BACKPACK, ACT_STEADY,
	ACT_PAUSE, ACT_QUICKSAVE, ACT_QUICKLOAD,
	ACT_RECENTRE, ACT_RECENTRE_HEIGHT,
	ACT_MENU_CLICK, ACT_MENU_CONFIRM, ACT_MENU_BACK,
	ACT_MENU_UP, ACT_MENU_DOWN, ACT_MENU_LEFT, ACT_MENU_RIGHT,
	ACT_MENU_PAGE_PREV, ACT_MENU_PAGE_NEXT, ACT_MENU_SCROLL_UP, ACT_MENU_SCROLL_DOWN,
	ACT_COUNT
};

// What's going on around the button
enum {
	CTX_MENU = 1,		// a menu or the console has the keys
	CTX_DOM_ZONE = 2,	// the weapon hand is behind the head (the backpack)
	CTX_OFF_ZONE = 4,	// the other hand is behind the head (quick save and load, as on the Quest)
	CTX_MULTI = 8,		// a multiplayer game
	CTX_OFF_GRIP_AWAY = 16	// the other hand's grip was pressed with that hand well away from the weapon
};

// What one physical input does in a context, ACT_NONE when nothing
int VrFrameMap_Action(int input, int context, bool leftHanded);

// The map as rows, for the docs and the tests. role is 0 for a plain input; or one of the
// weapon-hand / other-hand roles below, which the hand setting turns into a physical input.
enum { ROLE_NONE, ROLE_TRIGGER_WEAPON, ROLE_TRIGGER_OTHER, ROLE_GRIP_WEAPON, ROLE_GRIP_OTHER };

typedef struct {
	int input;			// FI_* (for a role row: one of the right-handed ones)
	int role;
	int require;		// context bits that must be set
	int forbid;			// context bits that must not be
	int action;
	const char* note;
} frameMapRow_t;

const frameMapRow_t* VrFrameMap_Rows(int* count);

const char* VrFrameMap_InputName(int input);
const char* VrFrameMap_ActionName(int action);

// The physical input behind a role
int VrFrameMap_RoleInput(int role, bool leftHanded);

#endif
