#include "Driver.h"

#ifdef ALLOC_PRAGMA
	#pragma alloc_text (INIT, DriverEntry)
	#pragma alloc_text (PAGE, DriverUnload)
	#pragma alloc_text (PAGE, AddDevice)
	#pragma alloc_text (PAGE, RemoveDevice)
	#pragma alloc_text (PAGE, GetLowerDeviceType)
#endif // ALLOC_PRAGMA


NTSTATUS DriverEntry(IN PDRIVER_OBJECT driverObject, IN PUNICODE_STRING registryPath)
{
	KdPrint(("DriverEntry(): driverObject = 0x%x\n", driverObject));

	driverObject->DriverUnload = DriverUnload;
	driverObject->DriverExtension->AddDevice = AddDevice;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		driverObject->MajorFunction[i] = DispatchAny;

	driverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	driverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;

	driverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalIoctl;

	return STATUS_SUCCESS;
}


void DriverUnload(IN PDRIVER_OBJECT driverObject)
{
	PAGED_CODE();

	KdPrint(("DriverUnload(): driverObject = 0x%x\n", driverObject));
}


NTSTATUS AddDevice(IN PDRIVER_OBJECT driverObject, IN PDEVICE_OBJECT pdo)
{
	PAGED_CODE();

	KdPrint(("AddDevice(): driverObject: 0x%x, pdo: 0x%x\n", driverObject, pdo));

	//Create the filter functional device object
	PDEVICE_OBJECT fido;
	ULONG type = GetLowerDeviceType(pdo);
	NTSTATUS status = IoCreateDevice(driverObject, sizeof(DEVICE_EXTENSION), NULL, type, 0, FALSE, &fido);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("AddDevice(): IoCreateDevice failed: 0x%x\n", status));
		return status;
	}

	//Add extension data to the fido
	PDEVICE_EXTENSION fidoExt = (PDEVICE_EXTENSION)fido->DeviceExtension;
	do
	{
		IoInitializeRemoveLock(&fidoExt->RemoveLock, 0, 0, 0);
		fidoExt->DeviceObject = fido;
		fidoExt->Pdo = pdo;

		//Add fido to the stack and propagate critical settings from the immediately lower device object
		PDEVICE_OBJECT fdo = IoAttachDeviceToDeviceStack(fido, pdo);
		if (!fdo)
		{
			KdPrint(("AddDevice(): IoAttachDeviceToDeviceStack failed\n"));
			status = STATUS_DEVICE_REMOVED;
			break;
		}
		fidoExt->LowerDeviceObject = fdo;

		//Copy the flags related to I/O buffering from the lower device object so the I/O manager
		//will create the expected data structures for reads and writes.
		fido->Flags |= fdo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);

		fido->Flags &= ~DO_DEVICE_INITIALIZING; // Clear the DO_DEVICE_INITIALIZING flag so that we can get IRPs

	} while (FALSE);

	if (!NT_SUCCESS(status))
	{
		if (fidoExt->LowerDeviceObject)
			IoDetachDevice(fidoExt->LowerDeviceObject);

		IoDeleteDevice(fido);
	}

	return status;
}


void RemoveDevice(IN PDEVICE_OBJECT fido)
{
	PAGED_CODE();

	PDEVICE_EXTENSION fidoExt = (PDEVICE_EXTENSION)fido->DeviceExtension;
	if (fidoExt->LowerDeviceObject)
		IoDetachDevice(fidoExt->LowerDeviceObject);

	IoDeleteDevice(fido);
}


ULONG GetLowerDeviceType(PDEVICE_OBJECT pdo)
{
	PDEVICE_OBJECT lowerPdo = IoGetAttachedDeviceReference(pdo);
	if (!lowerPdo)
		return FILE_DEVICE_UNKNOWN;

	ULONG devtype = lowerPdo->DeviceType;
	ObDereferenceObject(lowerPdo);

	return devtype;
}


NTSTATUS CompleteRequest(IN PIRP irp, IN NTSTATUS status, IN ULONG_PTR info)
{
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = info;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}


NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP irp)
{
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;
	PoStartNextPowerIrp(irp);
	
	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(irp, status, 0);

	IoSkipCurrentIrpStackLocation(irp);
	
	status = PoCallDriver(pdx->LowerDeviceObject, irp);
	
	IoReleaseRemoveLock(&pdx->RemoveLock, irp);

	return status;
}							


NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP irp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);
	ULONG fcn = stack->MinorFunction;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fido->DeviceExtension;
	
	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(irp, status, 0);

	//Handle usage notification specially in order to track power pageable
	//flag correctly. We need to avoid allowing a non-pageable handler to be
	//layered on top of a pageable handler.
	if (fcn == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		if (!fido->AttachedDevice || (fido->AttachedDevice->Flags & DO_POWER_PAGABLE))
			fido->Flags |= DO_POWER_PAGABLE;

		IoCopyCurrentIrpStackLocationToNext(irp);
		IoSetCompletionRoutine(irp, (PIO_COMPLETION_ROUTINE)UsageNotificationCompletionRoutine, (PVOID)pdx, TRUE, TRUE, TRUE);

		return IoCallDriver(pdx->LowerDeviceObject, irp);
	}

	//Handle start device specially in order to correctly inherit FILE_REMOVABLE_MEDIA
	if (fcn == IRP_MN_START_DEVICE)
	{
		IoCopyCurrentIrpStackLocationToNext(irp);

		IoSetCompletionRoutine(irp, (PIO_COMPLETION_ROUTINE)StartDeviceCompletionRoutine, (PVOID)pdx, TRUE, TRUE, TRUE);

		return IoCallDriver(pdx->LowerDeviceObject, irp);
	}

	//Handle remove device specially in order to cleanup device stack
	if (fcn == IRP_MN_REMOVE_DEVICE)
	{
		IoSkipCurrentIrpStackLocation(irp);

		status = IoCallDriver(pdx->LowerDeviceObject, irp);
		
		IoReleaseRemoveLockAndWait(&pdx->RemoveLock, irp);

		RemoveDevice(fido);

		return status;
	}

	//Simply forward any other type of PnP request
	IoSkipCurrentIrpStackLocation(irp);
	status = IoCallDriver(pdx->LowerDeviceObject, irp);

	IoReleaseRemoveLock(&pdx->RemoveLock, irp);

	return status;
}


NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP irp, PDEVICE_EXTENSION pdx)
{
	if (irp->PendingReturned)
		IoMarkIrpPending(irp);

	// Inherit FILE_REMOVABLE_MEDIA flag from lower object.
	if (pdx->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		fido->Characteristics |= FILE_REMOVABLE_MEDIA;

	IoReleaseRemoveLock(&pdx->RemoveLock, irp);
	return STATUS_SUCCESS;
}


NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP irp, PDEVICE_EXTENSION pdx)
{
	if (irp->PendingReturned)
		IoMarkIrpPending(irp);

	// If lower driver cleared pageable flag, we must do the same
	if (!(pdx->LowerDeviceObject->Flags & DO_POWER_PAGABLE))
		fido->Flags &= ~DO_POWER_PAGABLE;

	IoReleaseRemoveLock(&pdx->RemoveLock, irp);

	return STATUS_SUCCESS;
}			


NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP irp)
{
	PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)fido->DeviceExtension;
	PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS status;

	status = IoAcquireRemoveLock(&devExt->RemoveLock, irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(irp, status, 0);

	IoSkipCurrentIrpStackLocation(irp);

	status = IoCallDriver(devExt->LowerDeviceObject, irp);

	IoReleaseRemoveLock(&devExt->RemoveLock, irp);

	return status;
}


void KdPrintBuffer(PCHAR text, PUCHAR buffer, ULONG length)
{
#if DBG
	KdPrint(("%s HEX: ", text));
	for (ULONG i = 0; i < length; ++i)
		KdPrint(("%02X ", buffer[i]));
	KdPrint(("\n"));
#endif
}

NTSTATUS InternalIoctlComplete(IN PDEVICE_OBJECT fido, IN PIRP irp, IN PVOID context)
{
	KdPrint(("ReadComplete()\n"));

	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	if (NT_SUCCESS(irp->IoStatus.Status))
	{
		PBRB pbrb = (PBRB)irpSp->Parameters.Others.Argument1;

		if (pbrb->BrbHeader.Type == BRB_L2CA_ACL_TRANSFER)
		{
			KdPrint(("ReadComplete(): BRB_L2CA_ACL_TRANSFER: BufferMDL = 0x%x, Buffer = 0x%x, BufferSize = %lu, RemainingBufferSize = %lu\n", 
				pbrb->BrbL2caAclTransfer.BufferMDL, pbrb->BrbL2caAclTransfer.Buffer, pbrb->BrbL2caAclTransfer.BufferSize, pbrb->BrbL2caAclTransfer.RemainingBufferSize));

			if (pbrb->BrbL2caAclTransfer.Buffer)
			{
				if (pbrb->BrbL2caAclTransfer.BufferSize == 3 || pbrb->BrbL2caAclTransfer.BufferSize == 10) //WirelessKeyboard
					ProcessWirelessKbBlock(pbrb);
				else if (pbrb->BrbL2caAclTransfer.BufferSize == 11) //MagicKeyboard 
					ProcessMagicKbBlock(pbrb);
			}
		}			
	}

	// Mark the Irp pending if required
	if (irp->PendingReturned)
		IoMarkIrpPending(irp);

	return irp->IoStatus.Status;
}


NTSTATUS DispatchInternalIoctl(IN PDEVICE_OBJECT fido, IN PIRP irp)
{
	PDEVICE_EXTENSION devExt = (PDEVICE_EXTENSION)fido->DeviceExtension;
	PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
	ULONG dwControlCode = irpSp->Parameters.DeviceIoControl.IoControlCode;

	NTSTATUS status = IoAcquireRemoveLock(&devExt->RemoveLock, irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(irp, status, 0);

	if (dwControlCode == IOCTL_INTERNAL_BTH_SUBMIT_BRB)
	{
		KdPrint(("DispatchInternalIoctl(): dwControlCode = IOCTL_INTERNAL_BTH_SUBMIT_BRB\n"));

		PBRB pbrb = (PBRB)irpSp->Parameters.Others.Argument1;
		if (pbrb->BrbHeader.Type == BRB_L2CA_ACL_TRANSFER)
		{
			KdPrint(("DispatchInternalIoctl(): BRB_L2CA_ACL_TRANSFER, BrbL2caAclTransfer.BufferSize = %lu\n", 
				pbrb->BrbL2caAclTransfer.BufferSize));

			//IoSkipCurrentIrpStackLocation(irp);
			IoCopyCurrentIrpStackLocationToNext(irp);
			IoSetCompletionRoutine(irp, InternalIoctlComplete, fido, TRUE, TRUE, TRUE);
		}
		//BRB_GET_DEVICE_INTERFACE_STRING
		//BRB_L2CA_OPEN_CHANNEL
		else
			IoSkipCurrentIrpStackLocation(irp);	
	}
	else
		IoSkipCurrentIrpStackLocation(irp);

	status = IoCallDriver(devExt->LowerDeviceObject, irp);
	
	IoReleaseRemoveLock(&devExt->RemoveLock, irp);

	return status;
}

