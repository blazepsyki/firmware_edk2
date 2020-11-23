#include "Prototype.h"

EFI_STATUS
EFIAPI
DxePrototypeEntryPoint (
  IN EFI_HANDLE ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
    CHAR8 Seed_value[8] = "1234567\n";
    UINTN Data_len = 8;

    DEBUG ((EFI_D_INFO, "URD Communication Start\n"));




    return EFI_SUCCESS;
}
