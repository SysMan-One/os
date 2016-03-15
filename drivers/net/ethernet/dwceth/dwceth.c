/*++

Copyright (c) 2014 Minoca Corp. All Rights Reserved

Module Name:

    dwceth.c

Abstract:

    This module implements the DesignWare Ethernet controller.

Author:

    Evan Green 5-Dec-2014

Environment:

    Kernel

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <minoca/kernel/driver.h>
#include <minoca/net/netdrv.h>
#include "dwceth.h"

//
// ---------------------------------------------------------------- Definitions
//

//
// ------------------------------------------------------ Data Type Definitions
//

//
// ----------------------------------------------- Internal Function Prototypes
//

KSTATUS
DweAddDevice (
    PVOID Driver,
    PSTR DeviceId,
    PSTR ClassId,
    PSTR CompatibleIds,
    PVOID DeviceToken
    );

VOID
DweDispatchStateChange (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
DweDispatchOpen (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
DweDispatchClose (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
DweDispatchIo (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
DweDispatchSystemControl (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    );

VOID
DweDestroyLink (
    PVOID DeviceContext
    );

KSTATUS
DwepProcessResourceRequirements (
    PIRP Irp
    );

KSTATUS
DwepStartDevice (
    PIRP Irp,
    PDWE_DEVICE Device
    );

//
// -------------------------------------------------------------------- Globals
//

PDRIVER DweDriver = NULL;

//
// ------------------------------------------------------------------ Functions
//

KSTATUS
DriverEntry (
    PDRIVER Driver
    )

/*++

Routine Description:

    This routine is the entry point for the e100 driver. It registers its other
    dispatch functions, and performs driver-wide initialization.

Arguments:

    Driver - Supplies a pointer to the driver object.

Return Value:

    STATUS_SUCCESS on success.

    Failure code on error.

--*/

{

    DRIVER_FUNCTION_TABLE FunctionTable;
    KSTATUS Status;

    DweDriver = Driver;
    RtlZeroMemory(&FunctionTable, sizeof(DRIVER_FUNCTION_TABLE));
    FunctionTable.Version = DRIVER_FUNCTION_TABLE_VERSION;
    FunctionTable.AddDevice = DweAddDevice;
    FunctionTable.DispatchStateChange = DweDispatchStateChange;
    FunctionTable.DispatchOpen = DweDispatchOpen;
    FunctionTable.DispatchClose = DweDispatchClose;
    FunctionTable.DispatchIo = DweDispatchIo;
    FunctionTable.DispatchSystemControl = DweDispatchSystemControl;
    Status = IoRegisterDriverFunctions(Driver, &FunctionTable);
    return Status;
}

KSTATUS
DweAddDevice (
    PVOID Driver,
    PSTR DeviceId,
    PSTR ClassId,
    PSTR CompatibleIds,
    PVOID DeviceToken
    )

/*++

Routine Description:

    This routine is called when a device is detected for which the e100 driver
    acts as the function driver. The driver will attach itself to the stack.

Arguments:

    Driver - Supplies a pointer to the driver being called.

    DeviceId - Supplies a pointer to a string with the device ID.

    ClassId - Supplies a pointer to a string containing the device's class ID.

    CompatibleIds - Supplies a pointer to a string containing device IDs
        that would be compatible with this device.

    DeviceToken - Supplies an opaque token that the driver can use to identify
        the device in the system. This token should be used when attaching to
        the stack.

Return Value:

    STATUS_SUCCESS on success.

    Failure code if the driver was unsuccessful in attaching itself.

--*/

{

    PDWE_DEVICE Device;
    KSTATUS Status;

    Device = MmAllocateNonPagedPool(sizeof(DWE_DEVICE), DWE_ALLOCATION_TAG);
    if (Device == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto AddDeviceEnd;
    }

    RtlZeroMemory(Device, sizeof(DWE_DEVICE));
    Device->InterruptHandle = INVALID_HANDLE;
    Device->OsDevice = DeviceToken;
    Status = IoAttachDriverToDevice(Driver, DeviceToken, Device);
    if (!KSUCCESS(Status)) {
        goto AddDeviceEnd;
    }

AddDeviceEnd:
    if (!KSUCCESS(Status)) {
        if (Device != NULL) {
            MmFreeNonPagedPool(Device);
            Device = NULL;
        }
    }

    return Status;
}

