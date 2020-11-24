/** @file
  This UEFI application is to check firmware integrity based on URD.

**/

#include <Uefi.h>

#include <Protocol/UsbIo.h>
#include <Protocol/Shell.h>

#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

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
    UINT32      USB_Status = 0;
    EFI_STATUS  Status;

    // Handle variables
    EFI_HANDLE  *HandleBuffer;
    UINTN       HandleCount;
    UINTN       HandleIndex;

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
    //CHAR8 Nonce_Data[33] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    CHAR8 Hash_Data[33] = "1A3E7942FC31AE45679B21DA967CE27\r\0";
    UINT8 Verify_Data = 127;
    //UINTN Nonce_len = 32;
    UINTN Hash_len = 33;
    UINTN VD_len = 1;

    // Firmware read variables 
    //UINTN       AddrIdx;

	Index = 0;
 
    Status = gBS->LocateProtocol (&gEfiShellProtocolGuid,
                    NULL,
                    (VOID **) &EfiShellProtocol
            );

    if (EFI_ERROR (Status))
        return Status;

    Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiUsbIoProtocolGuid,
                NULL,
                &HandleCount,
                &HandleBuffer
            );
    Print(L"Connected USB Device: %d\n", HandleCount);

    for(HandleIndex=0; HandleIndex < HandleCount; HandleIndex++) {
        Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &UsbProtocol
                );
        //Print(L"Opening Device Handle, Status=%d\n", Status);
              
        Status = UsbProtocol->UsbGetDeviceDescriptor (
                    UsbProtocol,
                    &DeviceDesc
                );
        //Print(L"Retrieving Device Descriptor, Status= %d\n", Status);
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
                
                /*Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            InEndpointAddr,
                            Nonce_Data,
                            &Nonce_len,
                            0,
                            &USB_Status
                        );
                Print(L"Receive nonce value, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);
                */
                /*Print(L"Nonce length=%d\n", Nonce_len);
                Print(L"Nonce=");
                for(Index=0;Index<Nonce_len;Index++) {
                    Print(L"%02x ", Nonce_Data[Index]);
                }
                Print(L"\n");*/
                
                // Hash Computation
                //gBS->Stall(10000);
                /*for(AddrIdx=0;AddrIdx<(1<<22);AddrIdx+=1<<3) {
                    SpiFlashRead( );
                    
                }*/
                Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            OutEndpointAddr,
                            Hash_Data,
                            &Hash_len,
                            0,
                            &USB_Status
                        );
                Print(L"Send hash value, Endpoint=0x%02x, Status:%r\n", OutEndpointAddr, Status);
                Print(L"Hash value=%a", Hash_Data);

                Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            InEndpointAddr,
                            &Verify_Data,
                            &VD_len,
                            0,
                            &USB_Status
                        );
                Print(L"Receive verification result, Endpoint=0x%02x, Status:%r\n", InEndpointAddr, Status);
                Print(L"%r\n", USB_Status);
                if(Verify_Data == 1) {
                    Print(L"this device is secure.\n");
                    Print(L"Firmware integrity validation completed. Boot process will be proceed.\n");
                    gBS->Stall(3000);
                    goto Done;
                }
                else if(Verify_Data == 2) {
                    Print(L"this device is not secure. Firmware overwrite initiated.\n");
                    /*Status = UsbProtocol->UsbBulkTransfer (
                                UsbProtocol,
                                InEndpointAddr,
                                &Verify_Data,
                                &Data_len,
                                0,
                                &USB_Status
                            );*/
                    Print(L"Receive firmware image, Endpoint=%02x\n", InEndpointAddr);
                    // Firmware Overwrite
                    
                    gBS->Stall(5000);
                    Print(L"Status:%r\n", Status);
                    Print(L"Recovery complete. System will be reboot soon.\n");
                    EfiShellProtocol->Execute (&ImageHandle,
                        L"reset",
                        NULL,
                        &Status);
                    return EFI_SUCCESS;

                }
                else {
                    Print(L"process didn't processed normaly.\n");
                }
                goto Done;
            }
        }
    }

    Print(L"\nURD is not detected. System will be shut down.\n\n");
    gBS->Stall(1000);
    Print(L"Shutdown in 5 seconds...");
    gBS->Stall(1000); 
    Print(L"Shutdown in 4 seconds...");
    gBS->Stall(1000);
    Print(L"Shutdown in 3 seconds...");
    gBS->Stall(1000);
    Print(L"Shutdown in 2 seconds...");
    gBS->Stall(1000);
    Print(L"Shutdown in 1 second...");
    gBS->Stall(1000);

    EfiShellProtocol->Execute (&ImageHandle,
                        L"reset -s",
                        NULL,
                        &Status);
    return EFI_NOT_READY;

Done:
    FreePool (HandleBuffer);  
    return EFI_SUCCESS;
}
