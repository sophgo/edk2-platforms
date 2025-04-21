#ifndef _BMC_OEM_CONFIG_H_
#define _BMC_OEM_CONFIG_H_


#include <Guid/SmBios.h>

#include <IndustryStandard/SmBios.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/IpmiLib.h>
#include <Library/SmbiosInformationLib.h>
#include <Library/IpmiCommandLib.h>

#define SMBIOS_OEM_IPMI_CMD_MAX_LEN             128
#define SMBIOS_OEM_IPMI_NETFN                   0x2E
#define SMBIOS_OEM_BIOS_FW_VERSION_CMD          0x01
#define SMBIOS_OEM_CPU_IPMI_CMD                 0x03
#define SMBIOS_OEM_DDR_IPMI_CMD                 0x05
#define CPU_MANUFACTURER_MAX_LEN                16
#define CPU_MANUFACTURER                        "SOPHGO"
#define CPU_BRAND_NAME_MAX_LEN                  16
#define CPU_BRAND_NAME                          "SG2044"


EFI_STATUS
EFIAPI
SendBiosFmVersionToBmc (
  VOID
  );

EFI_STATUS
EFIAPI
SendCpuInfoToBmc (
  VOID
  );

EFI_STATUS
EFIAPI
SendDdrInfoToBmc (
  VOID
  );

VOID
EFIAPI
SendSmbiosOemToBmc (
  VOID
  );

#endif