VOID
DweDispatchStateChange (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles State Change IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    KSTATUS Status;

    ASSERT(Irp->MajorCode == IrpMajorStateChange);

    if (Irp->Direction == IrpUp) {
        switch (Irp->MinorCode) {
        case IrpMinorQueryResources:
            Status = DwepProcessResourceRequirements(Irp);
            if (!KSUCCESS(Status)) {
                IoCompleteIrp(DweDriver, Irp, Status);
            }

            break;

        case IrpMinorStartDevice:
            Status = DwepStartDevice(Irp, DeviceContext);
            if (!KSUCCESS(Status)) {
                IoCompleteIrp(DweDriver, Irp, Status);
            }

            break;

        default:
            break;
        }
    }

    return;
}

VOID
DweDispatchOpen (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles Open IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    return;
}

VOID
DweDispatchClose (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles Close IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    return;
}

VOID
DweDispatchIo (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles I/O IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    return;
}

VOID
DweDispatchSystemControl (
    PIRP Irp,
    PVOID DeviceContext,
    PVOID IrpContext
    )

/*++

Routine Description:

    This routine handles System Control IRPs.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

    DeviceContext - Supplies the context pointer supplied by the driver when it
        attached itself to the driver stack. Presumably this pointer contains
        driver-specific device context.

    IrpContext - Supplies the context pointer supplied by the driver when
        the IRP was created.

Return Value:

    None.

--*/

{

    PDWE_DEVICE Device;
    PSYSTEM_CONTROL_DEVICE_INFORMATION DeviceInformationRequest;
    KSTATUS Status;

    ASSERT(Irp->MajorCode == IrpMajorSystemControl);

    Device = DeviceContext;
    if (Irp->Direction == IrpDown) {
        switch (Irp->MinorCode) {
        case IrpMinorSystemControlDeviceInformation:
            DeviceInformationRequest = Irp->U.SystemControl.SystemContext;
            Status = NetGetSetLinkDeviceInformation(
                                         Device->NetworkLink,
                                         &(DeviceInformationRequest->Uuid),
                                         DeviceInformationRequest->Data,
                                         &(DeviceInformationRequest->DataSize),
                                         DeviceInformationRequest->Set);

            IoCompleteIrp(DweDriver, Irp, Status);
            break;

        default:
            break;
        }
    }

    return;
}

KSTATUS
DwepAddNetworkDevice (
    PDWE_DEVICE Device
    )

/*++

Routine Description:

    This routine adds the device to core networking's available links.

Arguments:

    Device - Supplies a pointer to the device to add.

Return Value:

    Status code.

--*/

