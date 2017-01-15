#include "driver.h"

static BOOLEAN FakeFnActive = 0;
static UCHAR WirelessKbBuffer[10] = { 0 };
static UCHAR SpecialKeyP4 = 0;
static UCHAR SpecialKeyP2 = 0;

void ProcessWirelessKbBlock(PBRB pbrb)
{
	PUCHAR buf = (PUCHAR)pbrb->BrbL2caAclTransfer.Buffer;
	KdPrintBuffer("ProcessWirelessKbBlock(): In:  ", buf, pbrb->BrbL2caAclTransfer.BufferSize);

	if (pbrb->BrbL2caAclTransfer.BufferSize == 3) //WirelessKeyboard special key
	{
		SpecialKeyP2 = 0;
		SpecialKeyP4 = 0;

		if (buf[2] & 0x8) //Eject
			SpecialKeyP4 = HidDel; //Del
		if (buf[2] & 0x10) //Fn
			SpecialKeyP2 |= HidLCtrlMask; //Set LCtrl
	}
	else if (pbrb->BrbL2caAclTransfer.BufferSize == 10) //WirelessKeyboard normal key
	{
		RtlCopyMemory(WirelessKbBuffer, buf, 10);

		//Process LCtrl and translate to FakeFn key
		if ((buf[2] & HidLCtrlMask) && !buf[4])
			FakeFnActive = TRUE;
		else if (!buf[4])
			FakeFnActive = FALSE;
		WirelessKbBuffer[2] &= ~HidLCtrlMask; //Clear LCtrl

		//Process Fn+[any key] combination 
		if (FakeFnActive && (buf[4] || buf[2]))
		{
			if (buf[4] == HidLeft) WirelessKbBuffer[4] = HidHome;
			else if (buf[4] == HidRight) WirelessKbBuffer[4] = HidEnd;
			else if (buf[4] == HidUp) WirelessKbBuffer[4] = HidPgUp;
			else if (buf[4] == HidDown) WirelessKbBuffer[4] = HidPgDown;
			else if (buf[4] == HidEnter) WirelessKbBuffer[4] = HidInsert;			
			else if (buf[4] == HidF1) WirelessKbBuffer[4] = HidF13;
			else if (buf[4] == HidF2) WirelessKbBuffer[4] = HidF14;
			else if (buf[4] == HidF3) WirelessKbBuffer[4] = HidF15;
			else if (buf[4] == HidF4) WirelessKbBuffer[4] = HidF16;
			else if (buf[4] == HidF5) WirelessKbBuffer[4] = HidF17;
			else if (buf[4] == HidF6) WirelessKbBuffer[4] = HidF18;
			else if (buf[4] == HidF7) WirelessKbBuffer[4] = HidF19;
			else if (buf[4] == HidF8) WirelessKbBuffer[4] = HidF20;
			else if (buf[4] == HidF9) WirelessKbBuffer[4] = HidF21;
			else if (buf[4] == HidF10) WirelessKbBuffer[4] = HidF22;
			else if (buf[4] == HidF11) WirelessKbBuffer[4] = HidF23;
			else if (buf[4] == HidF12) WirelessKbBuffer[4] = HidF24;
			else if (buf[4] == HidKeyP) WirelessKbBuffer[4] = HidPrtScr;
			else if (buf[4] == HidKeyB) WirelessKbBuffer[4] = HidPauseBreak;
			else if (buf[4] == HidKeyS) WirelessKbBuffer[4] = HidScrLck;
			else if (buf[2] & HidLCtrlMask)
			{
				buf[2] &= ~HidLCtrlMask; //Clear LCtrl
				buf[2] |= HidRCtrlMask; //Set RCtrl
			}
			else
				RtlZeroMemory(buf + 2, pbrb->BrbL2caAclTransfer.BufferSize - 2);
		}
	}

	pbrb->BrbL2caAclTransfer.BufferSize = 10;
	RtlZeroMemory(buf, 10);
	buf[0] = 0xa1;
	buf[1] = 0x1;
	buf[2] = WirelessKbBuffer[2] | SpecialKeyP2;

	int k = 4;
	for (; k < 10; k++)
	{
		if (WirelessKbBuffer[k] != 0)
			buf[k] = WirelessKbBuffer[k];
		else
			break;
	}
	if (k < 10)
		buf[k] = SpecialKeyP4;

	KdPrintBuffer("ProcessWirelessKbBlock(): Out: ", buf, pbrb->BrbL2caAclTransfer.BufferSize);
}
