/** @file
Some functions about print password input interface.

Copyright (c) 2024, Sophgo. All rights reserved.

SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Guid/MdeModuleHii.h>
#include <Guid/GlobalVariable.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiString.h>
#include <Library/CustomizedDisplayLib/Colors.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CustomizedDisplayLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include "FrontPageCustomizedUiSupport.h"
#include <Library/PasswordConfigData.h>
#include <Library/PasswordRead.h>
#define SPACE_BUFFER_SIZE      1000

CHAR16 *mSpaceBuffer;
EFI_SCREEN_DESCRIPTOR         gScreenDimensions;
//
// Print Functions
//

/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

**/
VOID
SetUnicodeMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN CHAR16 Value
  )
{
  CHAR16  *Ptr;

  Ptr = Buffer;
  while ((Size--)  != 0) {
    *(Ptr++) = Value;
  }
}
/**
  The internal function prints to the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
  protocol instance.

  @param Width           Width of string to be print.
  @param Column          The position of the output string.
  @param Row             The position of the output string.
  @param Out             The EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL instance.
  @param Fmt             The format string.
  @param Args            The additional argument for the variables in the format string.

  @return Number of Unicode character printed.

**/
UINTN
PrintInternal (
  IN UINTN                            Width,
  IN UINTN                            Column,
  IN UINTN                            Row,
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Out,
  IN CHAR16                           *Fmt,
  IN VA_LIST                          Args
  )
{
  CHAR16  *Buffer;
  CHAR16  *BackupBuffer;
  UINTN   Index;
  UINTN   PreviousIndex;
  UINTN   Count;
  UINTN   TotalCount;
  UINTN   PrintWidth;
  UINTN   CharWidth;

  mSpaceBuffer = AllocatePool ((SPACE_BUFFER_SIZE + 1) * sizeof (CHAR16));
  ASSERT (mSpaceBuffer != NULL);
  SetUnicodeMem (mSpaceBuffer, SPACE_BUFFER_SIZE, L' ');
  mSpaceBuffer[SPACE_BUFFER_SIZE] = L'\0';

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer        = AllocateZeroPool (0x10000);
  BackupBuffer  = AllocateZeroPool (0x10000);
  ASSERT (Buffer);
  ASSERT (BackupBuffer);

  if (Column != (UINTN) -1) {
    Out->SetCursorPosition (Out, Column, Row);
  }

  UnicodeVSPrint (Buffer, 0x10000, Fmt, Args);

  Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;

  Out->SetAttribute (Out, Out->Mode->Attribute);

  Index         = 0;
  PreviousIndex = 0;
  Count         = 0;
  TotalCount    = 0;
  PrintWidth    = 0;
  CharWidth     = 1;

  do {
    for (; (Buffer[Index] != NARROW_CHAR) && (Buffer[Index] != WIDE_CHAR) && (Buffer[Index] != 0); Index++) {
      BackupBuffer[Index] = Buffer[Index];
    }

    if (Buffer[Index] == 0) {
      break;
    }

    //
    // Print this out, we are about to switch widths
    //
    Out->OutputString (Out, &BackupBuffer[PreviousIndex]);
    Count = StrLen (&BackupBuffer[PreviousIndex]);
    PrintWidth += Count * CharWidth;
    TotalCount += Count;

    //
    // Preserve the current index + 1, since this is where we will start printing from next
    //
    PreviousIndex = Index + 1;

    //
    // We are at a narrow or wide character directive.  Set attributes and strip it and print it
    //
    if (Buffer[Index] == NARROW_CHAR) {
      //
      // Preserve bits 0 - 6 and zero out the rest
      //
      Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;
      Out->SetAttribute (Out, Out->Mode->Attribute);
      CharWidth = 1;
    } else {
      //
      // Must be wide, set bit 7 ON
      //
      Out->Mode->Attribute = Out->Mode->Attribute | EFI_WIDE_ATTRIBUTE;
      Out->SetAttribute (Out, Out->Mode->Attribute);
      CharWidth = 2;
    }

    Index++;

  } while (Buffer[Index] != 0);

  //
  // We hit the end of the string - print it
  //
  Out->OutputString (Out, &BackupBuffer[PreviousIndex]);
  Count = StrLen (&BackupBuffer[PreviousIndex]);
  PrintWidth += Count * CharWidth;
  TotalCount += Count;
  if (PrintWidth < Width) {
    Out->Mode->Attribute = Out->Mode->Attribute & 0x7f;
    Out->SetAttribute (Out, Out->Mode->Attribute);
    Out->OutputString (Out, &mSpaceBuffer[SPACE_BUFFER_SIZE - Width + PrintWidth]);
  }

  FreePool (Buffer);
  FreePool (BackupBuffer);
  return TotalCount;
}

