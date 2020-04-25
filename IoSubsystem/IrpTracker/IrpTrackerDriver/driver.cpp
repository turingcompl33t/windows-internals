// support_driver.cpp
// 
// Support driver entry point.

#include <ntddk.h>

#include "synchronization.hpp"

constexpr auto const DEVICE_NAME = L"\\Device\\IrpTracker";
constexpr auto const SYMLINK_NAME = L"\\??\\IrpTracker";

constexpr auto const IRP_TRACKER_ALLOC_TAG = 0x11223344u;

constexpr auto const IRP_TRACKER_DEVICE = 0x8000;
constexpr auto const IRP_TRACKER_IOCTL_ADD_PENDING
= CTL_CODE(IRP_TRACKER_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS);
constexpr auto const IRP_TRACKER_IOCTL_FLUSH_PENDING
= CTL_CODE(IRP_TRACKER_DEVICE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS);

struct State
{
	LIST_ENTRY queue_head;
	FastMutex  guard;
};

struct QueueItem
{
	LIST_ENTRY list_entry;
	PIRP       irp;
};

// yes, this should be in the device extension rather
// than hanging out here in the global scope...
State g_state;

NTSTATUS DispatchDeviceIoControl(PDEVICE_OBJECT, PIRP irp)
{
	auto status = STATUS_SUCCESS;
	auto io_stack = ::IoGetCurrentIrpStackLocation(irp);

	switch (io_stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case IRP_TRACKER_IOCTL_ADD_PENDING:
	{
		auto item = static_cast<QueueItem*>(
			::ExAllocatePoolWithTag(PagedPool, sizeof(QueueItem), IRP_TRACKER_ALLOC_TAG));
		if (nullptr == item)
		{
			KdPrint(("ExAllocatePoolWithTag() failed"));
			status = STATUS_INSUFFICIENT_RESOURCES;

			irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
			irp->IoStatus.Information = 0;

			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		item->irp = irp;

		{
			auto locker = AutoLock<FastMutex>{ g_state.guard };
			::InsertTailList(&g_state.queue_head, &item->list_entry);
		}

		::IoMarkIrpPending(irp);

		status = STATUS_PENDING;
		break;
	}
	case IRP_TRACKER_IOCTL_FLUSH_PENDING:
	{
		{
			auto locker = AutoLock<FastMutex>{ g_state.guard };
			while (!::IsListEmpty(&g_state.queue_head))
			{
				auto list_entry = ::RemoveHeadList(&g_state.queue_head);
				auto entry = CONTAINING_RECORD(list_entry, QueueItem, list_entry);

				// complete the pended IRP
				auto* pending_irp = entry->irp;
				pending_irp->IoStatus.Status = STATUS_SUCCESS;
				pending_irp->IoStatus.Information = 0;
				IoCompleteRequest(pending_irp, IO_NO_INCREMENT);

				::ExFreePoolWithTag(entry, IRP_TRACKER_ALLOC_TAG);
			}
		}

		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		status = STATUS_SUCCESS;
		break;
	}
	default:
	{
		irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);

		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	}

	return status;
}

NTSTATUS DispatchCreate(PDEVICE_OBJECT, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	::IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

NTSTATUS DispatchClose(PDEVICE_OBJECT, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	::IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT driver_object)
{
	UNICODE_STRING symlink_name = RTL_CONSTANT_STRING(SYMLINK_NAME);

	::IoDeleteSymbolicLink(&symlink_name);
	::IoDeleteDevice(driver_object->DeviceObject);
}

extern "C"
NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ PUNICODE_STRING /* registry_path */
)
{
	::InitializeListHead(&g_state.queue_head);
	g_state.guard.init();

	auto device_name = UNICODE_STRING{};
	auto symlink_name = UNICODE_STRING{};
	::RtlInitUnicodeString(&device_name, DEVICE_NAME);
	::RtlInitUnicodeString(&symlink_name, SYMLINK_NAME);

	auto device_object = PDEVICE_OBJECT{};

	auto status = ::IoCreateDevice(
		driver_object,
		0,
		&device_name,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&device_object);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateDevice() failed\n"));
		return status;
	}

	status = ::IoCreateSymbolicLink(&symlink_name, &device_name);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("IoCreateSymbolicLink() failed\n"));
		::IoDeleteDevice(device_object);
		return status;
	}

	driver_object->DriverUnload = DriverUnload;

	driver_object->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceIoControl;

	return STATUS_SUCCESS;
}