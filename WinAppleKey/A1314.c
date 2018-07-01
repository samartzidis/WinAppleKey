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
			switch (TmpBuf[4])
			{
				case HidLeft: TmpBuf[4] = HidHome; break;
				case HidRight: TmpBuf[4] = HidEnd; break;
				case HidUp: TmpBuf[4] = HidPgUp; break;
				case HidDown: TmpBuf[4] = HidPgDown; break;
				case HidEnter: TmpBuf[4] = HidInsert; break;
				case HidF1: TmpBuf[4] = HidF13; break;
				case HidF2: TmpBuf[4] = HidF14; break;
				case HidF3: TmpBuf[4] = HidF15; break;
				case HidF4: TmpBuf[4] = HidF16; break;
				case HidF5: TmpBuf[4] = HidF17; break;
				case HidF6: TmpBuf[4] = HidF18; break;
				case HidF7: TmpBuf[4] = HidF19; break;
				case HidF8: TmpBuf[4] = HidF20; break;
				case HidF9: TmpBuf[4] = HidF21; break;
				case HidF10: TmpBuf[4] = HidF22; break;
				case HidF11: TmpBuf[4] = HidF23; break;
				case HidF12: TmpBuf[4] = HidF24; break;
				case HidKeyP: TmpBuf[4] = HidPrtScr; break;
				case HidKeyB: TmpBuf[4] = HidPauseBreak; break;
				case HidKeyS: TmpBuf[4] = HidScrLck; break;
				default:
					if (TmpBuf[2] & HidLCtrlMask) //Map Fn+LCtrl to RCtrl
					{
						TmpBuf[2] &= ~HidLCtrlMask; //Clear LCtrl
						TmpBuf[2] |= HidRCtrlMask; //Set RCtrl
					}
					else
						RtlZeroMemory(TmpBuf + 2, pBrb->BrbL2caAclTransfer.BufferSize - 2); //Ignore all other Fn+[key] combinations
					break;
			}
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
