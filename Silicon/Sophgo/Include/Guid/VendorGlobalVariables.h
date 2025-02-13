/** @file
  GUID for EFI (NVRAM) Variables.

  Copyright (c) 2025, Sophgo. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
**/

#ifndef __GLOBAL_VARIABLE_GUID_H__
#define __GLOBAL_VARIABLE_GUID_H__

#define EFI_SOPHGO_VARSTORE_VENDOR_GUID { 0x570cf83d, 0x5a8d, 0x4f79, { 0x91, 0xa9, 0xba, 0x82, 0x8f, 0x05, 0x79, 0xf6 } }

extern EFI_GUID  gEfiSophgoGlobalVariableGuid;

//
// Variable name for storing the password toggle status data.
// This NVRAM variable holds a PASSWORD_TOGGLE_DATA structure that records whether
// password checking is enabled, if it is the first time setting, the user privilege level,
// and whether the platform is EVB.
// The variable name must be a Unicode string (CHAR16 array).
//
#define EFI_PASSWORD_TOGGLE_VARIABLE_NAME    L"PassWordToggleData"

//
// Variable name for storing the system hardware and BIOS information data.
// This NVRAM variable holds an INFORMATION_DATA structure that includes details such as
// the processor's maximum speed, cache sizes, BIOS version, BIOS release date, BIOS vendor,
// and other system information.
// The variable name must be a Unicode string (CHAR16 array).
//
#define EFI_INFORMATION_VARIABLE_NAME         L"InformationData"

//
// Variable name for storing the system time data.
// This NVRAM variable holds a TIME_DATA structure that records the current system time,
// including the year, month, day, hour, minute, second, and an additional focus field.
// The variable name must be a Unicode string (CHAR16 array).
//
#define EFI_TIME_VARIABLE_NAME                L"DynamicTimeData"

//
// Variable name for storing the password configuration data.
// This NVRAM variable holds a PASSWORD_CONFIG_DATA structure that includes flags indicating
// whether the user and administrator passwords are enabled, the user privilege level,
// and the actual password strings for both user and administrator.
// The variable name must be a Unicode string (CHAR16 array).
//
#define EFI_PASSWORD_CONFIG_VARIABLE_NAME     L"PasswordConfigSetup"

/**
 * @brief BMC configuration variable name
 *
 * This macro defines the variable name used in UEFI variable storage
 * for storing BMC network configuration information.
 * The stored data includes the BMC IP address, subnet mask, gateway,
 * and DHCP enable status.
 * In UEFI code, this name is used with `GetVariable` and `SetVariable`
 * operations to retrieve or modify the BMC configuration.
 */
#define EFI_BMC_CONFIG_VARIABLE_NAME          L"BMCConfigData"

//
// Variable name for storing the "Reserved Memory Size" NVRAM variable.
// This variable holds a RESERVE_MEMORY_DATA structure, which contains a UINT32 value representing the size of reserved memory in the system.
// The variable name must be a Unicode string (CHAR16 array).
//
#define EFI_RESERVE_MEMORYSIZE_VARIABLE_NAME  L"ReservedMemorySize"
#endif
