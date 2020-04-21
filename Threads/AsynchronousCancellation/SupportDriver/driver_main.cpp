// driver_main.cpp
//
// Support driver entry point.

#include <ntddk.h>

constexpr const auto FILE_DEVICE_CANCELDRV = 0x00008005;
constexpr const auto IOCTL_CANCELDRV_SET_ALERTABLE
    = CTL_CODE(FILE_DEVICE_CANCELDRV, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS);

constexpr const auto CANCELDRV_DEVICE_NAME     = L"\\Device\\Canceldrv";
constexpr const auto CANCELDRV_DOS_DEVICE_NAME = L"\\DosDevices\\CANCELDRV";

#ifdef DBG
#define CancelDrvKdPrint(_x_) \
                DbgPrint("CancelDrv.sys: ");\
                DbgPrint _x_;
#else
#define CancelDrvKdPrint(_x_)
#endif

// from ReactOS source
typedef VOID
(NTAPI* PKNORMAL_ROUTINE)(
    IN PVOID NormalContext OPTIONAL,
    IN PVOID SystemArgument1 OPTIONAL,
    IN PVOID SystemArgument2 OPTIONAL);

// from ReactOS source
typedef VOID
(NTAPI* PKKERNEL_ROUTINE)(
    IN KAPC* Apc,
    IN OUT PKNORMAL_ROUTINE* NormalRoutine OPTIONAL,
    IN OUT PVOID* NormalContext OPTIONAL,
    IN OUT PVOID* SystemArgument1 OPTIONAL,
    IN OUT PVOID* SystemArgument2 OPTIONAL);

// from ReactOS source
typedef VOID
(NTAPI* PKRUNDOWN_ROUTINE)(
    IN KAPC* Apc);

extern "C" 
void KeInitializeApc(
    PKAPC             Apc,
    PKTHREAD          Thread,
    CCHAR             ApcStateIndex,
    PKKERNEL_ROUTINE  KernelRoutine,
    PKRUNDOWN_ROUTINE RundownRoutine,
    PKNORMAL_ROUTINE  NormalRoutine,
    KPROCESSOR_MODE   ApcMode,
    PVOID             NormalContext
    );

extern "C" 
void KeInsertQueueApc(
    PKAPC Apc,
    PVOID SystemArgument1,
    PVOID SystemArgument2,
    UCHAR unknown
    );

extern "C" 
void KernelApcCallBack(
    PKAPC apc, 
    PKNORMAL_ROUTINE, 
    PVOID, 
    PVOID, 
    PVOID
    )
{
    auto timeout = LARGE_INTEGER{};

    CancelDrvKdPrint(("Freeing APC Object\n"));

    ExFreePool(apc);

    timeout.QuadPart = 0;
    ::KeDelayExecutionThread(UserMode, TRUE, &timeout);

    return;
}

extern "C" 
void UserApcCallBack(PVOID, PVOID, PVOID)
{
    return;
}

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT, IN PUNICODE_STRING);

