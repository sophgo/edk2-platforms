#ifndef __NS16550_H__
#define __NS16550_H__


#include <Base.h>
#include <Uefi.h>
#include <IndustryStandard/Pci.h>
#include <Library/SerialPortLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/Cpu.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>


#define UART3_BASEADDRESS    0x7030003000
#define UART3_BAUD_38400     38400
#define UART3_BAUD_9600      9600
#define UART3_BAUD_115200    115200
#define UART3_PCLK           500000000
#define UART3_REG_IO_WIDTH   4
#define UART3_REG_SHIFT      2
#define UART3_BAUDRATE       UART3_BAUD_9600


typedef struct ns16550 {
	UINTN base;
	unsigned int baudrate;
	unsigned int pclk;
	unsigned int reg_io_width;
	unsigned int reg_shift;
}UART_NS16550_DEV;

UINTN
EFIAPI
SophgoSerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  );

UINTN
EFIAPI
SophgoSerialPortRead (
  OUT UINT8  *Buffer,
  IN  UINTN  NumberOfBytes
  );

BOOLEAN
EFIAPI
SophgoSerialPortPoll (
  VOID
  );

#endif
