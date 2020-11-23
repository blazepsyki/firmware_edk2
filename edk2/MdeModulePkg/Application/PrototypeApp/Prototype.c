/** @file
  This UEFI application is to check firmware integrity based on URD.

**/

#include <Uefi.h>

#include <Protocol/UsbIo.h>

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
    UINT32      USB_Status;
    UINTN       HandleCount;
    UINTN       HandleIndex;

    // URD Specification
    UINT16      URD_IdVendor = 7531;
    UINT16      URD_IdProduct = 260;
    UINT8       URD_InterfaceClass = 255;
    UINT8       URD_InterfaceSubClass = 0;
    UINT8       URD_InterfaceProtocol = 0;
        
    EFI_STATUS  Status;
    
    EFI_HANDLE  *HandleBuffer;

    // USB Data Structures
    EFI_USB_IO_PROTOCOL             *UsbProtocol;
    EFI_USB_DEVICE_DESCRIPTOR       DeviceDesc;
    //EFI_USB_CONFIG_DESCRIPTOR       ConfigDesc;
    EFI_USB_INTERFACE_DESCRIPTOR    IntfDesc;
    EFI_USB_ENDPOINT_DESCRIPTOR     EndpDesc;

    
    //UINT8   InEndpointAddr = 0;
    UINT8   OutEndpointAddr = 0;

    CHAR8 Data[8] = "1234567\n";
    UINTN Data_len = 8;

	Index = 0;
  
    Status = gBS->LocateHandleBuffer (
                ByProtocol,
                &gEfiUsbIoProtocolGuid,
                NULL,
                &HandleCount,
                &HandleBuffer
            );
    //Print(L"USB Handle Count: %d\n", HandleCount);

    for(HandleIndex=0; HandleIndex < HandleCount; HandleIndex++) {
        Status = gBS->HandleProtocol (
                    HandleBuffer[HandleIndex],
                    &gEfiUsbIoProtocolGuid,
                    (VOID **) &UsbProtocol
                );
        //Print(L"Open Protocol Status=%d\n", Status);
              
        Status = UsbProtocol->UsbGetDeviceDescriptor (
                    UsbProtocol,
                    &DeviceDesc
                );
        //Print(L"Status= %d\n", Status);
        //Print(L"IdVendor=0x%04x, IdProduct=0x%04x\n", DeviceDesc.IdVendor, DeviceDesc.IdProduct);
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
                //Print(L"URD is detected.\n");
                for(Index = 0; Index < IntfDesc.NumEndpoints; Index++) {
                    Status = UsbProtocol->UsbGetEndpointDescriptor (
                                UsbProtocol,
                                Index,
                                &EndpDesc
                            );
                   // if(EndpDesc.EndpointAddress > 127)
                     //   InEndpointAddr = EndpDesc.EndpointAddress;
                    if(EndpDesc.EndpointAddress < 128)
                        OutEndpointAddr = EndpDesc.EndpointAddress;
                }
                
                /*Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            InEndpointAddr,
                            Data,
                            &Data_len,
                            0,
                            &USB_Status
                        );
                Print(L"Receive nonce value, Endpoint=%02x, Status=%d\n", InEndpointAddr, Status);*/
                Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            OutEndpointAddr,
                            Data,
                            &Data_len,
                            0,
                            &USB_Status
                        );
                Print(L"Send hash value, Endpoint=%02x, Status=%d\n", OutEndpointAddr, Status);


            }
            else {
                Print(L"URD is not detected. System will be shut down.\n");
                // Shutdown Shell command

            }
        }
        
        
        ASSERT_EFI_ERROR(Status);
        
    }
  FreePool (HandleBuffer);  
  return EFI_SUCCESS;
}
