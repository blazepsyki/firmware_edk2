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
    EFI_STATUS  Status;
    
    EFI_HANDLE  *HandleBuffer;

    EFI_USB_IO_PROTOCOL *UsbProtocol;
    //EFI_DEVICE_PATH     *DevicePath = NULL;

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
    ASSERT_EFI_ERROR(Status);

    for(HandleIndex=0;HandleIndex < HandleCount; HandleIndex++) {

        Status = gBS->LocateProtocol (
                &gEfiUsbIoProtocolGuid,
                NULL,
                (VOID **)&UsbProtocol
            );
        ASSERT_EFI_ERROR(Status);
        for(Index=0; Index<16; Index++) {
            Status = UsbProtocol->UsbSyncInterruptTransfer (
                UsbProtocol,
                Index,
                Data,
                &Data_len,
                0,
                &USB_Status
                );
        //DEBUG((EFI_D_INFO, "USB bulk transfer =%d\n", Status));
            Status = UsbProtocol->UsbSyncInterruptTransfer (
                UsbProtocol,
                Index+128,
                Data,
                &Data_len,
                0,
                &USB_Status
                );
        //DEBUG((EFI_D_INFO, "USB bulk transfer =%d\n", Status));
        }     

    }


  FreePool (HandleBuffer);  
  return EFI_SUCCESS;
}
