# 1 Overview
To perform stress testing on the read/write function of an SD card driver, the method in the current file can be employed.

# 2 HowTo
## 2.1 Generate The Test File on Linux
Use the command below to create the test file for read/write test:
```bash
sudo dd if=/dev/random of=testbin bs=1M count=20 status=progress
```  
This command generates a 20 MB file named `testbin`. Adjust the `count` parameter to modify the file size for varying test loads.

## 2.2 Create EDK2-Compatible Test Script
Save the following code as `rwtest.nsh`, which can be executed by the EDK2 shell:

```nsh
echo -off
if %1 == "" then
  echo "Error: Missing loop count parameter!"
  echo "Usage: %0 <loop_count>"
  exit /b
endif

for %i run (1 %1)
  echo "Loop: %i/%1"
  echo "Start cp after 5s"
  stall 5000000
  cp testbin cpbin

  echo "Start compare 2 files after 5s"
  stall 5000000
  comp testbin cpbin

  echo "Start rm after 5s"
  stall 5000000
  rm cpbin
endfor

echo "Test completed!"
```

## 2.3 Prepare the SD Card
Copy both `testbin` and `rwtest.nsh` to the FAT partition of the SD card configured with the EDK2 boot environment.

## 2.4 Execute the Script in EDK2 Shell environment
Boot into the EDK2 Shell environment and execute the script:  
- Navigate to the test file directory (e.g., `fs0:` followed by `cd <directory>`).
- Run the script with the desired loop count (e.g., `rwtest.nsh 200` for 200 iterations).

By modifying the first parameter passed to the test script, the number of test loops can be altered.

## 2.5 Test Workflow
The testing content of this script includes:
1. Copying `testbin` to `cpbin`.
2. Verifying file integrity by comparing `testbin` and `cpbin`.
3. Deleting `cpbin`.
4. Repeating steps 1–3 until the specified loop count is reached.

If the test fails, analyze the logs printed during execution to identify issues in the read/write operations.