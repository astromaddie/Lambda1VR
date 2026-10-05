#include "VrFrameMap.h"

// The map. One physical input does one thing in a context: the rows of an input must not overlap
// (the host test checks that, and that every action can be reached). The same action on two
// inputs is an alias.
static const frameMapRow_t rows[] = {
	// ---- playing
	{ FI_R_TRIGGER, ROLE_TRIGGER_WEAPON, 0, CTX_MENU, ACT_FIRE, "weapon hand trigger" },
	{ FI_L_TRIGGER, ROLE_TRIGGER_OTHER, 0, CTX_MENU, ACT_ALTFIRE, "other hand trigger" },
	{ FI_A, 0, 0, CTX_MENU, ACT_JUMP, "" },
	{ FI_B, 0, 0, CTX_MENU, ACT_CROUCH_HOLD, "" },
	{ FI_X, 0, 0, CTX_MENU, ACT_USE, "" },
	{ FI_Y, 0, 0, CTX_MENU, ACT_RELOAD, "" },
	{ FI_L_STICK_CLICK, 0, 0, CTX_MENU, ACT_CROUCH_TOGGLE, "" },
	{ FI_R_STICK_CLICK, 0, CTX_OFF_ZONE, CTX_MENU, ACT_QUICKSAVE, "off hand behind the head" },
	{ FI_R_STICK_CLICK, 0, 0, CTX_MENU | CTX_OFF_ZONE, ACT_FLASHLIGHT, "" },
	{ FI_VIEW_SHORT, 0, CTX_OFF_ZONE, CTX_MENU, ACT_QUICKLOAD, "off hand behind the head" },
	{ FI_VIEW_SHORT, 0, CTX_MULTI, CTX_MENU | CTX_OFF_ZONE, ACT_SCOREBOARD, "multiplayer" },
	{ FI_VIEW_SHORT, 0, 0, CTX_MENU | CTX_OFF_ZONE | CTX_MULTI, ACT_FLASHLIGHT, "" },
	{ FI_L_STICK_UP, 0, 0, CTX_MENU, ACT_MOVE, "move" },
	{ FI_L_STICK_DOWN, 0, 0, CTX_MENU, ACT_MOVE, "move" },
	{ FI_L_STICK_LEFT, 0, 0, CTX_MENU, ACT_MOVE, "move" },
	{ FI_L_STICK_RIGHT, 0, 0, CTX_MENU, ACT_MOVE, "move" },
	{ FI_R_STICK_LEFT, 0, 0, CTX_MENU, ACT_TURN, "snap or smooth turn" },
	{ FI_R_STICK_RIGHT, 0, 0, CTX_MENU, ACT_TURN, "snap or smooth turn" },
	// (right stick up and down: nothing in the game, they scroll in a menu)
	{ FI_L_BUMPER, 0, 0, CTX_MENU, ACT_PREV_WEAPON, "" },
	{ FI_R_BUMPER, 0, 0, CTX_MENU, ACT_NEXT_WEAPON, "" },
	{ FI_DPAD_LEFT, 0, 0, CTX_MENU, ACT_PREV_WEAPON, "alias of the left bumper" },
	{ FI_DPAD_RIGHT, 0, 0, CTX_MENU, ACT_NEXT_WEAPON, "alias of the right bumper" },
	{ FI_DPAD_UP, 0, 0, CTX_MENU, ACT_LAST_WEAPON, "" },
	{ FI_DPAD_DOWN, 0, 0, CTX_MENU, ACT_LASER, "laser sight, steadies a scope" },
	{ FI_R_GRIP, ROLE_GRIP_WEAPON, CTX_DOM_ZONE, CTX_MENU, ACT_BACKPACK, "the crowbar, from behind the head" },
	{ FI_R_GRIP, ROLE_GRIP_WEAPON, 0, CTX_MENU | CTX_DOM_ZONE, ACT_USE, "alias of X: grab, push, pull, press" },
	{ FI_L_GRIP, ROLE_GRIP_OTHER, 0, CTX_MENU | CTX_OFF_GRIP_AWAY, ACT_STEADY, "two-handed hold, scope: near the weapon" },
	{ FI_L_GRIP, ROLE_GRIP_OTHER, CTX_OFF_GRIP_AWAY, CTX_MENU, ACT_USE_OFF, "grab with this hand: well away from the weapon" },
	{ FI_MENU_SHORT, 0, 0, CTX_MENU, ACT_PAUSE, "" },
	{ FI_MENU_HOLD, 0, 0, CTX_MENU, ACT_QUICKSAVE, "held for half a second" },
	{ FI_VIEW_HOLD_1S, 0, 0, 0, ACT_RECENTRE, "" },
	{ FI_VIEW_HOLD_3S, 0, 0, 0, ACT_RECENTRE_HEIGHT, "" },

	// ---- in a menu (the laser points from the weapon hand)
	{ FI_R_TRIGGER, ROLE_TRIGGER_WEAPON, CTX_MENU, 0, ACT_MENU_CLICK, "pointer click" },
	{ FI_A, 0, CTX_MENU, 0, ACT_MENU_CONFIRM, "" },
	{ FI_B, 0, CTX_MENU, 0, ACT_MENU_BACK, "" },
	{ FI_MENU_SHORT, 0, CTX_MENU, 0, ACT_MENU_BACK, "alias of B" },
	{ FI_VIEW_SHORT, 0, CTX_MENU, 0, ACT_MENU_BACK, "alias of B" },
	{ FI_DPAD_UP, 0, CTX_MENU, 0, ACT_MENU_UP, "" },
	{ FI_DPAD_DOWN, 0, CTX_MENU, 0, ACT_MENU_DOWN, "" },
	{ FI_DPAD_LEFT, 0, CTX_MENU, 0, ACT_MENU_LEFT, "" },
	{ FI_DPAD_RIGHT, 0, CTX_MENU, 0, ACT_MENU_RIGHT, "" },
	{ FI_L_STICK_UP, 0, CTX_MENU, 0, ACT_MENU_UP, "alias of the D-pad" },
	{ FI_L_STICK_DOWN, 0, CTX_MENU, 0, ACT_MENU_DOWN, "alias of the D-pad" },
	{ FI_L_STICK_LEFT, 0, CTX_MENU, 0, ACT_MENU_LEFT, "alias of the D-pad" },
	{ FI_L_STICK_RIGHT, 0, CTX_MENU, 0, ACT_MENU_RIGHT, "alias of the D-pad" },
	{ FI_L_BUMPER, 0, CTX_MENU, 0, ACT_MENU_PAGE_PREV, "previous tab or page" },
	{ FI_R_BUMPER, 0, CTX_MENU, 0, ACT_MENU_PAGE_NEXT, "next tab or page" },
	{ FI_R_STICK_UP, 0, CTX_MENU, 0, ACT_MENU_SCROLL_UP, "scroll" },
	{ FI_R_STICK_DOWN, 0, CTX_MENU, 0, ACT_MENU_SCROLL_DOWN, "scroll" },
};

