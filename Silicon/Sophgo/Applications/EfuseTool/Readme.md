# Overview

The "`efuse`" command is used in the Shell environment of EDK2 to perform read/write operations on efuse device.

In the Shell command line, you can use the commands "`help efuse`" or "`efuse -h`" to view the particular usage of the "`efuse`" command.

The structure of the efuse command is as follows:

```
efuse [-h] [-v] [-d device] [-o offset] [-f file] [-w value]
      [-r [-s size]] 
   -h           - Show help informations.
   -v           - Show version informations.
   -d device    - Set the index of eFuse device(IP) to be operated on,
                  if not present, device default is 0. 
   -o offset    - Set the offset used by read/write/program, if not
                  present, default offset is 0. 
   -f file      - Set the file that be programmed into efuse.
   -w value     - Write value to the eFuse at specified offset, only 1
                  byte data is supported.
   -r           - Read value from efuse at specified offset.
   -s size      - Used with -r to specify size of byte to read. if not
                  present, default size is 1 byte.
```

For example, read 4 bytes at offset 0x128 from efuse1 device, the command is:
`efuse -d 1 -r -s 4 -o 0x128`

# Cell & byte

The unit of operation for the "`efuse`" command is bytes, but in some cases it is necessary to operate on efuse in "cells", which requires conversion.
If the size of a cell is 4 bytes, the efuse command considers the memory arrangement of the efuse device to be as follows:

```
|| Cell Index || Byte Index (little-endian)                     ||
|| ---------- || ---------- | --------- | --------- | --------- ||
|| 0          || 0          | 1         | 2         | 3         ||
|| 1          || 4          | 5         | 6         | 7         ||
|| ... ...    ||            |           |           |           ||
|| N          || 4 * N      | 4 * N + 1 | 4 * N + 2 | 4 * N + 3 ||
```

According to the above memory layout, if you want to write 0x1000 to Cell number 88 (starting from 0), the memory layout of the efuse device after writing should be as follows:

```
|| Cell Index || Byte Index (little-endian), value                         ||
|| ---------- || ------------ | ------------ | ------------ | ------------ ||
|| 0          || 0, val: **   | 1, val: **   | 2, val: **   | 3, val: **   ||
|| ... ...    ||              |              |              |              ||
|| 88         || 352, val: 00 | 353, val: 10 | 354, val: 00 | 355, val: 00 ||
```

So to complete the operation of writing 0x1000 to Cell 88 in efuse1 device, 0x10 should be written to byte 353, and the corresponding command is:
`efuse -d 1 -w 0x10 -o 353`

# Caution

Writing data to the efuse device is an irreversible operation, so it is important to carefully consider whether the current write operation is correct before performing the write operation!
