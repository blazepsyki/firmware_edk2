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

    // Handle variables
    EFI_HANDLE  *UsbHandleBuffer;
    UINTN       UsbHandleCount;
    UINTN       UsbHandleIndex;

    EFI_HANDLE  BootLoaderHandle;

    EFI_HANDLE  *Hash2ControllerHandle;
    EFI_HANDLE  Hash2SBHandle           = NULL;
    EFI_HANDLE  Hash2ChildHandle        = NULL;

    // URD Specification
    UINT16      URD_IdVendor = 1317;
    UINT16      URD_IdProduct = 42149;
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
    CHAR8 Hello_Data[16]    = "1234567890ABCDEF";
    CHAR8 Nonce_Data[33]    = { 0, };
    CHAR8 Hash_Data[32]     = "1A3E7942FC31AE45679B21DA967CE275";
    UINT8 FW_Data[4096]     = { 0, };
    UINT8 Verify_Data[100]  = { 0, };
    CHAR8 BD_Data[48]     = "FEDCBA0987654321";

    UINTN Hello_len     = 16;
    UINTN Nonce_len     = 32;
    UINTN Hash_len      = 32;
    UINTN FW_len        = 4096;
    UINTN Verify_len    = 100;
    UINTN BD_len      = 16;

    // Firmware read variables
    UINT32 ReadByte_Num = 4096;
    UINT8 ReadBuffer[4096] = { 0, };

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

    // Device and Boot Path
    CONST CHAR16 HashDriverPath[] = L"PciRoot(0x0)/Pci(0x14,0x0)/USB(0x0,0x0)/HD(1,MBR,0xA82DEFD5,0x800,0x3A7F800)/Hash2DxeCrypto.efi";
    CONST CHAR16 BootPath_Succ[] = L"PciRoot(0x0)/Pci(0x14,0x0)/USB(0x0,0x0)/HD(1,MBR,0xA82DEFD5,0x800,0x3A7F800)/Hash2DxeCrypto.efi";
    CONST CHAR16 BootPath_Fail[] = L"PciRoot(0x0)/Pci(0x14,0x0)/USB(0x0,0x0)/HD(1,MBR,0xA82DEFD5,0x800,0x3A7F800)/grupX64.efi";

    // Hash variables
    EFI_HASH2_OUTPUT HashResult;

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

    //
    // Make available protocols to use them
    //
    
    // Locate Spi protocol
    Status = gBS->LocateProtocol (
            &gEfiSpiProtocolGuid,
            NULL,
            (VOID **)&mSpiProtocol
            );

    //Print(L"Load SPI Protocol Status: %r\n", Status);
    if (EFI_ERROR (Status))
        return Status;

    // Locate Shell protocol TODO: fix it
    Status = gBS->LocateProtocol (
            &gEfiShellProtocolGuid,
            NULL,
            (VOID **) &EfiShellProtocol
            );
    Print(L"Load Shell Protocol Status: %r\n", Status);
    /*if (EFI_ERROR (Status))
        return Status;*/

    // Make interface for hash protocol
    /*Status = gBS->InstallProtocolInterface (
            Hash2SBHandle,
            &gEfiHash2ServiceBindingProtocolGuid,
            EFI_NATIVE_INTERFACE,
            (VOID **) &mHash2ServiceBindingProtocol
            );
    Print(L"Install Protocol Interface for hash Status: %r\n", Status);
    if (EFI_ERROR (Status))
        return Status;*/
    
    // Load and start Hash Driver from USB 
    Status = gBS->LoadImage (
            FALSE,
            ImageHandle,
            ConvertTextToDevicePath (HashDriverPath),
            NULL,
            0,
            &Hash2SBHandle);
    //Print(L"Load Hash driver Status: %r\n", Status);
    if (EFI_ERROR (Status))
        return Status;
    
    Status = gBS->StartImage (
            Hash2SBHandle,
            0,
            NULL);
    //Print(L"Start Hash driver Status: %r\n", Status);  
    if (EFI_ERROR (Status))
        return Status;

    // Locate Hash Service Binding protocol
    Status = gBS->LocateProtocol (
            &gEfiHash2ServiceBindingProtocolGuid,
            NULL,
            (VOID **)&mHash2ServiceBindingProtocol
            );
    //Print(L"Hash Service Binding Status: %r\n", Status);
    if (EFI_ERROR (Status))
        return Status;

    // Create a ChildHandle with the Hash2 Protocol
    Status = mHash2ServiceBindingProtocol->CreateChild (mHash2ServiceBindingProtocol, &Hash2ChildHandle);
    //Print(L"Child Handle by Hash Service Binding Protocol Status: %r\n", Status);
    if (EFI_ERROR (Status))
        return Status;
        
    // Retrieve the Hash2Protocol from Hash2ChildHandle
    Status = gBS->OpenProtocol ( 
            Hash2ChildHandle,
            &gEfiHash2ProtocolGuid,
            (VOID **)&mHash2Protocol,
            Hash2SBHandle,
            Hash2ControllerHandle,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
    //Print(L"Hash Protocol Status: %r\n", Status);
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

    //
    // Make connection with URD
    //
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
                Index = 5308416;
                Address = PcdGet32 (PcdFlashChipBase) + Index;

                Status = mHash2Protocol->HashInit (
                        mHash2Protocol,
                        &gEfiHashAlgorithmSha256Guid);
                //Print(L"Hash Initialize status: %r\n", Status);
                if (EFI_ERROR (Status))
                    return Status;
                
                // Hash test code for nonce only
                /*Status = mHash2Protocol->Hash (
                        mHash2Protocol,
                        &gEfiHashAlgorithmSha256Guid,
                        (CONST UINT8 *) Nonce_Data,
                        32,
                        &HashResult);
                Print(L"Hash, with only Nonce, value: ");
                for(Index = 0; Index < 32; Index++) {
                    Print(L"%02x ", HashResult.Sha256Hash[Index]);
                }
                Print(L"\n");
                */
                
                Status = mHash2Protocol->HashUpdate (
                        mHash2Protocol,
                        (CONST UINT8 *) Nonce_Data,
                        32);
                Print(L"Hash Update first with nonce, Status: %r\n", Status);
 
                if (EFI_ERROR (Status))
                    return Status;

                OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
                while(Index < (1<<23)) {
                    Status = SpiFlashRead (
                            (UINTN) Address,
                            &ReadByte_Num, 
                            ReadBuffer);
//                    if(EFI_ERROR(Status)) {
//                        Print(L"SpiError: %r, %d, %d\r", Status, Address, &ReadByte_Num);
//                        continue;
//                    }
                    if((EFI_ERROR(Status)) || (ReadByte_Num != 4096)) {
                        ReadByte_Num = 4096;
//                        Print(L"SpiError: %r, %d, %d\n", Status, Address, &ReadByte_Num);
                        continue;
                    }
                    Status = mHash2Protocol->HashUpdate (
                            mHash2Protocol,
                            ReadBuffer,
                            ReadByte_Num);
                    if(EFI_ERROR(Status)) {
//                        Print(L"HashError: %r, %d, %d\n", Status, Address, &ReadByte_Num);
                        continue;
                    }
//                    Print(L"Read SPI and Hash update: %r, Readbyte: %d, Index: %d\n", Status, ReadByte_Num, (Index>>12));
                    // Test code: reading flash verification
                    /*Print(L"Read result=");
                    for(Index2 = 0; Index2 < ReadByte_Num; Index2++) {
                        Print(L"%02x ", ReadBuffer[Index2]);
                    }
                    Print(L"\n");*/
//                    Print (L".");
                    Address += 1<<12;
                    Index += 1<<12;
                }

                gBS->RestoreTPL (OldTpl);
                Status = mHash2Protocol->HashFinal (
                        mHash2Protocol,
                        &HashResult
                        );
                Print(L"\rHash Computation complete, Status: %r\n", Status);
                if (EFI_ERROR (Status))
                    return Status;
                Print(L"Hash value: %a\n", HashResult.Sha256Hash);
                Print(L"Hash value: ");
                for(Index = 0; Index < 32; Index++) {
                    Print(L"%02x ", HashResult.Sha256Hash[Index]);
                }
                Print(L"\n");

                // Copy Hash value to variable for Tx
                for(Index = 0; Index < 32; Index ++) {
                    Hash_Data[Index] = (CHAR8) HashResult.Sha256Hash[Index];
                }
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
                    if(Verify_len == 40)
                        break;
                    else
                        Verify_len = 40;
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
                        Print(L"Error occurs when erasing. Reboot now.\n");
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
    }
    //
    // Exchange boot decision
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
        if(BD_len == 48)
            break;
        else
            BD_len = 48;
    }
    Print(L"Receive decision data value, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);

    Print(L"Decision data data length=%d\n", BD_len);
    Print(L"Decision data=");
    for(Index=0;Index<BD_len;Index++) {
        Print(L"%02x", BD_Data[Index]);
    }
    Print(L"\n");

    FreePool (UsbHandleBuffer);

    //
    // Loading Bootloader
    //
    if(BD_Data[10] == 'O') {
        Status = gBS->LoadImage (
                FALSE,
                ImageHandle,
                ConvertTextToDevicePath (BootPath_Succ),
                NULL,
                0,
                &BootLoaderHandle);
        Print(L"Load Normal bootloader Status: %r\n", Status);
        if (EFI_ERROR (Status))
            return Status;
    }
    else if(BD_Data[10] == 'X') {
        Status = gBS->LoadImage (
                FALSE,
                ImageHandle,
                ConvertTextToDevicePath (BootPath_Fail),
                NULL,
                0,
                &BootLoaderHandle);
        Print(L"Load Emergency bootloader Status: %r\n", Status);
        if (EFI_ERROR (Status))
            return Status;

    }
    else
    {
        Print(L"Wrong response. Program will shut down.\n");
        return EFI_UNSUPPORTED;
    }
    Status = gBS->StartImage (
            BootLoaderHandle,
            0,
            NULL);
    return EFI_SUCCESS;
}
