/** @file
  Features in MSR_IA32_FEATURE_CONTROL register.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuCommonFeatures.h"

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
VOID *
EFIAPI
FeatureControlGetConfigData (
  IN UINTN               NumberOfProcessors
  )
{
  VOID          *ConfigData;

  ConfigData = AllocateZeroPool (sizeof (MSR_IA32_FEATURE_CONTROL_REGISTER) * NumberOfProcessors);
  ASSERT (ConfigData != NULL);
  return ConfigData;
}

/**
  Detects if VMX feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     VMX feature is supported.
  @retval FALSE    VMX feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
VmxSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  MsrRegister[ProcessorNumber].Uint64 = AsmReadMsr64 (MSR_IA32_FEATURE_CONTROL);
  return (CpuInfo->CpuIdVersionInfoEcx.Bits.VMX == 1);
}

/**
  Initializes VMX inside SMX feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the VMX inside SMX feature must be enabled.
                               If FALSE, then the VMX inside SMX feature must be disabled.

  @retval RETURN_SUCCESS       VMX inside SMX feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
VmxInsideSmxInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  if (MsrRegister[ProcessorNumber].Bits.Lock == 0) {
    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_FEATURE_CONTROL,
      MSR_IA32_FEATURE_CONTROL_REGISTER,
      Bits.EnableVmxInsideSmx,
      (State) ? 1 : 0
      );
  }
  return RETURN_SUCCESS;
}

/**
  Initializes SENTER feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the SENTER feature must be enabled.
                               If FALSE, then the SENTER feature must be disabled.

  @retval RETURN_SUCCESS       SENTER feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
SenterInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  if (MsrRegister[ProcessorNumber].Bits.Lock == 0) {
    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_FEATURE_CONTROL,
      MSR_IA32_FEATURE_CONTROL_REGISTER,
      Bits.SenterLocalFunctionEnables,
      (State) ? 0x7F : 0
      );

    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_FEATURE_CONTROL,
      MSR_IA32_FEATURE_CONTROL_REGISTER,
      Bits.SenterGlobalEnable,
      (State) ? 1 : 0
      );
  }
  return RETURN_SUCCESS;
}

/**
  Detects if Lock Feature Control Register feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Lock Feature Control Register feature is supported.
  @retval FALSE    Lock Feature Control Register feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
LockFeatureControlRegisterSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  MsrRegister[ProcessorNumber].Uint64 = AsmReadMsr64 (MSR_IA32_FEATURE_CONTROL);
  return TRUE;
}

/**
  Initializes Lock Feature Control Register feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Lock Feature Control Register feature must be enabled.
                               If FALSE, then the Lock Feature Control Register feature must be disabled.

  @retval RETURN_SUCCESS       Lock Feature Control Register feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
LockFeatureControlRegisterInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  if (MsrRegister[ProcessorNumber].Bits.Lock == 0) {
    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_FEATURE_CONTROL,
      MSR_IA32_FEATURE_CONTROL_REGISTER,
      Bits.Lock,
      1
      );
  }
  return RETURN_SUCCESS;
}

/**
  Detects if SMX feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     SMX feature is supported.
  @retval FALSE    SMX feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
SmxSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  MsrRegister[ProcessorNumber].Uint64 = AsmReadMsr64 (MSR_IA32_FEATURE_CONTROL);
  return (CpuInfo->CpuIdVersionInfoEcx.Bits.SMX == 1);
}

/**
  Initializes VMX outside SMX feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the VMX outside SMX feature must be enabled.
                               If FALSE, then the VMX outside SMX feature must be disabled.

  @retval RETURN_SUCCESS       VMX outside SMX feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
VmxOutsideSmxInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  if (MsrRegister[ProcessorNumber].Bits.Lock == 0) {
    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_FEATURE_CONTROL,
      MSR_IA32_FEATURE_CONTROL_REGISTER,
      Bits.EnableVmxOutsideSmx,
      (State) ? 1 : 0
      );
  }
  return RETURN_SUCCESS;
}
