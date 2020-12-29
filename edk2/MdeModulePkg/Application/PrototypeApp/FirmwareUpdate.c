

#include "FirmwareUpdate.h"



//
// MinnowMax Flash Layout
//
//Start (hex)	End (hex)	Length (hex)	Area Name
//-----------	---------	------------	---------
//00000000	007FFFFF	00800000	Flash Image
//
//00000000	00000FFF	00001000	Descriptor Region
//00001000	003FFFFF	003FF000	TXE Region
//00500000	007FFFFF	00400000	BIOS Region
//


EFI_SPI_PROTOCOL  *mSpiProtocol;






INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINT32                FileSize;
  UINT32                BufferSize;
  UINT8                 *FileBuffer;
  UINT8                 *Buffer;
  EFI_PHYSICAL_ADDRESS  Address;
  UINTN                 CountOfBlocks;
  EFI_TPL               OldTpl;
  BOOLEAN               ResetRequired;
  BOOLEAN               FlashError;

  Index             = 0;
  FileSize          = 0;
  BufferSize        = 0;
  FileBuffer        = NULL;
  Buffer            = NULL;
  Address           = 0;
  CountOfBlocks     = 0;
  ResetRequired     = FALSE;
  FlashError        = FALSE;

  Status = EFI_SUCCESS;

  mInputData.FullFlashUpdate = TRUE;


  //
  // Locate the SPI protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiSpiProtocolGuid,
                  NULL,
                  (VOID **)&mSpiProtocol
                  );
  if (EFI_ERROR (Status)) {
    PrintToken (STRING_TOKEN (STR_SPI_NOT_FOUND), HiiHandle);
    return EFI_DEVICE_ERROR;
  }



    // Update it.
    //
    Buffer        = FileBuffer;
    BufferSize    = FileSize;
    Address       = PcdGet32 (PcdFlashChipBase);
    CountOfBlocks = (UINTN) (BufferSize / BLOCK_SIZE);

    //
    // Raise TPL to TPL_NOTIFY to block any event handler,
    // while still allowing RaiseTPL(TPL_NOTIFY) within
    // output driver during Print().
    //
    OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
    for (Index = 0; Index < CountOfBlocks; Index++) {
      //
      // Handle block based on address and contents.
      //
      if (!UpdateBlock (Address)) {
        DEBUG((EFI_D_INFO, "Skipping block at 0x%lx\n", Address));
      } else if (!EFI_ERROR (InternalCompareBlock (Address, Buffer))) {
        DEBUG((EFI_D_INFO, "Skipping block at 0x%lx (already programmed)\n", Address));
      } else {
        //
        // Display a dot for each block being updated.
        //
        Print (L".");

        //
        // Flag that the flash image will be changed and the system must be rebooted
        // to use the change.
        //
        ResetRequired = TRUE;

        //
        // Make updating process uninterruptable,
        // so that the flash memory area is not accessed by other entities
        // which may interfere with the updating process.
        //
        Status  = InternalEraseBlock (Address);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR (Status)) {
          gBS->RestoreTPL (OldTpl);
          FlashError = TRUE;
          goto Done;
        }
        Status = InternalWriteBlock (
                  Address,
                  Buffer,
                  (BufferSize > BLOCK_SIZE ? BLOCK_SIZE : BufferSize)
                  );
        if (EFI_ERROR (Status)) {
          gBS->RestoreTPL (OldTpl);
          FlashError = TRUE;
          goto Done;
        }
      }

      //
      // Move to next block to update.
      //
      Address += BLOCK_SIZE;
      Buffer += BLOCK_SIZE;
      if (BufferSize > BLOCK_SIZE) {
        BufferSize -= BLOCK_SIZE;
      } else {
        BufferSize = 0;
      }
    }
    gBS->RestoreTPL (OldTpl);

    //
    // Print result of update.
    //
    if (!FlashError) {
      if (ResetRequired) {
        Print (L"\n");
        PrintToken (STRING_TOKEN (STR_FWUPDATE_UPDATE_SUCCESS), HiiHandle);
      } else {
        PrintToken (STRING_TOKEN (STR_FWUPDATE_NO_RESET), HiiHandle);
      }
    } else {
      goto Done;
    }
  }

  //
  // All flash updates are done so see if the system needs to be reset.
  //
  if (ResetRequired && !FlashError) {
    //
    // Update successful.
    //
    for (Index = 5; Index > 0; Index--) {
      PrintToken (STRING_TOKEN (STR_FWUPDATE_SHUTDOWN), HiiHandle, Index);
      gBS->Stall (1000000);
    }

    gRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
    PrintToken (STRING_TOKEN (STR_FWUPDATE_MANUAL_RESET), HiiHandle);
    CpuDeadLoop ();
  }

Done:
  //
  // Print flash update failure message if error detected.
  //
  if (FlashError) {
    PrintToken (STRING_TOKEN (STR_FWUPDATE_UPDATE_FAILED), HiiHandle, Index);
  }

  //
  // Do cleanup.
  //
  if (HiiHandle != NULL) {
    HiiRemovePackages (HiiHandle);
  }
  if (FileBuffer) {
    gBS->FreePool (FileBuffer);
  }

  return Status;
}

/**
  Read NumBytes bytes of data from the address specified by
  PAddress into Buffer.

  @param[in]      Address       The starting physical address of the read.
  @param[in,out]  NumBytes      On input, the number of bytes to read. On output, the number
                                of bytes actually read.
  @param[out]     Buffer        The destination data buffer for the read.

  @retval         EFI_SUCCESS       Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
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

/**
  Write NumBytes bytes of data from Buffer to the address specified by
  PAddresss.

  @param[in]      Address         The starting physical address of the write.
  @param[in,out]  NumBytes        On input, the number of bytes to write. On output,
                                  the actual number of bytes written.
  @param[in]      Buffer          The source data buffer for the write.

  @retval         EFI_SUCCESS       Opertion is successful.
  @retval         EFI_DEVICE_ERROR  If there is any device errors.

**/
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

/**
  Erase the block starting at Address.

  @param[in]  Address         The starting physical address of the block to be erased.
                              This library assume that caller garantee that the PAddress
                              is at the starting address of this block.
  @param[in]  NumBytes        On input, the number of bytes of the logical block to be erased.
                              On output, the actual number of bytes erased.

  @retval     EFI_SUCCESS.      Opertion is successful.
  @retval     EFI_DEVICE_ERROR  If there is any device errors.

**/
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


