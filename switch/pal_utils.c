#include "main.h"
#include "pal_config.h"


int ctrl = 0;
int sRelease = 0;

int stick_position(int direction) {
	int axis;
	int axisValue;

	if (direction == STICK_LEFT || direction == STICK_RIGHT) axis = 0;
	if (direction == STICK_UP || direction == STICK_DOWN) axis = 1;

	axisValue = PAL_JoystickGetAxis(axis);

	return (axisValue < -16384 && direction == STICK_LEFT)
	  || (axisValue > 16384 && direction == STICK_RIGHT)
	  || (axisValue < -16384 && direction == STICK_UP)
	  || (axisValue > 16384 && direction == STICK_DOWN);
}

static int input_event_filter(const SDL_Event *lpEvent, volatile PALINPUTSTATE *state)
{
	int sRight, sLeft, sDown, sUp;

	switch (lpEvent->type)
	{
	case SDL_JOYAXISMOTION:
		sRight = stick_position(STICK_RIGHT);
		sLeft = stick_position(STICK_LEFT);
		sDown = stick_position(STICK_DOWN);
		sUp = stick_position(STICK_UP);

		if (sRight | (sRight && sDown))
		{
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirEast;
			state->dwKeyPress |= kKeyRight;
			sRelease = 1;
		}
		else if (sLeft | (sLeft && sUp))
		{
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirWest;
			state->dwKeyPress |= kKeyLeft;
			sRelease = 1;
		}
		else if (sDown | (sDown && sLeft))
		{
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirSouth;
			state->dwKeyPress |= kKeyDown;
			sRelease = 1;
		}
		else if (sUp | (sUp && sRight))
		{
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirNorth;
			state->dwKeyPress |= kKeyUp;
			sRelease = 1;
		}
		else if (sRelease)
		{
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirUnknown;
			state->dwKeyPress = kKeyNone;
			sRelease = 0;
		}
		return 1;

	case SDL_JOYBUTTONDOWN:
		switch (lpEvent->jbutton.button)
		{
		case BTN_UP:
			if (ctrl)
			{
				state->dwKeyPress |= kKeyPgUp;
			}
			else
			{
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirNorth;
				state->dwKeyPress |= kKeyUp;
			}
			break;

		case BTN_DOWN:
			if (ctrl)
			{
				state->dwKeyPress |= kKeyPgDn;
			}
			else
			{
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirSouth;
				state->dwKeyPress |= kKeyDown;
			}
			break;

		case BTN_LEFT:
			if (ctrl)
			{
				state->dwKeyPress |= kKeyHome;
			}
			else
			{
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirWest;
				state->dwKeyPress |= kKeyLeft;
			}
			break;

		case BTN_RIGHT:
			if (ctrl)
			{
				state->dwKeyPress |= kKeyEnd;
			}
			else
			{
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirEast;
				state->dwKeyPress |= kKeyRight;
			}
			break;

		case BTN_A:
			state->dwKeyPress |= (ctrl) ? kKeyDefend : kKeySearch;
			break;

		case BTN_B:
			state->dwKeyPress |= (ctrl) ? kKeyFlee : kKeyMenu;
			break;

		case BTN_Y:
			state->dwKeyPress |= (ctrl) ? kKeyAuto : kKeyForce;
			break;

		case BTN_X:
			state->dwKeyPress |= (ctrl) ? kKeyRepeat : kKeyStatus; // FIXME: remove kKeyRepeat if needed
			break;

		case BTN_L:
			ctrl = 1;
			state->dwKeyPress |= kKeyNone;
			break;

		case BTN_R:
			state->dwKeyPress |= (ctrl) ? kKeyThrowItem : kKeyUseItem;
			break;
		}
		return 1;

	case SDL_JOYBUTTONUP:
		switch (lpEvent->jbutton.button)
		{
		case BTN_UP:
			if (state->dir == kDirNorth)
			{
				state->dir = state->prevdir;
			}
			state->prevdir = kDirUnknown;
			break;

		case BTN_DOWN:
			if (state->dir == kDirSouth)
			{
				state->dir = state->prevdir;
			}
			state->prevdir = kDirUnknown;
			break;

		case BTN_LEFT:
			if (state->dir == kDirWest)
			{
				state->dir = state->prevdir;
			}
			state->prevdir = kDirUnknown;
			break;

		case BTN_RIGHT:
			if (state->dir == kDirEast)
			{
				state->dir = state->prevdir;
			}
			state->prevdir = kDirUnknown;
			break;

		case BTN_L:
			ctrl = 0;
			break;
		}
		return 1;
	}
	return 0;
}

BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
)
{
	UTIL_LogOutput(LOGLEVEL_DEBUG, "enter UTIL_GetScreenSize\n");
	if( gConfig.fKeepAspectRatio ) {
		SDL_DisplayMode DM;
		int ret = SDL_GetCurrentDisplayMode(0, &DM);
		if(ret == 0){
			*pdwScreenWidth = DM.w;
			*pdwScreenHeight = DM.h;
		}
		return ret == 0;
	}
	else
	return FALSE;
}

BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return FALSE;
}

void LOG_to_nxlink_stdio(LOGLEVEL level, const char *full_log, const char *user_log){
	printf(full_log);
}

int UTIL_Platform_Startup(
	int argc,
	char* argv[]
)
{
	socketInitializeDefault();              // Initialize sockets
	nxlinkStdio();                          // Redirect stdout and stderr over the network to nxlink
	UTIL_LogAddOutputCallback(LOG_to_nxlink_stdio, 0);
	return 0;
}

INT
UTIL_Platform_Init(
	int argc,
	char* argv[]
)
{
	PAL_RegisterInputFilter(NULL, input_event_filter, NULL);
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
	socketExit();  
}
