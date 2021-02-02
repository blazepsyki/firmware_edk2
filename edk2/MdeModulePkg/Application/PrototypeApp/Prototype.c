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
    UINT8       RNG         = 0; 
    UINT32      USB_Status  = 0;
    EFI_STATUS  Status;

    // Handle variables
    EFI_HANDLE  *UsbHandleBuffer;
    UINTN       UsbHandleCount;
    UINTN       UsbHandleIndex;

    EFI_HANDLE  BootLoaderHandle;

    EFI_HANDLE  *Hash2ControllerHandle;
    EFI_HANDLE  Hash2SBHandle           = NULL;
    EFI_HANDLE  Hash2ChildHandle        = NULL;

    EFI_HANDLE RNGHandle                = NULL;

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

    // RNG Data Structure
    EFI_RNG_PROTOCOL *mRngProtocol;

    // USB Data Structures
    EFI_USB_IO_PROTOCOL             *UsbProtocol;
    EFI_USB_DEVICE_DESCRIPTOR       DeviceDesc;
    //EFI_USB_CONFIG_DESCRIPTOR       ConfigDesc;
    EFI_USB_INTERFACE_DESCRIPTOR    IntfDesc;
    EFI_USB_ENDPOINT_DESCRIPTOR     EndpDesc;

    //One-time Password
    UINT8 Auth_Data[32][32] = { {0x35, 0xC3, 0x10, 0xAA, 0x38, 0xA6, 0x70, 0x11, 0x23, 0x29, 0x3B, 0x3F, 0x41, 0x9D, 0xE8, 0xE9, 0x20, 0xC0, 0x82, 0xC0, 0xAB, 0xA5, 0x7B, 0xBD, 0xB2, 0x30, 0xDB, 0xD0, 0x1A, 0x08, 0xD3, 0x7C},
        {0xA6, 0x50, 0x67, 0xB6, 0xAE, 0x31, 0x70, 0x90, 0xD8, 0x9C, 0x2A, 0x91, 0x2F, 0x1D, 0xDF, 0xDD, 0x42, 0xC1, 0x74, 0x44, 0xD8, 0x92, 0x43, 0xAC, 0x5B, 0x67, 0x84, 0xE9, 0x50, 0x35, 0x43, 0x93},
        {0xD8, 0x6B, 0x90, 0xFF, 0xF0, 0x2E, 0x67, 0x4D, 0x6F, 0x18, 0xF5, 0x02, 0x73, 0x64, 0x80, 0xB1, 0x1B, 0x24, 0xD9, 0x29, 0xB9, 0x54, 0xA8, 0x3C, 0xA0, 0xC5, 0xE3, 0xEE, 0x79, 0x60, 0x61, 0x66},
        {0x21, 0xA8, 0x07, 0x9D, 0x37, 0xE3, 0x2E, 0x0A, 0x17, 0x98, 0x9E, 0xAD, 0xA1, 0xF8, 0xA7, 0x28, 0x7A, 0xBF, 0x61, 0xE4, 0xA0, 0xFB, 0x97, 0x5B, 0xFE, 0xE4, 0xE0, 0x6C, 0x08, 0xF4, 0x76, 0x51},
        {0x18, 0xC8, 0xBB, 0x15, 0xEB, 0xC6, 0xA9, 0x6D, 0xA6, 0xA4, 0x1E, 0x21, 0x47, 0x29, 0x8A, 0x14, 0xF7, 0xDA, 0xF3, 0x66, 0x68, 0x5A, 0xAC, 0x7A, 0x85, 0x2F, 0xF7, 0xF5, 0xE5, 0xD4, 0x3E, 0x7B},
        {0x5F, 0x8E, 0x0E, 0xBC, 0xCE, 0x5C, 0xAF, 0x99, 0x69, 0x24, 0x9D, 0xAC, 0xB9, 0xB5, 0x4C, 0xEA, 0x3E, 0x58, 0x66, 0xF7, 0xFE, 0x60, 0xF4, 0x7E, 0x81, 0x9D, 0x4A, 0x83, 0x6C, 0x71, 0x57, 0x85},
        {0xCF, 0x51, 0x1C, 0xEB, 0xE2, 0x07, 0x18, 0x41, 0x8F, 0x60, 0x1E, 0x6C, 0x3E, 0x4D, 0xAA, 0xFC, 0x01, 0xCA, 0x88, 0x7B, 0xA6, 0x3C, 0xF4, 0x24, 0xD7, 0x14, 0xF1, 0xC8, 0x4F, 0x5D, 0xF0, 0x76},
        {0x36, 0xA9, 0xFB, 0xEF, 0x3F, 0xCD, 0x9C, 0xF3, 0x01, 0xF3, 0x74, 0xA4, 0x66, 0xC1, 0x50, 0x54, 0xFB, 0x95, 0x60, 0x7D, 0x47, 0xC4, 0x1B, 0x1D, 0xAE, 0x0A, 0xC1, 0x59, 0xE7, 0xD3, 0x02, 0xDA},
        {0xAA, 0x7B, 0x1E, 0xC6, 0x03, 0x9C, 0x9E, 0x61, 0x45, 0x6D, 0xF1, 0x23, 0x9E, 0xD7, 0xA1, 0x8F, 0xCA, 0xD2, 0x7E, 0xC6, 0x6E, 0x52, 0x5A, 0x4A, 0x9E, 0x54, 0xD8, 0x9F, 0x23, 0xFA, 0x97, 0xD3},
        {0x14, 0xAC, 0x04, 0x5F, 0x6C, 0xD8, 0xAE, 0xDA, 0xED, 0xC9, 0x29, 0xD0, 0xBA, 0x86, 0x60, 0x8F, 0xB6, 0x2C, 0xDA, 0xED, 0x61, 0x86, 0x39, 0xB2, 0xED, 0x91, 0xE3, 0x78, 0x0D, 0xD5, 0xB8, 0x80},
        {0x65, 0xED, 0x35, 0x56, 0xCA, 0xD3, 0x21, 0x09, 0x1D, 0xB7, 0x32, 0x39, 0x62, 0x43, 0xA7, 0x67, 0xD5, 0x43, 0x83, 0x35, 0xBB, 0x69, 0xB2, 0x9A, 0x71, 0xCA, 0x52, 0xF1, 0x00, 0x89, 0x91, 0xBA},
        {0xC7, 0x71, 0xE2, 0x19, 0x4B, 0x9A, 0x3A, 0x68, 0x19, 0x7D, 0x4C, 0x36, 0x6A, 0xDC, 0xC6, 0xAB, 0xE0, 0x24, 0x81, 0x5F, 0x31, 0x50, 0x81, 0x76, 0x48, 0x84, 0x8D, 0xEF, 0x28, 0x4E, 0xED, 0xB2},
        {0x62, 0x4E, 0x86, 0x65, 0x94, 0x75, 0xE5, 0xEC, 0xC5, 0xA6, 0x1A, 0xB1, 0x12, 0x34, 0x41, 0x71, 0x77, 0x84, 0x2C, 0xA9, 0xFF, 0x3F, 0xD9, 0x46, 0x76, 0x3E, 0x9E, 0x7D, 0xE4, 0xE7, 0x64, 0xA6},
        {0xAA, 0xE0, 0xDD, 0x36, 0xDD, 0x9C, 0x96, 0x12, 0x27, 0xD0, 0x6E, 0x56, 0xEC, 0x93, 0xBF, 0x18, 0xE6, 0x81, 0x95, 0xF1, 0x32, 0xFF, 0x7C, 0xA5, 0xB3, 0x76, 0x98, 0x42, 0xE1, 0x11, 0x83, 0x90},
        {0x36, 0xC4, 0x5F, 0x62, 0x06, 0xF0, 0xAD, 0x70, 0xA1, 0x92, 0xC6, 0xA1, 0xB4, 0xA5, 0xED, 0x03, 0x77, 0x1F, 0x8C, 0xA9, 0xEE, 0x7B, 0x26, 0xDE, 0x1B, 0x51, 0x4C, 0x5C, 0xE8, 0xCF, 0xC1, 0x10},
        {0x3C, 0xE2, 0xE5, 0x5B, 0x1F, 0x8A, 0x90, 0x67, 0x4C, 0x0E, 0x3D, 0xCE, 0x2E, 0xE3, 0x09, 0xF7, 0xCC, 0xB1, 0xF3, 0x49, 0xFF, 0x29, 0x31, 0xD0, 0xB8, 0xC7, 0x24, 0x9E, 0x69, 0xD5, 0xEC, 0x15},
        {0xAF, 0xBE, 0x54, 0x56, 0xAB, 0x55, 0xED, 0x05, 0xEB, 0x67, 0x0E, 0x3E, 0x71, 0x6E, 0x2F, 0xB3, 0x9E, 0x1C, 0xB6, 0xDF, 0xDA, 0xD4, 0x8A, 0xD7, 0xF1, 0xA1, 0xC4, 0xFE, 0x3D, 0x40, 0x69, 0x63},
        {0xB0, 0xE5, 0xC9, 0x58, 0x2C, 0x80, 0x52, 0x06, 0xE2, 0x5D, 0x3F, 0x5A, 0x98, 0xCB, 0x83, 0x34, 0xD0, 0xFD, 0xCC, 0xA8, 0x15, 0x5A, 0x26, 0x42, 0x5D, 0x81, 0x39, 0x95, 0x38, 0x8C, 0x2D, 0xCD},
        {0xE4, 0x4B, 0x3B, 0x71, 0xFA, 0x5C, 0xA3, 0x98, 0xE2, 0xEC, 0x90, 0x27, 0x57, 0x27, 0x74, 0x01, 0x87, 0x63, 0x20, 0x97, 0x10, 0x92, 0x0A, 0x4C, 0x2C, 0x64, 0x70, 0xC9, 0x7F, 0x73, 0x7B, 0xFF},
        {0x7E, 0x8E, 0xFA, 0x52, 0x49, 0x5E, 0x62, 0x96, 0x96, 0x46, 0x8A, 0x78, 0x13, 0xE2, 0xE2, 0x19, 0x51, 0x4B, 0xA6, 0x8C, 0x99, 0x86, 0xDB, 0xF4, 0x97, 0xF4, 0xF1, 0x17, 0xCF, 0xAC, 0xF7, 0x03},
        {0x22, 0xF9, 0xFC, 0x3E, 0x14, 0x9C, 0x87, 0x24, 0xD7, 0xAA, 0xB4, 0x40, 0x36, 0xA5, 0xEB, 0xEB, 0x9A, 0xDB, 0xE3, 0x5F, 0xA7, 0xB5, 0xE1, 0x27, 0x8C, 0xB8, 0x38, 0x30, 0x76, 0x64, 0xF6, 0xAB},
        {0xA3, 0xC4, 0xD0, 0xC1, 0x54, 0xB3, 0x8E, 0xAE, 0xBC, 0xC7, 0xD1, 0x78, 0x8D, 0xDB, 0x38, 0x61, 0xC1, 0x83, 0x14, 0x80, 0xA9, 0xF6, 0x9D, 0xC2, 0x97, 0xFB, 0xE2, 0x4F, 0x46, 0xD0, 0x30, 0x93},
        {0xA7, 0xFE, 0xC6, 0xCF, 0x0F, 0xB1, 0x1D, 0x91, 0x07, 0x2D, 0xA1, 0x65, 0xBA, 0x0D, 0x21, 0xA4, 0x05, 0x52, 0x8C, 0xF0, 0x2B, 0xE0, 0x36, 0x73, 0xD5, 0x9F, 0xF4, 0x18, 0xBA, 0x1A, 0x88, 0xF1},
        {0xBE, 0x45, 0xF0, 0x77, 0x10, 0xDD, 0xF4, 0x8E, 0xA7, 0x8D, 0xC8, 0xB8, 0x18, 0x00, 0x11, 0x67, 0x1A, 0x1E, 0xC1, 0x39, 0xA2, 0xF6, 0xB6, 0x86, 0x7A, 0x89, 0x2D, 0x6E, 0x9E, 0x31, 0x24, 0xC8},
        {0x75, 0x32, 0x78, 0x8A, 0x79, 0x3B, 0x1D, 0xD6, 0x72, 0x79, 0x4A, 0x5C, 0x12, 0x9E, 0x32, 0x84, 0x6F, 0x74, 0xAA, 0xE1, 0x55, 0x6B, 0xDD, 0xFF, 0xC0, 0xA0, 0x40, 0x5C, 0x5C, 0x36, 0x91, 0x69},
        {0x17, 0x70, 0xD8, 0x8E, 0xE5, 0x5E, 0x17, 0xD0, 0x56, 0xA3, 0xCE, 0x8D, 0x5B, 0x9F, 0x47, 0x1A, 0x94, 0xC8, 0x72, 0xAB, 0x42, 0x86, 0x47, 0x3E, 0x12, 0x16, 0x1B, 0x04, 0xD7, 0x3D, 0xBF, 0xB0},
        {0x78, 0xCC, 0xFB, 0x9D, 0x84, 0x24, 0x03, 0x8C, 0xBC, 0xE8, 0x34, 0xBB, 0x25, 0xD5, 0x2D, 0x61, 0x7E, 0x49, 0x6D, 0x72, 0x5D, 0x76, 0xB2, 0x6D, 0x72, 0xCB, 0x2A, 0xB1, 0x3A, 0xCB, 0x50, 0x4D},
        {0x9F, 0x00, 0x41, 0x25, 0x33, 0x6B, 0x4B, 0x64, 0xB9, 0xF6, 0x9D, 0xC5, 0x84, 0xD0, 0xB9, 0x80, 0xD9, 0x16, 0x15, 0x34, 0x74, 0xA0, 0x8D, 0x08, 0x72, 0x85, 0xDF, 0x75, 0xE9, 0xE0, 0xD3, 0x34},
        {0xFA, 0xCD, 0x14, 0x75, 0xBB, 0x5C, 0x9E, 0x33, 0x42, 0x6C, 0x7B, 0xBE, 0x7E, 0x8B, 0xFB, 0xED, 0xD5, 0xAB, 0x73, 0x66, 0xCE, 0xAA, 0x9C, 0x5D, 0xFA, 0xE8, 0x2A, 0xFE, 0x1F, 0xF2, 0x44, 0xFB},
        {0x98, 0x92, 0xEF, 0x56, 0xFB, 0xF2, 0xC9, 0x99, 0x71, 0x2E, 0x99, 0x87, 0x4D, 0x52, 0xE9, 0xD0, 0x7A, 0x01, 0xEC, 0x53, 0x3B, 0xA2, 0x25, 0x04, 0x0E, 0x3E, 0xD3, 0xB6, 0x38, 0x07, 0x16, 0xF0},
        {0xA0, 0x3A, 0xD8, 0x15, 0x26, 0x5B, 0xDF, 0x52, 0x9C, 0x44, 0x5F, 0xC3, 0x80, 0x62, 0xDD, 0x24, 0xB8, 0xA5, 0xF8, 0x33, 0x29, 0xB6, 0xFD, 0x30, 0x19, 0x8A, 0xC3, 0x45, 0x64, 0xEA, 0x76, 0x68},
        {0xE3, 0x62, 0x61, 0x5B, 0x13, 0x19, 0xBB, 0xB2, 0x5B, 0x0B, 0x71, 0xC2, 0xC2, 0xEE, 0xC2, 0xD4, 0xD8, 0x63, 0x71, 0xA4, 0x98, 0x0B, 0x70, 0x76, 0xC2, 0x8D, 0x91, 0xAF, 0xD3, 0x11, 0x4F, 0x0A} };
    
    // Data transfer
    UINT8 AuthTx_Data       = -1;
    UINT8 AuthRx_Data[32]   = { 0, };
    CHAR8 Hello_Data[16]    = "1234567890ABCDEF";
    CHAR8 Nonce_Data[33]    = { 0, };
    CHAR8 Hash_Data[32]     = "1A3E7942FC31AE45679B21DA967CE275";
    UINT8 FW_Data[4096]     = { 0, };
    UINT8 Verify_Data[100]  = { 0, };
    CHAR8 BD_Data[16]     = "FEDCBA0987654321";

    UINTN AuthTx_len    = 1;
    UINTN AuthRx_len    = 32;
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
    CONST CHAR16 RNGDriverPath[] = L"PciRoot(0x0)/Pci(0x14,0x0)/USB(0x0,0x0)/HD(1,MBR,0xA82DEFD5,0x800,0x3A7F800)/RngDxe.efi";
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
    //Print(L"Load Shell Protocol Status: %r\n", Status);
    //if (EFI_ERROR (Status))
    //    return Status;
 
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

    // Load and start RNG Driver from USB 
    Status = gBS->LoadImage (
            FALSE,
            ImageHandle,
            ConvertTextToDevicePath (RNGDriverPath),
            NULL,
            0,
            &RNGHandle);
    //Print(L"Load RNG driver Status: %r\n", Status);
    if (EFI_ERROR (Status))
        return Status;
    
    Status = gBS->StartImage (
            RNGHandle,
            0,
            NULL);
    //Print(L"Start Hash driver Status: %r\n", Status);  
    if (EFI_ERROR (Status))
        return Status;

    // Locate RNG protocol
    Status = gBS->LocateProtocol (
            &gEfiRngProtocolGuid,
            NULL,
            (VOID **) &mRngProtocol
            );
    //Print(L"RNG Protocol Status: %r\n", Status);
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
                // Send Authentication code
                //
                Status = mRngProtocol->GetRNG (
                        mRngProtocol,
                        &gEfiRngAlgorithmSp80090Ctr256Guid,
                        1,
                        &RNG
                        );
                //Print(L"Getting RNG Status: %r\n", Status);
                /*for(Index = 0; Index < 32; Index++) {
                    AuthRx_Data[Index] = RNG % 32;
                }*/
                AuthTx_Data = (RNG % 32);
                
                //Print(L"RNG number is %d, and its remainder by dividing 32 is %d\n", RNG, (RNG % 32));
                Status = UsbProtocol->UsbBulkTransfer (
                        UsbProtocol,
                        OutEndpointAddr,
                        &AuthTx_Data,
                        &AuthTx_len,
                        0,
                        &USB_Status
                        );
                //
                // Receive Password
                //
                while(1) {
                    Status = UsbProtocol->UsbBulkTransfer (
                            UsbProtocol,
                            InEndpointAddr,
                            AuthRx_Data,
                            &AuthRx_len,
                            0,
                            &USB_Status
                            );
                    if(AuthRx_len == 32)
                        break;
                    else
                        AuthRx_len = 32;
                }
                //
                // Check validity
                //
                Print(L"Host Auth: ");
                for(Index = 0; Index < 32; Index++) {
                    Print(L"%02x ", Auth_Data[AuthTx_Data][Index]);                
                }
                Print(L"\n");
                Print(L"URD Auth: ");
                for(Index = 0; Index < 32; Index++) {
                    Print(L"%02x ", AuthRx_Data[Index]);
                }
                Print(L"\n");
                
                for(Index = 0; Index < 32; Index++) {
                    if(AuthRx_Data[Index] != Auth_Data[(RNG % 32)][Index]) {
                        Print(L"\nURD is not valid. System will be shut down.\n\n");
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
                    }
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
                
                   Print(L"Send hello signal, Endpoint=0x%02x, Status:%r\n", OutEndpointAddr, Status);
                   Print(L"signal length=%d\n", Hello_len);
                   Print(L"signal=");
                   for(Index=0;Index<Hello_len;Index++) {
                   Print(L"%02x ", Hello_Data[Index]);
                   }
                   Print(L"\n");
                 

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
                
                   Print(L"Nonce length=%d\n", Nonce_len);
                   Print(L"Nonce=");
                   for(Index=0;Index<Nonce_len;Index++) {
                   Print(L"%02x ", Nonce_Data[Index]);
                   }
                   Print(L"\n");
                 

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
