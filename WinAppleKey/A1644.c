#include "driver.h"

static BOOLEAN FakeFnActive = 0;

void ProcessA1644Buffer(BYTE* buf, ULONG size)
{
	DebugPrintBuffer("ProcessA1644Buffer(): <= ", buf, size);

	BYTE* pModifier = &buf[0];
	BYTE* pKey1 = &buf[2];
	BYTE* pSpecialKey = &buf[8];

	if (g_dwSwapFnCtrl)
	{
		//Process LCtrl modifier and translate to FakeFn key
		if ((*pModifier & 0x1) && !*pKey1)
			FakeFnActive = TRUE;
		else if (!*pKey1)
			FakeFnActive = FALSE;

		*pModifier &= ~HidLCtrlMask; //Clear LCtrl modifier
	}

	//Process special key input
	if (*pSpecialKey)
	{
		if (*pSpecialKey & 0x1) //Eject (translate to Del)
			*pKey1 = HidDel; //Set Del key

		//If we swap Fn and Ctrl
		if (g_dwSwapFnCtrl)
		{
			if (*pSpecialKey & 0x2) //Fn (translate to LCtrl)
				*pModifier |= HidLCtrlMask; //Set LCtrl modifier
		}
		else
			FakeFnActive = *pSpecialKey & 0x2; //Set fake Fn state based on special key state

		*pSpecialKey = 0; //Clear special key so that the buffer can be understood by hidclass up the stack	
	}

	//Process optional Alt-Cmd swap
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

	//Process Fn+[key] combination 
	if (FakeFnActive && (*pKey1 || *pModifier))
	{
		if (*pKey1 == HidLeft) *pKey1 = HidHome;
		else if (*pKey1 == HidRight) *pKey1 = HidEnd;
		else if (*pKey1 == HidUp) *pKey1 = HidPgUp;
		else if (*pKey1 == HidDown) *pKey1 = HidPgDown;
		else if (*pKey1 == HidEnter) *pKey1 = HidInsert;
		else if (*pKey1 == HidF1) *pKey1 = HidF13;
		else if (*pKey1 == HidF2) *pKey1 = HidF14;
		else if (*pKey1 == HidF3) *pKey1 = HidF15;
		else if (*pKey1 == HidF4) *pKey1 = HidF16;
		else if (*pKey1 == HidF5) *pKey1 = HidF17;
		else if (*pKey1 == HidF6) *pKey1 = HidF18;
		else if (*pKey1 == HidF7) *pKey1 = HidF19;
		else if (*pKey1 == HidF8) *pKey1 = HidF20;
		else if (*pKey1 == HidF9) *pKey1 = HidF21;
		else if (*pKey1 == HidF10) *pKey1 = HidF22;
		else if (*pKey1 == HidF11) *pKey1 = HidF23;
		else if (*pKey1 == HidF12) *pKey1 = HidF24;
		else if (*pKey1 == HidKeyP) *pKey1 = HidPrtScr;
		else if (*pKey1 == HidKeyB) *pKey1 = HidPauseBreak;
		else if (*pKey1 == HidKeyS) *pKey1 = HidScrLck;
		else if (*pModifier & HidLCtrlMask) //Map Fn+LCtrl to RCtrl
		{
			*pModifier &= ~HidLCtrlMask; //Clear LCtrl
			*pModifier |= HidRCtrlMask; //Set RCtrl
		}
		else
			RtlZeroMemory(buf, size); //Ignore all other Fn+[key] combinations
	}
	
	DebugPrintBuffer("ProcessA1644Buffer(): => ", buf, size);
}