/**
  Prints a formatted unicode string to the default console, at
  the supplied cursor position.

  @param  Width      Width of String to be printed.
  @param  Column     The cursor position to print the string at.
  @param  Row        The cursor position to print the string at.
  @param  Fmt        Format string.
  @param  ...        Variable argument list for format string.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintAt (
  IN UINTN     Width,
  IN UINTN     Column,
  IN UINTN     Row,
  IN CHAR16    *Fmt,
  ...
  )
{
  VA_LIST Args;
  UINTN   LengthOfPrinted;

  VA_START (Args, Fmt);
  LengthOfPrinted = PrintInternal (Width, Column, Row, gST->ConOut, Fmt, Args);
  VA_END (Args);
  return LengthOfPrinted;
}


/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintStringAt (
  IN UINTN   Column,
  IN UINTN   Row,
  IN CHAR16  *String
  )
{
  return PrintAt (0, Column, Row, L"%s", String);
}

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  String     String pointer.
  @param  Width      Width for String.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintStringAtWithWidth (
  IN UINTN   Column,
  IN UINTN   Row,
  IN CHAR16  *String,
  IN UINTN   Width
  )
{
  return PrintAt (Width, Column, Row, L"%s", String);
}

/**
  Prints a character to the default console, at
  the supplied cursor position, using L"%c" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
EFIAPI
PrintCharAt (
  IN UINTN  Column,
  IN UINTN  Row,
  CHAR16    Character
  )
{
  return PrintAt (0, Column, Row, L"%c", Character);
}

/**
  Clear retangle with specified text attribute.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
EFIAPI
ClearLines (
  IN UINTN  LeftColumn,
  IN UINTN  RightColumn,
  IN UINTN  TopRow,
  IN UINTN  BottomRow,
  IN UINTN  TextAttribute
  )
{
  CHAR16  *Buffer;
  UINTN   Row;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  //
  // Set foreground and background as defined
  //
  gST->ConOut->SetAttribute (gST->ConOut, TextAttribute);

  //
  // Much faster to buffer the long string instead of print it a character at a time
  //
  SetUnicodeMem (Buffer, RightColumn - LeftColumn, L' ');

  //
  // Clear the desired area with the appropriate foreground/background
  //
  for (Row = TopRow; Row <= BottomRow; Row++) {
    PrintStringAt (LeftColumn, Row, Buffer);
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, LeftColumn, TopRow);

  FreePool (Buffer);
}

/**
  Get OEM/Vendor specific popup attribute colors.

  @retval  Byte code color setting for popup inverse color.
**/
UINT8
EFIAPI
GetPopupInverseColor (
  VOID
  )
{
  return POPUP_INVERSE_TEXT | POPUP_INVERSE_BACKGROUND;
}

/**
  Get OEM/Vendor specific popup attribute colors.

  @retval  Byte code color setting for popup color.
**/
UINT8
EFIAPI
GetPopupColor (
  VOID
  )
{
  return POPUP_TEXT | POPUP_BACKGROUND;
}
/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  If String is NULL, then ASSERT ().

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
GetStringWidth (
  IN CHAR16               *String
  )
{
  UINTN Index;
  UINTN Count;
  UINTN IncrementValue;

  ASSERT (String != NULL);
  if (String == NULL) {
    return 0;
  }

  Index           = 0;
  Count           = 0;
  IncrementValue  = 1;

  do {
    //
    // Advance to the null-terminator or to the first width directive
    //
    for (;
         (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0);
         Index++, Count = Count + IncrementValue
        )
      ;

    //
    // We hit the null-terminator, we now have a count
    //
    if (String[Index] == 0) {
      break;
    }
    //
    // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
    // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
    //
    if (String[Index] == NARROW_CHAR) {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 1;
    } else {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 2;
    }
  } while (String[Index] != 0);

  //
  // Increment by one to include the null-terminator in the size
  //
  Count++;

  return Count * sizeof (CHAR16);
}


