#include "BmcConfigOem.h"

EFI_STATUS
EFIAPI
SendBiosFmVersionToBmc (
  VOID
  )
{
	CHAR16      BiosFmVersion[16];
  CHAR8       AsciiBiosFmVersion[16];
  UINT8       Commanddata[SMBIOS_OEM_IPMI_CMD_MAX_LEN];
  UINT8       Commanddatasize;
  UINT8       Response[20];
  UINT32       Responsesize;
  EFI_STATUS  Status;

	Status = GetBiosFmVersion(BiosFmVersion);

	if (EFI_ERROR(Status)) {
		DEBUG((
			DEBUG_ERROR,
			"%a: GetBiosFmVersion-%r\n",
			__func__,
			Status));
	} else {
    UnicodeStrToAsciiStrS(BiosFmVersion, AsciiBiosFmVersion, StrLen(BiosFmVersion) + 1);

		ZeroMem (Commanddata, 20);
		ZeroMem (Response, 20);
		CopyMem (&Commanddata[0], &AsciiBiosFmVersion[0], AsciiStrLen(AsciiBiosFmVersion));
		Commanddatasize =  AsciiStrLen(AsciiBiosFmVersion);
		Responsesize    = 10;
		Status = IpmiSubmitCommand (
							SMBIOS_OEM_IPMI_NETFN,
							SMBIOS_OEM_BIOS_FW_VERSION_CMD,
							(UINT8 *) &Commanddata[0],
							Commanddatasize,
							(UINT8 *) &Response,
							(UINT32 *) &Responsesize
							);
		if (EFI_ERROR(Status)) {
			DEBUG((
				DEBUG_ERROR,
				"%a: IpmiSubmitCommand-%r\n",
				__func__,
				Status));
		}
  }
  return Status;
}

EFI_STATUS
EFIAPI
SendCpuInfoToBmc (
  VOID
  )
{
	UINT8       Commanddata[SMBIOS_OEM_IPMI_CMD_MAX_LEN];
	CHAR16      CpuSerialNumber[CPU_SERIALNUM_MAX_LEN];
	CHAR16      CpuSpeed;
  UINT8       Commanddatasize;
  UINT8       Response[20];
  UINT32      Responsesize;
	UINT8       Index, CpuManufacturerLen, CpuBrandNameLen;
  EFI_STATUS  Status;

	Index = 0;
	CpuManufacturerLen   = AsciiStrLen(CPU_MANUFACTURER) + 1;
	CpuBrandNameLen      = AsciiStrLen(CPU_BRAND_NAME) + 1;
	ZeroMem(Commanddata,sizeof(Commanddata));

	Status = GetCpuSnAndSpeed(CpuSerialNumber, &CpuSpeed);

	if (EFI_ERROR(Status)) {
		DEBUG((
			DEBUG_ERROR,
			"%a: GetCpuSnAndSpeed-%r\n",
			__func__,
			Status));
	} else {
		//Assembling the protocol package
		Commanddata[Index++] = 0x01;
		Commanddata[Index++] = 0x01;

		AsciiStrCpyS((CHAR8*)&Commanddata[Index], CpuManufacturerLen < CPU_MANUFACTURER_MAX_LEN ? CpuManufacturerLen : CPU_MANUFACTURER_MAX_LEN,CPU_MANUFACTURER);
		Index += CPU_MANUFACTURER_MAX_LEN;

		Commanddata[Index++] = 0x10;//I-cache 64KB x 64core = 4096KB
		Commanddata[Index++] = 0x10;//D-cache 64KB x 64core = 4096KB
		Commanddata[Index++] = 0x10;//L2:16M
		Commanddata[Index++] = 0x40;//L3:64M

		UnicodeStrToAsciiStrS(CpuSerialNumber, (CHAR8 *)&Commanddata[Index], StrLen(CpuSerialNumber) + 1);
		Index += CPU_SERIALNUM_MAX_LEN;

		AsciiStrCpyS((CHAR8*)&Commanddata[Index], CpuBrandNameLen < CPU_BRAND_NAME_MAX_LEN ? CpuBrandNameLen : CPU_BRAND_NAME_MAX_LEN,CPU_BRAND_NAME);
		Index += CPU_BRAND_NAME_MAX_LEN;

		Commanddata[Index++] = (UINT8)(CpuSpeed&0x00FF);
		Commanddata[Index++] = (UINT8)(CpuSpeed >> 8);
		Commanddata[Index++] = 0x40;//64Cores

		Commanddatasize = Index;
		Responsesize    = 10;

		Status = IpmiSubmitCommand (
            SMBIOS_OEM_IPMI_NETFN,           // NetFunction
            SMBIOS_OEM_CPU_IPMI_CMD,     // Command
            (UINT8 *) &Commanddata[0],  // *CommandData
            Commanddatasize,            // CommandDataSize
            (UINT8 *) &Response,        // *ResponseData
            (UINT32 *) &Responsesize     // *ResponseDataSize
            );
		if (EFI_ERROR(Status)) {
			DEBUG((
				DEBUG_ERROR,
				"%a: IpmiSubmitCommand-%r\n",
				__func__,
				Status));
		}
	}

  return Status;
}