{

    PNET_PACKET_SIZE_INFORMATION PacketSizeInformation;
    NET_LINK_PROPERTIES Properties;
    KSTATUS Status;

    if (Device->NetworkLink != NULL) {
        Status = STATUS_SUCCESS;
        goto AddNetworkDeviceEnd;
    }

    //
    // Add a link to the core networking library.
    //

    RtlZeroMemory(&Properties, sizeof(NET_LINK_PROPERTIES));
    Properties.Version = NET_LINK_PROPERTIES_VERSION;
    Properties.TransmitAlignment = 1;
    Properties.Device = Device->OsDevice;
    Properties.DeviceContext = Device;
    PacketSizeInformation = &(Properties.PacketSizeInformation);
    PacketSizeInformation->MaxPacketSize = DWE_RECEIVE_FRAME_DATA_SIZE;
    Properties.DataLinkType = NetDataLinkEthernet;
    Properties.MaxPhysicalAddress = MAX_ULONG;
    Properties.PhysicalAddress.Network = SocketNetworkPhysicalEthernet;

    ASSERT(Device->MacAddressAssigned != FALSE);

    RtlCopyMemory(&(Properties.PhysicalAddress.Address),
                  &(Device->MacAddress),
                  sizeof(Device->MacAddress));

    Properties.Interface.Send = DweSend;
    Properties.Interface.GetSetInformation = DweGetSetInformation;
    Properties.Interface.DestroyLink = DweDestroyLink;
    Properties.ChecksumFlags = NET_LINK_CHECKSUM_FLAG_TRANSMIT_IP_OFFLOAD |
                               NET_LINK_CHECKSUM_FLAG_TRANSMIT_UDP_OFFLOAD |
                               NET_LINK_CHECKSUM_FLAG_TRANSMIT_TCP_OFFLOAD |
                               NET_LINK_CHECKSUM_FLAG_RECEIVE_IP_OFFLOAD |
                               NET_LINK_CHECKSUM_FLAG_RECEIVE_TCP_OFFLOAD |
                               NET_LINK_CHECKSUM_FLAG_RECEIVE_UDP_OFFLOAD;

    Device->ChecksumFlags = Properties.ChecksumFlags;
    Status = NetAddLink(&Properties, &(Device->NetworkLink));
    if (!KSUCCESS(Status)) {
        goto AddNetworkDeviceEnd;
    }

AddNetworkDeviceEnd:
    if (!KSUCCESS(Status)) {
        if (Device->NetworkLink != NULL) {
            NetRemoveLink(Device->NetworkLink);
            Device->NetworkLink = NULL;
        }
    }

    return Status;
}

VOID
DweDestroyLink (
    PVOID DeviceContext
    )

/*++

Routine Description:

    This routine notifies the device layer that the networking core is in the
    process of destroying the link and will no longer call into the device for
    this link. This allows the device layer to release any context that was
    supporting the device link interface.

Arguments:

    DeviceContext - Supplies a pointer to the device context associated with
        the link being destroyed.

Return Value:

    None.

--*/

{

    return;
}

//
// --------------------------------------------------------- Internal Functions
//

KSTATUS
DwepProcessResourceRequirements (
    PIRP Irp
    )

/*++

Routine Description:

    This routine filters through the resource requirements presented by the
    bus for an e100 LAN controller. It adds an interrupt vector requirement for
    any interrupt line requested.

Arguments:

    Irp - Supplies a pointer to the I/O request packet.

Return Value:

    Status code.

--*/

{

    PRESOURCE_CONFIGURATION_LIST Requirements;
    KSTATUS Status;
    RESOURCE_REQUIREMENT VectorRequirement;

    ASSERT((Irp->MajorCode == IrpMajorStateChange) &&
           (Irp->MinorCode == IrpMinorQueryResources));

    //
    // Initialize a nice interrupt vector requirement in preparation.
    //

    RtlZeroMemory(&VectorRequirement, sizeof(RESOURCE_REQUIREMENT));
    VectorRequirement.Type = ResourceTypeInterruptVector;
    VectorRequirement.Minimum = 0;
    VectorRequirement.Maximum = -1;
    VectorRequirement.Length = 1;

    //
    // Loop through all configuration lists, creating a vector for each line.
    //

    Requirements = Irp->U.QueryResources.ResourceRequirements;
    Status = IoCreateAndAddInterruptVectorsForLines(Requirements,
                                                    &VectorRequirement);

    if (!KSUCCESS(Status)) {
        goto ProcessResourceRequirementsEnd;
    }

ProcessResourceRequirementsEnd:
    return Status;
}

KSTATUS
DwepStartDevice (
    PIRP Irp,
    PDWE_DEVICE Device
    )

/*++

Routine Description:

    This routine starts the DesignWare Ethernet device.

Arguments:

    Irp - Supplies a pointer to the start IRP.

    Device - Supplies a pointer to the device information.

Return Value:

    Status code.

--*/

