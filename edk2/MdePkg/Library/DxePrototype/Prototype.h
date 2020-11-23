#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
#include <Protocol/UsbIo.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.hD>

EFI_STATUS
EFIAPI
DxePrototypeEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
);

typedef struct {
  UINT64 Signature;

  EFI_USB_IO_PROTOCOL             *UsbIo;

  EFI_USB_INTERFACE_DESCRIPTOR    InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR     BulkInEndpointDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR     BulkOutEndpointDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR     InterruptEndpointDescriptor;

} USB_URD_DEVICE;
