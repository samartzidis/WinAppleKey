#include "driver.h"

static BOOLEAN FakeFnActive = 0;
static UCHAR TmpBuf[10] = { 0 };
static UCHAR SpecialKey = 0;
static UCHAR SpecialKeyModifier = 0;

void ProcessA1314Block(PBRB pBrb)
{
	PUCHAR buf = (PUCHAR)pBrb->BrbL2caAclTransfer.Buffer;
	DebugPrintBuffer("ProcessA1314Block(): <= ", buf, pBrb->BrbL2caAclTransfer.BufferSize);

	if (pBrb->BrbL2caAclTransfer.BufferSize == 3) //special key
	{
		SpecialKey = 0;
		SpecialKeyModifier = 0;
		
		if (buf[2] & 0x8) //Eject
			SpecialKey = HidDel; //Del	

		if (g_dwSwapFnCtrl)
		{
			if (buf[2] & 0x10) //Fn
				SpecialKeyModifier |= HidLCtrlMask; //Set LCtrl
		}
		else
			FakeFnActive = buf[2] & 0x10; //Set fake Fn based on value
	}
	else if (pBrb->BrbL2caAclTransfer.BufferSize == 10) //normal key
	{
		RtlCopyMemory(TmpBuf, buf, 10);

		//If we swap Fn and Ctrl
		if (g_dwSwapFnCtrl)
		{
			//Process LCtrl and translate to FakeFn key
			if ((TmpBuf[2] & HidLCtrlMask) && !TmpBuf[4])
				FakeFnActive = TRUE;
			else if (!TmpBuf[4])
				FakeFnActive = FALSE;

			TmpBuf[2] &= ~HidLCtrlMask; //Clear LCtrl
		}

		//Process optional Alt-Cmd swap
		if (g_dwSwapAltCmd)
		{
			if (TmpBuf[2] & HidLAltMask)
			{
				TmpBuf[2] &= ~HidLAltMask;
				TmpBuf[2] |= HidLCmdMask;
			}
			else if (TmpBuf[2] & HidLCmdMask)
			{
				TmpBuf[2] &= ~HidLCmdMask;
				TmpBuf[2] |= HidLAltMask;
			}

			if (TmpBuf[2] & HidRAltMask)
			{
				TmpBuf[2] &= ~HidRAltMask;
				TmpBuf[2] |= HidRCmdMask;
			}
			else if (TmpBuf[2] & HidRCmdMask)
			{
				TmpBuf[2] &= ~HidRCmdMask;
				TmpBuf[2] |= HidRAltMask;
			}
		}

		//Process Fn+[key] combination 
		if (FakeFnActive && (TmpBuf[4] || TmpBuf[2]))
		{
			if (TmpBuf[4] == HidLeft) TmpBuf[4] = HidHome;
			else if (TmpBuf[4] == HidRight) TmpBuf[4] = HidEnd;
			else if (TmpBuf[4] == HidUp) TmpBuf[4] = HidPgUp;
			else if (TmpBuf[4] == HidDown) TmpBuf[4] = HidPgDown;
			else if (TmpBuf[4] == HidEnter) TmpBuf[4] = HidInsert;			
			else if (TmpBuf[4] == HidF1) TmpBuf[4] = HidF13;
			else if (TmpBuf[4] == HidF2) TmpBuf[4] = HidF14;
			else if (TmpBuf[4] == HidF3) TmpBuf[4] = HidF15;
			else if (TmpBuf[4] == HidF4) TmpBuf[4] = HidF16;
			else if (TmpBuf[4] == HidF5) TmpBuf[4] = HidF17;
			else if (TmpBuf[4] == HidF6) TmpBuf[4] = HidF18;
			else if (TmpBuf[4] == HidF7) TmpBuf[4] = 0xB4; //Back
			else if (TmpBuf[4] == HidF8) TmpBuf[4] = 0xCD; //Pause/Play
			else if (TmpBuf[4] == HidF9) TmpBuf[4] = 0xB5; //Forward
			else if (TmpBuf[4] == HidF10) TmpBuf[4] = 0x7F; //Mute
			else if (TmpBuf[4] == HidF11) TmpBuf[4] = 0x81; //Quieter
			else if (TmpBuf[4] == HidF12) TmpBuf[4] = 0x80; //Louder
			//else if (TmpBuf[4] == HidKeyP) TmpBuf[4] = HidPrtScr;
			//else if (TmpBuf[4] == HidKeyB) TmpBuf[4] = HidPauseBreak;
			//else if (TmpBuf[4] == HidKeyS) TmpBuf[4] = HidScrLck;
			else if (TmpBuf[2] & HidLCtrlMask) //Map Fn+LCtrl to RCtrl
			{
				TmpBuf[2] &= ~HidLCtrlMask; //Clear LCtrl
				TmpBuf[2] |= HidRCtrlMask; //Set RCtrl
			}
			else
				RtlZeroMemory(TmpBuf + 2, pBrb->BrbL2caAclTransfer.BufferSize - 2); //Ignore all other Fn+[key] combinations
		}
	}

	pBrb->BrbL2caAclTransfer.BufferSize = 10;
	RtlZeroMemory(buf, 10);
	
	buf[0] = 0xa1;
	buf[1] = 0x1;
	buf[2] = TmpBuf[2] | SpecialKeyModifier;
	buf[3] = TmpBuf[3];

	int k = 4;
	while (k < 10 && TmpBuf[k] != 0)
	{
		buf[k] = TmpBuf[k];
		k++;
	}
	if (k < 10)
		buf[k] = SpecialKey;

	DebugPrintBuffer("ProcessA1314Block(): => ", buf, pBrb->BrbL2caAclTransfer.BufferSize);
}