{

    ULONG AlignmentOffset;
    PRESOURCE_ALLOCATION Allocation;
    PRESOURCE_ALLOCATION_LIST AllocationList;
    IO_CONNECT_INTERRUPT_PARAMETERS Connect;
    PRESOURCE_ALLOCATION ControllerBase;
    PHYSICAL_ADDRESS EndAddress;
    PRESOURCE_ALLOCATION LineAllocation;
    ULONG PageSize;
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Size;
    KSTATUS Status;

    ControllerBase = NULL;

    //
    // Loop through the allocated resources to get the controller base and the
    // interrupt.
    //

    AllocationList = Irp->U.StartDevice.ProcessorLocalResources;
    Allocation = IoGetNextResourceAllocation(AllocationList, NULL);
    while (Allocation != NULL) {

        //
        // If the resource is an interrupt vector, then it should have an
        // owning interrupt line allocation.
        //

        if (Allocation->Type == ResourceTypeInterruptVector) {

            //
            // Currently only one interrupt resource is expected.
            //

            ASSERT(Device->InterruptResourcesFound == FALSE);
            ASSERT(Allocation->OwningAllocation != NULL);

            //
            // Save the line and vector number.
            //

            LineAllocation = Allocation->OwningAllocation;
            Device->InterruptLine = LineAllocation->Allocation;
            Device->InterruptVector = Allocation->Allocation;
            Device->InterruptResourcesFound = TRUE;

        //
        // Look for the first physical address reservation, the registers.
        //

        } else if (Allocation->Type == ResourceTypePhysicalAddressSpace) {
            if (ControllerBase == NULL) {
                ControllerBase = Allocation;
            }
        }

        //
        // Get the next allocation in the list.
        //

        Allocation = IoGetNextResourceAllocation(AllocationList, Allocation);
    }

    //
    // Fail to start if the controller base was not found.
    //

    if (ControllerBase == NULL) {
        Status = STATUS_INVALID_CONFIGURATION;
        goto StartDeviceEnd;
    }

    //
    // Map the controller.
    //

    if (Device->ControllerBase == NULL) {

        //
        // Page align the mapping request.
        //

        PageSize = MmPageSize();
        PhysicalAddress = ControllerBase->Allocation;
        EndAddress = PhysicalAddress + ControllerBase->Length;
        PhysicalAddress = ALIGN_RANGE_DOWN(PhysicalAddress, PageSize);
        AlignmentOffset = ControllerBase->Allocation - PhysicalAddress;
        EndAddress = ALIGN_RANGE_UP(EndAddress, PageSize);
        Size = (ULONG)(EndAddress - PhysicalAddress);
        Device->ControllerBase = MmMapPhysicalAddress(PhysicalAddress,
                                                      Size,
                                                      TRUE,
                                                      FALSE,
                                                      TRUE);

        if (Device->ControllerBase == NULL) {
            Status = STATUS_NO_MEMORY;
            goto StartDeviceEnd;
        }

        Device->ControllerBase += AlignmentOffset;
    }

    ASSERT(Device->ControllerBase != NULL);

    //
    // Allocate the controller structures.
    //

    Status = DwepInitializeDeviceStructures(Device);
    if (!KSUCCESS(Status)) {
        goto StartDeviceEnd;
    }

    //
    // Attempt to connect the interrupt.
    //

    ASSERT(Device->InterruptHandle == INVALID_HANDLE);

    RtlZeroMemory(&Connect, sizeof(IO_CONNECT_INTERRUPT_PARAMETERS));
    Connect.Version = IO_CONNECT_INTERRUPT_PARAMETERS_VERSION;
    Connect.Device = Irp->Device;
    Connect.LineNumber = Device->InterruptLine;
    Connect.Vector = Device->InterruptVector;
    Connect.InterruptServiceRoutine = DwepInterruptService;
    Connect.LowLevelServiceRoutine = DwepInterruptServiceWorker;
    Connect.Context = Device;
    Connect.Interrupt = &(Device->InterruptHandle);
    Status = IoConnectInterrupt(&Connect);
    if (!KSUCCESS(Status)) {
        goto StartDeviceEnd;
    }

    //
    // Start up the controller.
    //

    Status = DwepResetDevice(Device);
    if (!KSUCCESS(Status)) {
        goto StartDeviceEnd;
    }

    ASSERT(Device->NetworkLink != NULL);

StartDeviceEnd:
    return Status;
}

