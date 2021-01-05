/** @file
  This UEFI application is to check firmware integrity based on URD.

 **/



#include "FirmwareUpdate.h"

//
// MinnowMax Flash Layout
//
// Start (hex)	End (hex)	Length (hex)	Area Name
// -----------	---------	------------	---------
// 00000000	    007FFFFF	00800000	Flash Image
//
// 00000000	    00000FFF	00001000	Descriptor Region
// 00001000	    003FFFFF	003FF000	TXE Region
// 00500000	    007FFFFF	00400000	BIOS Region
//


/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

 **/
EFI_STATUS
    EFIAPI
UefiMain (
        IN EFI_HANDLE        ImageHandle,
        IN EFI_SYSTEM_TABLE  *SystemTable
        )
{
    UINT32      Index;
    UINT32      Index2;
    UINT32      USB_Status = 0;
    EFI_STATUS  Status;

    // Usb Handle variables
    EFI_HANDLE  *UsbHandleBuffer;
    UINTN       UsbHandleCount;
    UINTN       UsbHandleIndex;

    // Hash Handle variables
    EFI_HANDLE  *Hash2ControllerHandle;
    EFI_HANDLE  Hash2SBHandle           = NULL;
    //UINTN       Hash2SBHandleCount;
 
    EFI_HANDLE  Hash2ChildHandle        = NULL;


    // URD Specification
    UINT16      URD_IdVendor = 7531;
    UINT16      URD_IdProduct = 260;
    UINT8       URD_InterfaceClass = 255;
    UINT8       URD_InterfaceSubClass = 0;
    UINT8       URD_InterfaceProtocol = 0;
    UINT8       InEndpointAddr = 0;
    UINT8       OutEndpointAddr = 0;   

    // Shell Data Structure
    EFI_SHELL_PROTOCOL  *EfiShellProtocol;

    // USB Data Structures
    EFI_USB_IO_PROTOCOL             *UsbProtocol;
    EFI_USB_DEVICE_DESCRIPTOR       DeviceDesc;
    //EFI_USB_CONFIG_DESCRIPTOR       ConfigDesc;
    EFI_USB_INTERFACE_DESCRIPTOR    IntfDesc;
    EFI_USB_ENDPOINT_DESCRIPTOR     EndpDesc;

    // Data transfer
    CHAR8 Hello_Data[16] = "1234567890ABCDEF";
    CHAR8 Nonce_Data[33] = { 0, };
    CHAR8 Hash_Data[32] = "1A3E7942FC31AE45679B21DA967CE275";
    UINT8 FW_Data[4096] = { 0, };
    UINT8 Verify_Data[100] = { 0, };
    CHAR8 BD_Data[16] = "FEDCBA0987654321";

    UINTN Hello_len = 16;
    UINTN Nonce_len = 32;
    UINTN Hash_len = 32;
    UINTN FW_len = 4096;
    UINTN Verify_len = 100;
    UINTN BD_len = 16;

    // Firmware read variables


    // Firmware write variables 
    UINTN                 FileSize;
    UINT32                BufferSize;
    UINT8                 *FileBuffer;
    UINT8                 *Buffer;
    EFI_PHYSICAL_ADDRESS  Address;
    UINTN                 CountOfBlocks;
    EFI_TPL               OldTpl;
    BOOLEAN               ResetRequired;
    BOOLEAN               FlashError;

    // Variable Initialization
    Index             = 0;
    Index2            = 0;
    FileSize          = 8388608;
    BufferSize        = 0;
    FileBuffer        = NULL;
    Buffer            = NULL;
    Address           = 0;
    CountOfBlocks     = 0;
    ResetRequired     = FALSE;
    FlashError        = FALSE;

    Status = EFI_SUCCESS;

    // Locate Spi protocol
    Status = gBS->LocateProtocol (
            &gEfiSpiProtocolGuid,
            NULL,
            (VOID **)&mSpiProtocol
            );

    // Locate Shell protocol
    Status = gBS->LocateProtocol (
            &gEfiShellProtocolGuid,
            NULL,
            (VOID **) &EfiShellProtocol
            );

    // Locate Hash Binding and protocol
    
    /*Status = gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiHash2ServiceBindingProtocolGuid,
            NULL,
            &Hash2SBHandleCount,
            &Hash2ControllerHandle
            );

    Print(L"Hash Binding Status: %r", Status);
    Print(L"handle count=%d", Hash2SBHandleCount);
    */ /*
    if (EFI_ERROR (Status))
        return Status;
    Print(L"Connected Hash Binding Buffer: %d\n", HashBindingHandleCount);

    Status = HashBindingHandleBuffer->Hash2ServiceBindingCreateChild ( 
            HashBindingHandleBuffer,
            HashHandleBuffer
            );

    Status = gBS->HandleProtocol (
            HashHandleBuffer,
            &gEfiHash2ProtocolGuid,
            (VOID **) &mHash2Protocol
            );
    Print(L"Hash Protocol Status: %r", Status);
    if (EFI_ERROR (Status))
        return Status;
     */

    //
    // Get the Hash2ServiceBinding Protocol
    //
    /*
       Status = gBS->OpenProtocol (
       Hash2ControllerHandle,
       &gEfiHash2ServiceBindingProtocolGuid,
       (VOID **)&mHash2ServiceBindingProtocol,
       Hash2SBHandle,
       Hash2ControllerHandle,
       EFI_OPEN_PROTOCOL_GET_PROTOCOL
       );
       */
    
    Status = gBS->LocateProtocol (
            &gEfiHash2ServiceBindingProtocolGuid,
            NULL,
            (VOID **)&mHash2ServiceBindingProtocol
            );
    
    Print(L"Hash Service Binding Protocol Status: %r", Status);
    if (EFI_ERROR (Status)) {

        return Status;
    }
    //
    // Initialize a ChildHandle
    //
    Hash2ChildHandle = NULL;

    //
    // Create a ChildHandle with the Hash2 Protocol
    //
    Status = mHash2ServiceBindingProtocol->CreateChild (mHash2ServiceBindingProtocol, &Hash2ChildHandle);
    Print(L"Child Handle by Hash Service Binding Protocol Status: %r", Status);
    if (EFI_ERROR (Status)) {
        return Status;
    }
    
    //
    // Retrieve the Hash2Protocol from Hash2ChildHandle
    //
    Status = gBS->OpenProtocol ( 
            Hash2ChildHandle,
            &gEfiHash2ProtocolGuid,
            (VOID **)&mHash2Protocol,
            Hash2SBHandle,
            Hash2ControllerHandle,
            EFI_OPEN_PROTOCOL_BY_DRIVER
            );

    Print(L"Hash Protocol Status: %r", Status);
    if (EFI_ERROR (Status))
        return Status;



    // Locate USB protocol
    Status = gBS->LocateHandleBuffer (
            ByProtocol,
            &gEfiUsbIoProtocolGuid,
            NULL,
            &UsbHandleCount,
            &UsbHandleBuffer
            );

    if (EFI_ERROR (Status))
        return Status;

    Print(L"Connected USB Device: %d\n", UsbHandleCount);

    for(UsbHandleIndex=0; UsbHandleIndex < UsbHandleCount; UsbHandleIndex++) {
        Status = gBS->HandleProtocol (
                UsbHandleBuffer[UsbHandleIndex],
                &gEfiUsbIoProtocolGuid,
                (VOID **) &UsbProtocol
                );
        //Print(L"Opening Device Handle, Status=%r\n", Status);

        Status = UsbProtocol->UsbGetDeviceDescriptor (
                UsbProtocol,
                &DeviceDesc
                );
        //Print(L"Retrieving Device Descriptor, Status= %r\n", Status);
        Print(L"IdVendor=0x%04x, IdProduct=0x%04x\n", DeviceDesc.IdVendor, DeviceDesc.IdProduct);
        if ((URD_IdVendor == DeviceDesc.IdVendor) && (URD_IdProduct == DeviceDesc.IdProduct)) {
            /*Status = UsbProtocol->UsbGetConfigDescriptor (
              UsbProtocol,
              &ConfigDesc
              );
              Print(L"NumInterfaces= %d\n", ConfigDesc.NumInterface);*/
            Status = UsbProtocol->UsbGetInterfaceDescriptor (
                    UsbProtocol,
                    &IntfDesc
                    );
            if((IntfDesc.InterfaceClass == URD_InterfaceClass) && (IntfDesc.InterfaceSubClass == URD_InterfaceSubClass) && (IntfDesc.InterfaceProtocol == URD_InterfaceProtocol)) {
                Print(L"URD is detected.\n");
                for(Index = 0; Index < IntfDesc.NumEndpoints; Index++) {
                    Status = UsbProtocol->UsbGetEndpointDescriptor (
                            UsbProtocol,
                            Index,
                            &EndpDesc
                            );
                    if(EndpDesc.EndpointAddress > 127)
                        InEndpointAddr = EndpDesc.EndpointAddress;
                    if(EndpDesc.EndpointAddress < 128)
                        OutEndpointAddr = EndpDesc.EndpointAddress;
                }



                // 
                // Send opening Signal
                //

                Status = UsbProtocol->UsbBulkTransfer (
                        UsbProtocol,
                        OutEndpointAddr,
                        Hello_Data,
                        &Hello_len,
                        0,
                        &USB_Status
                        );
                /*
                   Print(L"Send hello signal, Endpoint=0x%02x, Status:%r\n", OutEndpointAddr, Status);
                   Print(L"signal length=%d\n", Hello_len);
                   Print(L"signal=");
                   for(Index=0;Index<Hello_len;Index++) {
                   Print(L"%02x ", Hello_Data[Index]);
                   }
                   Print(L"\n");
                 */

                //
                // Receive Nonce
                //
                while(1) {
                    Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            InEndpointAddr,
                            Nonce_Data,
                            &Nonce_len,
                            0,
                            &USB_Status
                            );
                    if(Nonce_len == 32)
                        break;
                    else
                        Nonce_len = 32;
                }
                Print(L"Receive nonce value, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);
                /*
                   Print(L"Nonce length=%d\n", Nonce_len);
                   Print(L"Nonce=");
                   for(Index=0;Index<Nonce_len;Index++) {
                   Print(L"%02x ", Nonce_Data[Index]);
                   }
                   Print(L"\n");
                 */

                // Hash Computation
                //Index  = 0;
                //Address = PcdGet32 (PcdFlashChipBase);
                //for(Index=0;AddrIdx<(1<<22);AddrIdx+=1<<3) {
                //    SpiFlashRead( );
                //    
                //}

                //
                // Send hash value
                //
                Status = UsbProtocol->UsbBulkTransfer (
                        UsbProtocol,
                        OutEndpointAddr,
                        Hash_Data,
                        &Hash_len,
                        0,
                        &USB_Status
                        );
                Print(L"Send hash value, Endpoint=0x%02x, Status:%r\n", OutEndpointAddr, Status);
                /*
                   Print(L"Hash length=%d\n", Hash_len);
                   Print(L"Hash=");
                   for(Index=0;Index<Hash_len;Index++) {
                   Print(L"%02x ", Hash_Data[Index]);
                   }
                   Print(L"\n");
                   Print(L"Hash value = %a\n", Hash_Data);
                 */

                //
                // Receive Verify data
                //
                while(1) {
                    Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            InEndpointAddr,
                            Verify_Data,
                            &Verify_len,
                            0,
                            &USB_Status
                            );
                    if(Verify_len == 100)
                        break;
                    else
                        Verify_len = 100;
                }
                Print(L"Receive verify data value, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);
                /*
                   Print(L"verify data length=%d\n", Verify_len);
                   Print(L"verify data=");
                   for(Index=0;Index<Verify_len;Index++) {
                   Print(L"%02x", Verify_Data[Index]);
                   }
                   Print(L"\n");
                 */

                //
                // Check Verify data
                //
                if(Verify_Data[15] == '1') {
                    Print(L"this device is secure.\n");
                    Print(L"Firmware integrity validation completed. Boot process will be proceed.\n");
                    goto Done;
                }
                else if(Verify_Data[15] == '2') {
                    Print(L"Device is not secure.");
                    // Return Not OK: Firmware Transmission
                    Index = 0;
                    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
                    Address = PcdGet32 (PcdFlashChipBase);

                    // Erase whole firmware
                    Print(L"Erase SPI flashrom. Processing...");
                    Status = SpiFlashBlockErase ((UINTN) Address, &FileSize);
                    if (EFI_ERROR (Status)) {
                        gBS->RestoreTPL (OldTpl);
                        FlashError = TRUE;
                        goto Done;
                    }

                    // Bulk transfer and write data
                    Print(L"Receive firmware, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);
                    while(Index < 8388608) {
                        Status = UsbProtocol->UsbBulkTransfer (
                                UsbProtocol,
                                InEndpointAddr,
                                FW_Data,
                                &FW_len,
                                0,
                                &USB_Status
                                );
                        //Print(L"%r\n", USB_Status);
                        //Print(L"Firmware length=%d, ", FW_len);
                        //Print(L"Index=%d\n", Index);
                        //Print(L"Firmware contents=%a", FW_Data);
                        //for(Index2=0;Index2<VD_len;Index2++) {
                        //    Print(L"%08X ", FW_Data[Index2]);
                        //}
                        //Print(L"\n");
                        if(FW_len > 1000) {
                            // firmware write
                            Buffer = FW_Data;
                            BufferSize = FW_len;
                            Status = SpiFlashWrite ((UINTN) Address, &BufferSize, Buffer);
                            if (EFI_ERROR (Status)) {
                                gBS->RestoreTPL (OldTpl);
                                FlashError = TRUE;
                                goto Done;
                            }
                            Address += FW_len;
                            // Re-initialization
                            Index += FW_len;
                            ResetRequired = TRUE;
                        }
                        FW_len = 4096;
                        FW_Data[0] = '\0';
                        Print(L".");
                    }
                    Print(L"\n");
                    gBS->RestoreTPL (OldTpl);
                    goto Done;
                }
                else {
                    Print(L"Process didn't processed normally. Somethings is gone wrong.\n");
                }

            }
        }
    }


    Print(L"\nURD is not detected. System will be shut down.\n\n");
    gBS->Stall(100000);
    Print(L"Shutdown in 5 seconds...\n");
    gBS->Stall(100000); 
    Print(L"Shutdown in 4 seconds...\n");
    gBS->Stall(100000);
    Print(L"Shutdown in 3 seconds...\n");
    gBS->Stall(100000);
    Print(L"Shutdown in 2 seconds...\n");
    gBS->Stall(100000);
    Print(L"Shutdown in 1 second...\n");
    gBS->Stall(100000);

    gRT-> ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    /*
       EfiShellProtocol->Execute (&ImageHandle,
       L"reset -s",
       NULL,
       &Status);*/
    return EFI_NOT_READY;


Done:

    Print(L"\nFirmware checking is complete.\n\n");
    if(ResetRequired && !FlashError) {
        Print(L"Reboot will be proceeded due to flash update.");
        gBS->Stall(100000);
        Print(L"Reboot in 5 seconds...\n");
        gBS->Stall(100000); 
        Print(L"Reboot in 4 seconds...\n");
        gBS->Stall(100000);
        Print(L"Reboot in 3 seconds...\n");
        gBS->Stall(100000);
        Print(L"Reboot in 2 seconds...\n");
        gBS->Stall(100000);
        Print(L"Reboot in 1 second...\n");
        gBS->Stall(100000);
        gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
        /*
           EfiShellProtocol->Execute (&ImageHandle,
           L"reset",
           NULL,
           &Status);
         */
    }
    //
    // Send boot decision message
    //
    Status = UsbProtocol->UsbBulkTransfer (
            UsbProtocol,
            OutEndpointAddr,
            BD_Data,
            &BD_len,
            0,
            &USB_Status
            );
    Print(L"Send boot decision message, Endpoint=0x%02x, Status:%r\n", OutEndpointAddr, Status);
    /*
       Print(L"Hash length=%d\n", Hash_len);
       Print(L"Hash=");
       for(Index=0;Index<Hash_len;Index++) {
       Print(L"%02x ", Hash_Data[Index]);
       }
       Print(L"\n");
       Print(L"Hash value = %a\n", Hash_Data);
     */

    //
    // Receive decision data
    //
    while(1) {
        Status = UsbProtocol->UsbBulkTransfer (
                UsbProtocol,
                InEndpointAddr,
                BD_Data,
                &BD_len,
                0,
                &USB_Status
                );
        if(Verify_len == 16)
            break;
        else
            Verify_len = 16;
    }
    Print(L"Receive decision data value, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);
    /*
       Print(L"verify data length=%d\n", Verify_len);
       Print(L"verify data=");
       for(Index=0;Index<Verify_len;Index++) {
       Print(L"%02x", Verify_Data[Index]);
       }
       Print(L"\n");
     */

    FreePool (UsbHandleBuffer);

    //
    // Loading Bootloader
    //

    /*
       EfiShellProtocol->Execute (&ImageHandle,
       L"fs0:",
       NULL,
       &Status);

       EfiShellProtocol->Execute (&ImageHandle,
       L"grubx64.efi",
       NULL,
       &Status);
     */

    // boot code

    return EFI_SUCCESS;
}