EFI_STATUS
EFIAPI
SendDdrInfoToBmc (
  VOID
  )
{
	CHAR16      MemoryManufacturer[MEM_MANUFACTURER_MAX_LEN];
	UINT8       MemoryType;
  UINT32      MemorySize;//MB
	UINT16      MemorySpeed;//MT/s
  UINT8       MemoryRank;
	UINT8       Commanddata[SMBIOS_OEM_IPMI_CMD_MAX_LEN];
  UINT8       Commanddatasize;
  UINT8       Response[20];
  UINT32      Responsesize;
	UINT8       Index;
  EFI_STATUS  Status;

	Status = GetMemoryInfo(MemoryManufacturer, &MemoryType, &MemorySize, &MemoryRank, &MemorySpeed);

	if (EFI_ERROR(Status)) {
		DEBUG((
			DEBUG_ERROR,
			"%a: GetMemoryInfo-%r\n",
			__func__,
			Status));
	} else {
		Index = 0;
		Commanddata[Index++] = 0x10;//Number of particles 16
		Commanddata[Index++] = MemoryType;//memory_type LPDDR5x
		Commanddata[Index++] = (UINT8)(MemorySpeed & 0x00FF);//memory_speed low Byte 8533MT/s
		Commanddata[Index++] = (UINT8)((MemorySpeed >> 8) & 0x00FF);//memory_speed High Byte 8533MT/s
		Commanddata[Index++] = MemoryRank;//memory_rank 2rank/chip
		Commanddata[Index++] = (UINT8)((MemorySize / 1024) & 0x000000FF);
		Commanddata[Index++] = (UINT8)(((MemorySize / 1024) >> 8) & 0x000000FF);//0x0080 128GB
		UnicodeStrToAsciiStrS(MemoryManufacturer, (CHAR8 *)&Commanddata[Index], StrLen(MemoryManufacturer) + 1);
		Index += MEM_MANUFACTURER_MAX_LEN;


		Commanddatasize = Index;
		Responsesize    = 10;

		Status = IpmiSubmitCommand (
							SMBIOS_OEM_IPMI_NETFN,           // Net Function
							SMBIOS_OEM_DDR_IPMI_CMD,     // Command
							(UINT8 *) &Commanddata[0],  // *CommandData
							Commanddatasize,            // CommandDataSize
							(UINT8 *) &Response,        // *ResponseData
							(UINT32 *) &Responsesize     // *ResponseDataSize
							);
		if (EFI_ERROR(Status)) {
			DEBUG((
				DEBUG_ERROR,
				"%a: IpmiSubmitCommand-%r\n",
				__func__,
				Status));
		}
	}

	return Status;
}

/**
  This function divides smbios into several OEM commands and sends them to BMC.

  @param  Event    The event of notify protocol.
  @param  Context  Notify event context.
**/
VOID
EFIAPI
SendSmbiosOemToBmc (
  VOID
  )
{
	DEBUG((DEBUG_INFO, "Set BMC smbios OEM cmd.\n"));
	SendBiosFmVersionToBmc();
	SendCpuInfoToBmc();
	SendDdrInfoToBmc();
}
