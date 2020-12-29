/** @file

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef _FIRMWARE_UPDATE_H_
#define _FIRMWARE_UPDATE_H_

#include <Uefi.h>
#include <PiDxe.h>

#include <Guid/FileInfo.h>

#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/Hash2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/Spi.h>
#include <Protocol/UsbIo.h>
#include <Protocol/Shell.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

EFI_SPI_PROTOCOL  *mSpiProtocol;
EFI_HASH2_PROTOCOL mHash2Protocol;

//
// Prefix Opcode Index on the host SPI controller.
//
typedef enum {
  SPI_WREN,             // Prefix Opcode 0: Write Enable.
  SPI_EWSR,             // Prefix Opcode 1: Enable Write Status Register.
} PREFIX_OPCODE_INDEX;

//
// Opcode Menu Index on the host SPI controller.
//
typedef enum {
  SPI_READ_ID,        // Opcode 0: READ ID, Read cycle with address.
  SPI_READ,           // Opcode 1: READ, Read cycle with address.
  SPI_RDSR,           // Opcode 2: Read Status Register, No address.
  SPI_WRDI_SFDP,      // Opcode 3: Write Disable or Discovery Parameters, No address.
  SPI_SERASE,         // Opcode 4: Sector Erase (4KB), Write cycle with address.
  SPI_BERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address.
  SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address.
  SPI_WRSR,           // Opcode 7: Write Status Register, No address.
} SPI_OPCODE_INDEX;

#define BLOCK_SIZE  SIZE_4KB

//
// Function Prototypes.
//
EFI_STATUS
EFIAPI
SpiFlashRead (
  IN     UINTN     Address,
  IN OUT UINT32    *NumBytes,
     OUT UINT8     *Buffer
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;
  UINTN         Offset = 0;

  ASSERT ((NumBytes != NULL) && (Buffer != NULL));

    Offset = Address - (UINTN)PcdGet32 (PcdFlashChipBase);

    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             1, //SPI_READ,
                             0, //SPI_WREN,
                             TRUE,
                             TRUE,
                             FALSE,
                             Offset,
                             BLOCK_SIZE,
                             Buffer,
                             EnumSpiRegionAll
                             );
    return Status;

}

STATIC
EFI_STATUS
EFIAPI
SpiFlashWrite (
  IN     UINTN     Address,
  IN OUT UINT32    *NumBytes,
  IN     UINT8     *Buffer
  )
{
  EFI_STATUS                Status;
  UINTN                     Offset;
  UINT32                    Length;
  UINT32                    RemainingBytes;

  ASSERT ((NumBytes != NULL) && (Buffer != NULL));
  ASSERT (Address >= (UINTN)PcdGet32 (PcdFlashChipBase));

  Offset    = Address - (UINTN)PcdGet32 (PcdFlashChipBase);

  ASSERT ((*NumBytes + Offset) <= (UINTN)PcdGet32 (PcdFlashChipSize));

  Status = EFI_SUCCESS;
  RemainingBytes = *NumBytes;

  while (RemainingBytes > 0) {
    if (RemainingBytes > SIZE_4KB) {
      Length = SIZE_4KB;
    } else {
      Length = RemainingBytes;
    }
    Status = mSpiProtocol->Execute (
                             mSpiProtocol,
                             SPI_PROG,
                             SPI_WREN,
                             TRUE,
                             TRUE,
                             TRUE,
                             (UINT32) Offset,
                             Length,
                             Buffer,
                             EnumSpiRegionAll
                             );
    if (EFI_ERROR (Status)) {
      break;
    }
    RemainingBytes -= Length;
    Offset += Length;
    Buffer += Length;
  }

  //
  // Actual number of bytes written.
  //
  *NumBytes -= RemainingBytes;

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
SpiFlashBlockErase (
  IN UINTN    Address,
  IN UINTN    *NumBytes
  )
{
  EFI_STATUS          Status;
  UINTN               Offset;
  UINTN               RemainingBytes;

  ASSERT (NumBytes != NULL);
  ASSERT (Address >= (UINTN)PcdGet32 (PcdFlashChipBase));

  Offset    = Address - (UINTN)PcdGet32 (PcdFlashChipBase);

  ASSERT ((*NumBytes % SIZE_4KB) == 0);
  ASSERT ((*NumBytes + Offset) <= (UINTN)PcdGet32 (PcdFlashChipSize));

  Status = EFI_SUCCESS;
  RemainingBytes = *NumBytes;

    while (RemainingBytes > 0) {
      Status = mSpiProtocol->Execute (
                               mSpiProtocol,
                               SPI_SERASE,
                               SPI_WREN,
                               FALSE,
                               TRUE,
                               FALSE,
                               (UINT32) Offset,
                               0,
                               NULL,
                               EnumSpiRegionAll
                               );
      if (EFI_ERROR (Status)) {
        break;
      }
      RemainingBytes -= SIZE_4KB;
      Offset         += SIZE_4KB;
    }

  //
  // Actual number of bytes erased.
  //
  *NumBytes -= RemainingBytes;

  return Status;
}



#endif
