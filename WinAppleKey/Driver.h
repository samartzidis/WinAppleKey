#pragma once

#include <ntddk.h>
#include <wdm.h>
#include <initguid.h> 
#include <ntstrsafe.h>
#include <bthdef.h>
#include <ntintsafe.h>
#include <bthguid.h>
#include <bthioctl.h>
#include <sdpnode.h>
#include <bthddi.h>
#include <bthsdpddi.h>
#include <bthsdpdef.h>

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// Globals

	enum HidCodes
	{
		HidDel = 0x4c,
		HidLeft = 0x50,
		HidHome = 0x4a,
		HidRight = 0x4f,
		HidEnd = 0x4d,
		HidUp = 0x52,
		HidPgUp = 0x4b,
		HidDown = 0x51,
		HidPgDown = 0x4e,
		HidEnter = 0x28,
		HidInsert = 0x49,
		HidF5 = 0x3e,
		HidF6 = 0x3f,
		HidPause = 0x48,
		HidPrtScr = 0x46,
		HidLCtrlMask = 0x1,
		HidRCtrlMask = 0x10
	};

	// Device extension structure
	typedef struct tagDEVICE_EXTENSION 
	{
		PDEVICE_OBJECT DeviceObject; // device object this extension belongs to
		PDEVICE_OBJECT LowerDeviceObject; // next lower driver in same stack
		PDEVICE_OBJECT Pdo; // the PDO
		IO_REMOVE_LOCK RemoveLock;
	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

	///////////////////////////////////////////////////////////////////////////////
	// Global functions

	NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
	void DriverUnload(IN PDRIVER_OBJECT fido);
	void RemoveDevice(IN PDEVICE_OBJECT fdo);
	NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
	NTSTATUS AddDevice(IN PDRIVER_OBJECT driver, IN PDEVICE_OBJECT pdo);
	NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP irp);
	NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP irp);
	NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP irp);
	ULONG GetLowerDeviceType(PDEVICE_OBJECT pdo);
	NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP irp, PDEVICE_EXTENSION pdx);
	NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP irp, PDEVICE_EXTENSION pdx);
	NTSTATUS InternalIoctlComplete(IN PDEVICE_OBJECT fido, IN PIRP irp, IN PVOID context);
	NTSTATUS DispatchInternalIoctl(IN PDEVICE_OBJECT fido, IN PIRP irp);
	void ProcessWirelessKbBlock(PBRB pbrb);
	void ProcessMagicKbBlock(PBRB pbrb);
	void KdPrintBuffer(PCHAR text, PUCHAR buffer, ULONG length);


#ifdef __cplusplus
}
#endif