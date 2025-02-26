# Requirements

## `inf` File:
- The `[Packages]` section must include `Silicon/Sophgo/Sophgo.dec`.
- The `[LibraryClasses]` section must include `EfuseLib`.

## `c` File:
- The header file `#include <Library/EfuseLib.h>` must be included in the source code.

# Code Details

Typically, the following two functions provided in the header file are used to perform read and write operations on the efuse device:

```c
EFI_STATUS
EFIAPI
EfuseWriteBytes (
  IN   UINT32    BusNum,
  IN   UINT32    Offset,
  IN   UINT32    Count,
  IN   VOID      *Buffer
  );

EFI_STATUS
EFIAPI
EfuseReadBytes (
  IN   UINT32    BusNum,
  IN   UINT32    Offset,
  IN   UINT32    Count,
  OUT  VOID      *Buffer
  );
```

These functions operate on a byte-level basis. However, in certain scenarios, operations need to be performed on the efuse device using cells as the unit. To achieve this, a conversion is required. 

If the size of a cell is 4 bytes, the efuse command considers the memory arrangement of the efuse device to be as follows:

```
|| Cell Index || Byte Index (little-endian)                     ||
|| ---------- || ---------- | --------- | --------- | --------- ||
|| 0          || 0          | 1         | 2         | 3         ||
|| 1          || 4          | 5         | 6         | 7         ||
|| ... ...    ||            |           |           |           ||
|| N          || 4 * N      | 4 * N + 1 | 4 * N + 2 | 4 * N + 3 ||
```

Based on the memory layout above, if you want to write the value `0x1000` into the Cell with index `88` (index starts from 0), the efuse device memory layout should appear as follows after the operation:

```
|| Cell Index || Byte Index (little-endian), value                         ||
|| ---------- || ------------ | ------------ | ------------ | ------------ ||
|| 0          || 0, val: **   | 1, val: **   | 2, val: **   | 3, val: **   ||
|| ... ...    ||              |              |              |              ||
|| 88         || 352, val: 00 | 353, val: 10 | 354, val: 00 | 355, val: 00 ||
```

It is important to note that efuse data is stored in **little-endian** format.

To write the value `0x1000` into Cell `88` of efuse1, the following code can be used:

```c
UINT32 BusNum = 1;
UINT32 Offset = 352;
UINT32 Count  = 4;
UINT8  Buff[] = {0x00, 0x10, 0x00, 0x00};
EFI_STATUS Status;

Status = EfuseWriteBytes (BusNum, Offset, Count, (VOID *)(Buff));
```

To read the entire content of Cell `88` from efuse1, the following code can be used:

```c
UINT32 BusNum = 1;
UINT32 Offset = 352;
UINT32 Count  = 4;
UINT8  Buff[4];
UINT32 Data   = 0;
EFI_STATUS Status;

gBS->SetMem (Buff, sizeof (Buff), 0);

Status = EfuseReadBytes (BusNum, Offset, Count, (VOID *)(Buff));
Data = (UINT32)Buff[0]
     | ((UINT32)Buff[1] << 8)
     | ((UINT32)Buff[2] << 16)
     | ((UINT32)Buff[3] << 24);
```

The above example demonstrates how to read and write efuse data at the byte level while considering the little-endian format.