/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param RequestedWidth  The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param Marker          The variable argument list for the list of string to be printed.

**/
VOID
CreateSharedPopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  IN  VA_LIST                     Marker
  )
{
  UINTN   Index;
  UINTN   Count;
  CHAR16  Character;
  UINTN   Start;
  UINTN   End;
  UINTN   Top;
  UINTN   Bottom;
  CHAR16  *String;
  UINTN   DimensionsWidth;
  UINTN   DimensionsHeight;

  DimensionsWidth   = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight  = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());

  if ((RequestedWidth + 2) > DimensionsWidth) {
    RequestedWidth = DimensionsWidth - 2;
  }

  //
  // Subtract the PopUp width from total Columns, allow for one space extra on
  // each end plus a border.
  //
  Start     = (DimensionsWidth - RequestedWidth - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  End       = Start + RequestedWidth + 1;

  Top       = ((DimensionsHeight - NumberOfLines - 2) / 2) + gScreenDimensions.TopRow - 1;
  Bottom    = Top + NumberOfLines + 2;

  Character = BOXDRAW_DOWN_RIGHT;
  PrintCharAt (Start, Top, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_DOWN_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  Character = BOXDRAW_VERTICAL;

  Count = 0;
  for (Index = Top; Index + 2 < Bottom; Index++, Count++) {
    String = VA_ARG (Marker, CHAR16*);

    //
    // This will clear the background of the line - we never know who might have been
    // here before us.  This differs from the next clear in that it used the non-reverse
    // video for normal printing.
    //
    if (GetStringWidth (String) / 2 > 1) {
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
    }

    //
    // Passing in a space results in the assumption that this is where typing will occur
    //
    if (String[0] == L' ') {
      ClearLines (Start + 1, End - 1, Index + 1, Index + 1, GetPopupInverseColor ());
    }

    //
    // Passing in a NULL results in a blank space
    //
    if (String[0] == CHAR_NULL) {
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
    }

    PrintStringAt (
      ((DimensionsWidth - GetStringWidth (String) / 2) / 2) + gScreenDimensions.LeftColumn + 1,
      Index + 1,
      String
      );
    gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
    PrintCharAt (Start, Index + 1, Character);
    PrintCharAt (End - 1, Index + 1, Character);
  }

  Character = BOXDRAW_UP_RIGHT;
  PrintCharAt (Start, Bottom - 1, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_UP_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
}

/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param RequestedWidth  The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param ...             A series of text strings that displayed in the pop-up.

**/
VOID
EFIAPI
CreateMultiStringPopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, NumberOfLines);

  CreateSharedPopUp (RequestedWidth, NumberOfLines, Marker);

  VA_END (Marker);
}