const frameMapRow_t* VrFrameMap_Rows(int* count)
{
	*count = (int)(sizeof(rows) / sizeof(rows[0]));
	return rows;
}

int VrFrameMap_RoleInput(int role, bool leftHanded)
{
	switch (role)
	{
		case ROLE_TRIGGER_WEAPON: return leftHanded ? FI_L_TRIGGER : FI_R_TRIGGER;
		case ROLE_TRIGGER_OTHER: return leftHanded ? FI_R_TRIGGER : FI_L_TRIGGER;
		case ROLE_GRIP_WEAPON: return leftHanded ? FI_L_GRIP : FI_R_GRIP;
		case ROLE_GRIP_OTHER: return leftHanded ? FI_R_GRIP : FI_L_GRIP;
		default: return -1;
	}
}

int VrFrameMap_Action(int input, int context, bool leftHanded)
{
	const int count = (int)(sizeof(rows) / sizeof(rows[0]));
	for (int i = 0; i < count; i++)
	{
		const frameMapRow_t* row = &rows[i];
		const int rowInput = row->role ? VrFrameMap_RoleInput(row->role, leftHanded) : row->input;
		if (rowInput != input)
		{
			continue;
		}
		if ((context & row->require) != row->require || (context & row->forbid) != 0)
		{
			continue;
		}
		return row->action;
	}
	return ACT_NONE;
}

const char* VrFrameMap_InputName(int input)
{
	static const char* const names[FI_COUNT] = {
		"left trigger", "right trigger", "left grip", "right grip", "left bumper", "right bumper",
		"left stick click", "right stick click", "A", "B", "X", "Y",
		"Menu (short)", "Menu (hold 0.5 s)", "View (short)", "View (hold 1 s)", "View (hold 3 s)",
		"D-pad up", "D-pad down", "D-pad left", "D-pad right",
		"left stick up", "left stick down", "left stick left", "left stick right",
		"right stick up", "right stick down", "right stick left", "right stick right"};
	return (input >= 0 && input < FI_COUNT) ? names[input] : "?";
}

const char* VrFrameMap_ActionName(int action)
{
	static const char* const names[ACT_COUNT] = {
		"none",
		"fire", "alt fire", "use", "use (other hand)", "reload", "jump", "crouch (hold)", "crouch (toggle)",
		"flashlight", "scoreboard", "laser sight / scope steady",
		"move", "turn",
		"previous weapon", "next weapon", "last weapon",
		"backpack weapon", "hold the gun steady",
		"pause", "quick save", "quick load",
		"recentre", "recentre and set height",
		"menu click", "menu confirm", "menu back",
		"menu up", "menu down", "menu left", "menu right",
		"menu previous tab", "menu next tab", "menu scroll up", "menu scroll down"};
	return (action >= 0 && action < ACT_COUNT) ? names[action] : "?";
}
