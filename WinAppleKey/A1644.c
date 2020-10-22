#include "driver.h"

BOOLEAN g_FakeFnActive = 0;

void ProcessA1644Buffer(BYTE* buf, ULONG size)
{
	BYTE* pModifier = &buf[0];
	BYTE* pKey1 = &buf[2];
	BYTE* pKey2 = &buf[3];
	BYTE* pKey3 = &buf[4];
	BYTE* pKey4 = &buf[5];
	BYTE* pKey5 = &buf[6];
	BYTE* pKey6 = &buf[7];
	BYTE* pSpecialKey = &buf[8];

	// SwapFnCtrl mode
	if (g_dwSwapFnCtrl)
	{
		// Physical LCtrl pressed
		if (*pModifier & 0x1)
		{
			if(!*pKey1) // And is it pressed alone?
				g_FakeFnActive = TRUE;

			*pModifier &= ~HidLCtrlMask; // Clear LCtrl modifier
		}
		else // Physical LCtrl not pressed
		{
			if (!*pKey1) // Only unset g_FakeFnActive when there is no other key still being pressed
				g_FakeFnActive = FALSE;
		}

		// Physical Fn pressed?
		if (*pSpecialKey & 0x2)
			*pModifier |= HidLCtrlMask; // Set LCtrl modifier
		else
			*pModifier &= ~HidLCtrlMask; // Clear LCtrl modifier
	}
	else // Not SwapFnCtrl mode
		g_FakeFnActive = *pSpecialKey & 0x2; // Set FakeFnActive state based on physical Fn key state

	// Eject Pressed?
	if (*pSpecialKey & 0x1)
		*pKey1 = HidDel; // Set Del key

	*pSpecialKey = 0; //Clear special key

	// Optionally process optional Alt-Cmd swap
	if (g_dwSwapAltCmd)
	{
		if (*pModifier & HidLAltMask)
		{
			*pModifier &= ~HidLAltMask;
			*pModifier |= HidLCmdMask;
		}
		else if (*pModifier & HidLCmdMask)
		{
			*pModifier &= ~HidLCmdMask;
			*pModifier |= HidLAltMask;
		}

		if (*pModifier & HidRAltMask)
		{
			*pModifier &= ~HidRAltMask;
			*pModifier |= HidRCmdMask;
		}
		else if (*pModifier & HidRCmdMask)
		{
			*pModifier &= ~HidRCmdMask;
			*pModifier |= HidRAltMask;
		}
	}

	// Process FakeFn+[key] combination 
	if (g_FakeFnActive && (*pKey1 || *pModifier))
	{
		switch (*pKey1)
		{
		case HidLeft: *pKey1 = HidHome; break;
		case HidRight: *pKey1 = HidEnd; break;
		case HidUp: *pKey1 = HidPgUp; break;
		case HidDown: *pKey1 = HidPgDown; break;
		case HidEnter: *pKey1 = HidInsert; break;
		case HidF1: *pKey1 = HidF13; break;
		case HidF2: *pKey1 = HidF14; break;
		case HidF3: *pKey1 = HidF15; break;
		case HidF4: *pKey1 = HidF16; break;
		case HidF5: *pKey1 = HidF17; break;
		case HidF6: *pKey1 = HidF18; break;
		case HidF7: *pKey1 = HidF19; break;
		case HidF8: *pKey1 = HidF20; break;
		case HidF9: *pKey1 = HidF21; break;
		case HidF10: *pKey1 = HidF22; break;
		case HidF11: *pKey1 = HidF23; break;
		case HidF12: *pKey1 = HidF24; break;
		case HidKeyP: *pKey1 = HidPrtScr; break;
		case HidKeyB: *pKey1 = HidPauseBreak; break;
		case HidKeyS: *pKey1 = HidScrLck; break;
		default:
			if (*pModifier & HidLCtrlMask) // Map Fn+LCtrl to RCtrl
			{
				*pModifier &= ~HidLCtrlMask; // Clear LCtrl
				*pModifier |= HidRCtrlMask; // Set RCtrl
			}
			else
				RtlZeroMemory(buf, size); // Ignore all other Fn+[key] combinations
			break;
		}
	}
	
}