/**
  Wait for a key to be pressed by user.

  @param Key         The key which is pressed by user.

  @retval EFI_SUCCESS The function always completed successfully.

**/
EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY  *Key
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  while (TRUE) {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
    if (!EFI_ERROR (Status)) {
      break;
    }

    if (Status != EFI_NOT_READY) {
      continue;
    }

    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
  }

  return Status;
}
/**
  Create popup window. It will replace CreateDialog().

  This function draws OEM/Vendor specific pop up windows.

  @param[out]  Key    User Input Key
  @param       ...    String to be shown in Popup. The variable argument list is terminated by a NULL.

**/
VOID
EFIAPI
CreateDialog (
  OUT EFI_INPUT_KEY  *Key,        OPTIONAL
  ...
  )
{
  VA_LIST       Marker;
  EFI_INPUT_KEY KeyValue;
  EFI_STATUS    Status;
  UINTN         LargestString;
  UINTN         LineNum;
  UINTN         Index;
  UINTN         Count;
  CHAR16        Character;
  UINTN         Start;
  UINTN         End;
  UINTN         Top;
  UINTN         Bottom;
  CHAR16        *String;
  UINTN         DimensionsWidth;
  UINTN         DimensionsHeight;
  UINTN         CurrentAttribute;
  BOOLEAN       CursorVisible;

  //
  // If screen dimension info is not ready, get it from console.
  //
  if (gScreenDimensions.RightColumn == 0 || gScreenDimensions.BottomRow == 0) {
    ZeroMem (&gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
    gST->ConOut->QueryMode (
                   gST->ConOut,
                   gST->ConOut->Mode->Mode,
                   &gScreenDimensions.RightColumn,
                   &gScreenDimensions.BottomRow
                   );
  }

  DimensionsWidth   = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight  = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  LargestString = 0;
  LineNum       = 0;
  VA_START (Marker, Key);
  while  ((String = VA_ARG (Marker, CHAR16 *)) != NULL) {
    LineNum ++;

    if ((GetStringWidth (String) / 2) > LargestString) {
      LargestString = (GetStringWidth (String) / 2);
    }
  }
  VA_END (Marker);

  if ((LargestString + 2) > DimensionsWidth) {
    LargestString = DimensionsWidth - 2;
  }

  CurrentAttribute  = gST->ConOut->Mode->Attribute;
  CursorVisible     = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());

  //
  // Subtract the PopUp width from total Columns, allow for one space extra on
  // each end plus a border.
  //
  Start     = (DimensionsWidth - LargestString - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  End       = Start + LargestString + 1;

  Top       = ((DimensionsHeight - LineNum - 2) / 2) + gScreenDimensions.TopRow - 1;
  Bottom    = Top + LineNum + 2;


  Character = BOXDRAW_DOWN_RIGHT;
  PrintCharAt (Start, Top, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_DOWN_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  Character = BOXDRAW_VERTICAL;

  Count = 0;
  VA_START (Marker, Key);
  for (Index = Top; Index + 2 < Bottom; Index++, Count++) {
    String = VA_ARG (Marker, CHAR16*);

    if (String[0] == CHAR_NULL) {
      //
      // Passing in a NULL results in a blank space
      //
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
    } else if (String[0] == L' ') {
      //
      // Passing in a space results in the assumption that this is where typing will occur
      //
      ClearLines (Start + 1, End - 1, Index + 1, Index + 1, POPUP_INVERSE_TEXT | POPUP_INVERSE_BACKGROUND);
      PrintStringAt (
        ((DimensionsWidth - GetStringWidth (String) / 2) / 2) + gScreenDimensions.LeftColumn + 1,
        Index + 1,
        String + 1
        );
    } else {
      //
      // This will clear the background of the line - we never know who might have been
      // here before us.  This differs from the next clear in that it used the non-reverse
      // video for normal printing.
      //
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
      PrintStringAt (
        ((DimensionsWidth - GetStringWidth (String) / 2) / 2) + gScreenDimensions.LeftColumn + 1,
        Index + 1,
        String
        );
    }

    gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
    PrintCharAt (Start, Index + 1, Character);
    PrintCharAt (End - 1, Index + 1, Character);
  }
  VA_END (Marker);

  Character = BOXDRAW_UP_RIGHT;
  PrintCharAt (Start, Bottom - 1, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN) -1, Character);
  }

  Character = BOXDRAW_UP_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN) -1, Character);

  if (Key != NULL) {
    Status = WaitForKeyStroke (&KeyValue);
    ASSERT_EFI_ERROR (Status);
    CopyMem (Key, &KeyValue, sizeof (EFI_INPUT_KEY));
  }

  gST->ConOut->SetAttribute (gST->ConOut, CurrentAttribute);
  gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
}