NTSTATUS CancelDrvDispatch(IN PDEVICE_OBJECT, IN PIRP);
void CancelDrvUnload(IN PDRIVER_OBJECT);
NTSTATUS CancelDrvQueueApc(PETHREAD);

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT driver_object, IN PUNICODE_STRING)
{
    PDEVICE_OBJECT device_object = NULL;
    
    auto device_name = UNICODE_STRING{};
    auto device_link = UNICODE_STRING{};

    CancelDrvKdPrint(("DriverEntry\n"));

    ::RtlInitUnicodeString(&device_name, CANCELDRV_DEVICE_NAME);

    auto status = ::IoCreateDevice(
        driver_object, 
        0, 
        &device_name,
        FILE_DEVICE_CANCELDRV, 
        0, 
        TRUE, 
        &device_object);

    if (!NT_SUCCESS(status))
    {
        CancelDrvKdPrint(("IoCreateDevice failed:%x\n", status));
        return status;
    }

    driver_object->MajorFunction[IRP_MJ_CREATE]         = CancelDrvDispatch;
    driver_object->MajorFunction[IRP_MJ_CLOSE]          = CancelDrvDispatch;
    driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CancelDrvDispatch;

    driver_object->DriverUnload = CancelDrvUnload;

    ::RtlInitUnicodeString(&device_link, CANCELDRV_DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink(&device_link, &device_name);
    if (!NT_SUCCESS(status))
    {
        CancelDrvKdPrint(("IoCreateSymbolicLink failed\n"));
        ::IoDeleteDevice(device_object);
    }

    return status;
}

NTSTATUS CancelDrvDispatch(IN PDEVICE_OBJECT, IN PIRP irp)
{
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    auto stack = ::IoGetCurrentIrpStackLocation(irp);

    auto io_buffer = irp->AssociatedIrp.SystemBuffer;
    auto const input_buffer_length = stack->Parameters.DeviceIoControl.InputBufferLength;

    switch (stack->MajorFunction)
    {
    case IRP_MJ_CREATE:
        CancelDrvKdPrint(("IRP_MJ_CREATE\n"));
        break;

    case IRP_MJ_CLOSE:
        CancelDrvKdPrint(("IRP_MJ_CLOSE\n"));
        break;

    case IRP_MJ_DEVICE_CONTROL:
        auto control_code = stack->Parameters.DeviceIoControl.IoControlCode;

        switch (control_code)
        {
        case IOCTL_CANCELDRV_SET_ALERTABLE:
            if (input_buffer_length >= sizeof(PVOID))
            {
                auto thread = PETHREAD{};
                auto ph = (PHANDLE)io_buffer;
                irp->IoStatus.Status = ::ObReferenceObjectByHandle(
                    *((PHANDLE)ph), 
                    THREAD_ALL_ACCESS, 
                    NULL, 
                    UserMode, 
                    (PVOID*)&thread, 
                    NULL);

                if (NT_ERROR(irp->IoStatus.Status))
                {
                    CancelDrvKdPrint(("ObReferenceObjectByHandle Failed (%ld)\n", irp->IoStatus.Status));
                }
                else
                {
                    CancelDrvKdPrint(("thread = 0x%lx\n", thread));

                    irp->IoStatus.Status = CancelDrvQueueApc(thread);
                    ObDereferenceObject((PVOID)thread);
                }
            }
            else
            {
                irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                CancelDrvKdPrint(("Invalid parameter passed!\n"));
            }
            break;

        default:
            CancelDrvKdPrint(("Unknown IRP_MJ_DEVICE_CONTROL\n"));
            irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
            break;

        }

        break;
    }

    auto const status = irp->IoStatus.Status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return status;
}

void CancelDrvUnload(IN PDRIVER_OBJECT driver_object)
{
    auto device_link = UNICODE_STRING{};
    ::RtlInitUnicodeString(&device_link, CANCELDRV_DOS_DEVICE_NAME);
    
    ::IoDeleteSymbolicLink(&device_link);
    ::IoDeleteDevice(driver_object->DeviceObject);

    CancelDrvKdPrint(("Driver unloaded\n"));
}


NTSTATUS CancelDrvQueueApc(PETHREAD thread)
{
    auto apc = (PKAPC)::ExAllocatePool(NonPagedPool, sizeof(KAPC));
    if (apc == NULL)
    {
        CancelDrvKdPrint(("ExAllocatePool returned NULL\n"));
        return STATUS_NO_MEMORY;
    }

    ::KeInitializeApc(
        apc,
        (PKTHREAD)thread,
        0,
        (PKKERNEL_ROUTINE)&KernelApcCallBack,
        0,
        (PKNORMAL_ROUTINE)&UserApcCallBack,
        KernelMode,
        NULL);

    ::KeInsertQueueApc(apc, NULL, NULL, 0);

    return STATUS_SUCCESS;
}