/**
  Get string or password input from user.

  @param  Prompt            The prompt string shown on popup window.
  @param  StringPtr         Old user input and destination for use input string.

  @retval EFI_SUCCESS       If string input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
ReadString (
  IN     CHAR16                      *Prompt,
  IN OUT CHAR16                      *StringPtr
  )
{
  EFI_STATUS              Status;
  EFI_INPUT_KEY           Key;
  CHAR16                  NullCharacter;
  UINTN                   ScreenSize;
  CHAR16                  Space[2];
  CHAR16                  KeyPad[2];
  CHAR16                  *TempString;
  CHAR16                  *BufferedString;
  UINTN                   Index;
  UINTN                   Index2;
  UINTN                   Count;
  UINTN                   Start;
  UINTN                   Top;
  UINTN                   DimensionsWidth;
  UINTN                   DimensionsHeight;
  UINTN                   CurrentCursor;
  BOOLEAN                 CursorVisible;
  UINTN                   Minimum;
  UINTN                   Maximum;
  BOOLEAN                 IsPassword;
  UINTN                   MaxLen;

  //
  // If screen dimension info is not ready, get it from console.
  //
  if (gScreenDimensions.RightColumn == 0 || gScreenDimensions.BottomRow == 0) {
    ZeroMem (&gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
    gST->ConOut->QueryMode (
                   gST->ConOut,
                   gST->ConOut->Mode->Mode,
                   &gScreenDimensions.RightColumn,
                   &gScreenDimensions.BottomRow
                   );
  }

  DimensionsWidth  = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  NullCharacter    = CHAR_NULL;
  ScreenSize       = GetStringWidth (Prompt) / sizeof (CHAR16);
  Space[0]         = L' ';
  Space[1]         = CHAR_NULL;

  Minimum = 0;
  //Maximum = 0xf;
  Maximum = PASSWD_MAXLEN_INPUT;
  IsPassword = TRUE;

  MaxLen = Maximum + 1;
  TempString = AllocateZeroPool (MaxLen * sizeof (CHAR16));
  ASSERT (TempString);

  if (ScreenSize < (Maximum + 1)) {
    ScreenSize = Maximum + 1;
  }

  if ((ScreenSize + 2) > DimensionsWidth) {
    ScreenSize = DimensionsWidth - 2;
  }

  BufferedString = AllocateZeroPool (ScreenSize * 2);
  ASSERT (BufferedString);

  Start = (DimensionsWidth - ScreenSize - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  Top   = ((DimensionsHeight - 6) / 2) + gScreenDimensions.TopRow - 1;

  //
  // Display prompt for string
  //
  CreateMultiStringPopUp (ScreenSize, 4, &NullCharacter, Prompt, Space, &NullCharacter);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));

  CursorVisible = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  CurrentCursor = GetStringWidth (StringPtr) / 2 - 1;
  if (CurrentCursor != 0) {
    //
    // Show the string which has beed saved before.
    //
    SetUnicodeMem (BufferedString, ScreenSize - 1, L' ');
    PrintStringAt (Start + 1, Top + 3, BufferedString);

    if ((GetStringWidth (StringPtr) / 2) > (DimensionsWidth - 2)) {
      Index = (GetStringWidth (StringPtr) / 2) - DimensionsWidth + 2;
    } else {
      Index = 0;
    }

    if (IsPassword) {
      gST->ConOut->SetCursorPosition (gST->ConOut, Start + 1, Top + 3);
    }

    for (Count = 0; Index + 1 < GetStringWidth (StringPtr) / 2; Index++, Count++) {
      BufferedString[Count] = StringPtr[Index];

      if (IsPassword) {
        PrintCharAt ((UINTN)-1, (UINTN)-1, L'*');
      }
    }

    if (!IsPassword) {
      PrintStringAt (Start + 1, Top + 3, BufferedString);
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, Start + GetStringWidth (StringPtr) / 2, Top + 3);
  }

  do {
    Status = WaitForKeyStroke (&Key);
    ASSERT_EFI_ERROR (Status);

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
    switch (Key.UnicodeChar) {
    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
        if (CurrentCursor > 0) {
          CurrentCursor--;
        }
        break;

      case SCAN_RIGHT:
        if (CurrentCursor < (GetStringWidth (StringPtr) / 2 - 1)) {
          CurrentCursor++;
        }
        break;

      case SCAN_ESC:
        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;

       case SCAN_DELETE:
        for (Index = CurrentCursor; StringPtr[Index] != CHAR_NULL; Index++) {
          StringPtr[Index] = StringPtr[Index + 1];
          PrintCharAt (Start + Index + 1, Top + 3, IsPassword && StringPtr[Index] != CHAR_NULL? L'*' : StringPtr[Index]);
        }
        break;

      default:
        break;
      }

      break;

    case CHAR_CARRIAGE_RETURN:
      if (GetStringWidth (StringPtr) >= ((Minimum + 1) * sizeof (CHAR16))) {

        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_SUCCESS;
      } else {
        //
        // Simply create a popup to tell the user that they had typed in too few characters.
        // To save code space, we can then treat this as an error and return back to the menu.
        //
        do {
          CreateDialog (&Key, &NullCharacter, L"Please enter enough characters", L"Press ENTER to continue", &NullCharacter, NULL);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;
      }


    case CHAR_BACKSPACE:
      if (StringPtr[0] != CHAR_NULL && CurrentCursor != 0) {
        for (Index = 0; Index < CurrentCursor - 1; Index++) {
          TempString[Index] = StringPtr[Index];
        }
        Count = GetStringWidth (StringPtr) / 2 - 1;
        if (Count >= CurrentCursor) {
          for (Index = CurrentCursor - 1, Index2 = CurrentCursor; Index2 < Count; Index++, Index2++) {
            TempString[Index] = StringPtr[Index2];
          }
          TempString[Index] = CHAR_NULL;
        }
        //
        // Effectively truncate string by 1 character
        //
        StrCpyS (StringPtr, MaxLen, TempString);
        CurrentCursor --;
      }

    default:
      //
      // If it is the beginning of the string, don't worry about checking maximum limits
      //
      if ((StringPtr[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        StrnCpyS (StringPtr, MaxLen, &Key.UnicodeChar, 1);
        CurrentCursor++;
      } else if ((GetStringWidth (StringPtr) < ((Maximum + 1) * sizeof (CHAR16))) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        KeyPad[0] = Key.UnicodeChar;
        KeyPad[1] = CHAR_NULL;
        Count = GetStringWidth (StringPtr) / 2 - 1;
        if (CurrentCursor < Count) {
          for (Index = 0; Index < CurrentCursor; Index++) {
            TempString[Index] = StringPtr[Index];
          }
      TempString[Index] = CHAR_NULL;
          StrCatS (TempString, MaxLen, KeyPad);
          StrCatS (TempString, MaxLen, StringPtr + CurrentCursor);
          StrCpyS (StringPtr, MaxLen, TempString);
        } else {
          StrCatS (StringPtr, MaxLen, KeyPad);
        }
        CurrentCursor++;
      }

      //
      // If the width of the input string is now larger than the screen, we nee to
      // adjust the index to start printing portions of the string
      //
      SetUnicodeMem (BufferedString, ScreenSize - 1, L' ');
      PrintStringAt (Start + 1, Top + 3, BufferedString);

      if ((GetStringWidth (StringPtr) / 2) > (DimensionsWidth - 2)) {
        Index = (GetStringWidth (StringPtr) / 2) - DimensionsWidth + 2;
      } else {
        Index = 0;
      }

      if (IsPassword) {
        gST->ConOut->SetCursorPosition (gST->ConOut, Start + 1, Top + 3);
      }

      for (Count = 0; Index + 1 < GetStringWidth (StringPtr) / 2; Index++, Count++) {
        BufferedString[Count] = StringPtr[Index];

        if (IsPassword) {
          PrintCharAt ((UINTN)-1, (UINTN)-1, L'*');
        }
      }

      if (!IsPassword) {
        PrintStringAt (Start + 1, Top + 3, BufferedString);
      }
      break;
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, Start + CurrentCursor + 1, Top + 3);
  } while (TRUE);

}
