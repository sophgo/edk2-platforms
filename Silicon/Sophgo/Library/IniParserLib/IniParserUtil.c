/* inih -- simple .INI file parser

SPDX-License-Identifier: BSD-3-Clause

Copyright (C) 2009-2020, Ben Hoyt

inih is released under the New BSD license (see LICENSE.txt). Go to the project
home page for more info:

https://github.com/benhoyt/inih

*/
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "IniParserUtil.h"

#if !INI_USE_STACK
#if INI_CUSTOM_ALLOCATOR
#include <stddef.h>
VOID *
IniMalloc (
  IN UINTN Size
  );

VOID
IniFree (
  IN VOID *Buffer
  );

VOID *
IniRealloc (
  IN UINTN OldSize,
  IN UINTN NewSize,
  IN VOID  *OldBuffer
  );
#else
#define IniMalloc  AllocatePool
#define IniFree    FreePool
#define IniRealloc ReallocatePool
#endif
#endif

#define MAX_SECTION  50
#define MAX_NAME     50

/**
   Used by IniParseString() to keep track of string parsing state.
**/
typedef struct {
  CONST CHAR8  *Ptr;
  UINTN        NumLeft;
} INI_PARSE_STRING_CONTEXT;

BOOLEAN
IsSpace (
  IN CHAR8 Char
  )
{
  if (Char =='\t'|| Char =='\n'|| Char ==' ') {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Strip whitespace chars off end of given string, in place. Return String.
**/
STATIC
CHAR8 *
IniRstrip (
  IN OUT CHAR8 *String
  )
{
  CHAR8 *Pointer = String + AsciiStrLen (String);

  while (Pointer > String && IsSpace ((UINT8)(*--Pointer))) {
    *Pointer = '\0';
  }

  return String;
}

/**
   Return pointer to first non-whitespace char in given string.
**/
STATIC
CHAR8 *
IniLskip (
  IN OUT CONST CHAR8 *String
  )
{
  while (*String && IsSpace ((UINT8)(*String))) {
    String ++;
  }

  return (CHAR8 *)String;
}

/**
  Locate the first occurance of a character in a string.

  @param  Str Pointer to NULL terminated ASCII string.
  @param  Chr Character to locate.

  @return  NULL or first occurance of Chr in Str.
**/
STATIC
CHAR8 *
IniStrChr (
  IN OUT CHAR8 *String,
  IN     CHAR8 Char
  )
{
  if (String == NULL) {
    return String;
  }

  while (*String != '\0' && *String != Char) {
    ++ String;
  }

  return (*String == Char) ? String : NULL;
}

/**
   Return pointer to first char (of chars) or inline comment in given string,
   or pointer to NUL at end of string if neither found. Inline comment must
   be prefixed by a whitespace character to register as a comment.
**/
STATIC
CHAR8 *
IniFindCharsOrComment (
  IN OUT CONST CHAR8 *String,
  IN           CHAR8 *Chars
  )
{
#if INI_ALLOW_INLINE_COMMENTS
  BOOLEAN WasSpace;
  WasSpace = FALSE;
  while (*String && (!Chars || !IniStrChr (Chars, *String)) &&
		!(WasSpace && IniStrChr (INI_INLINE_COMMENT_PREFIXES, *String))) {
    WasSpace = IsSpace ((UINT8)(*String));
    String ++;
  }
#else
  while (*String && (!Chars || !IniStrChr (Chars, *String))) {
    String ++;
  }
#endif
  return (CHAR8 *)String;
}

/**
  Similar to strncpy, but ensures dest (size bytes) is
  NUL-terminated, and doesn't pad with NULs.
**/
STATIC
CHAR8 *
IniStrncpy0 (
  IN OUT CHAR8       *Dest,
  IN     CONST CHAR8 *Src,
  IN     UINTN       Size
  )
{
  //
  // Could use strncpy internally,
  // but it causes gcc warnings (see issue #91)
  //
  UINTN Index;

  for (Index = 0; Index < Size - 1 && Src[Index]; Index++) {
    Dest[Index] = Src[Index];
  }

  Dest[Index] = '\0';

  return Dest;
}

/**
   See documentation in header file.
**/
INT32
IniParseStream (
  IN INI_READER  Reader,
  IN VOID        *Stream,
  IN INI_HANDLER Handler,
  IN VOID        *User
  )
{
  //
  // Uses a fair bit of stack (use heap instead if you need to)
  //
#if INI_USE_STACK
  CHAR8 Line[INI_MAX_LINE];
  UINTN MaxLine = INI_MAX_LINE;
#else
  CHAR8 *Line;
  UINTN MaxLine = INI_INITIAL_ALLOC;
#endif
#if INI_ALLOW_REALLOC && !INI_USE_STACK
  CHAR8 *NewLine;
  UINTN Offset;
#endif
  CHAR8 Section[MAX_SECTION] = "";
#if INI_ALLOW_MULTILINE
  CHAR8 PrevName[MAX_NAME] = "";
#endif

  CHAR8* Start;
  CHAR8* End;
  CHAR8* Name;
  CHAR8* Value;
  INT32 Lineno = 0;
  INT32 Error = 0;

#if !INI_USE_STACK
  Line = (CHAR8 *)IniMalloc (INI_INITIAL_ALLOC);
  if (!Line) {
    return -2;
  }
#endif

#if INI_HANDLER_LINENO
#define HANDLER(u, s, n, v) Handler(u, s, n, v, lineno)
#else
#define HANDLER(u, s, n, v) Handler(u, s, n, v)
#endif

  //
  // Scan through stream line by line
  //
  while (Reader (Line, (INT32)MaxLine, Stream) != NULL) {
#if INI_ALLOW_REALLOC && !INI_USE_STACK
    Offset = AsciiStrLen (Line);
    while (Offset == MaxLine - 1 && Line[Offset - 1] != '\n') {
      MaxLine *= 2;
      if (MaxLine > INI_MAX_LINE) {
        MaxLine = INI_MAX_LINE;
      }

      NewLine = IniRealloc (Line, MaxLine);
      if (!NewLine) {
        IniFree (Line);
	return -2;
      }

      Line = NewLine;
      if (Reader (Line + Offset, (INT32)(MaxLine - Offset), Stream) == NULL) {
        break;
      }

      if (MaxLine >= INI_MAX_LINE) {
        break;
      }

      Offset += AsciiStrLen (Line + Offset);
    }
#endif

    Lineno ++;

    Start = Line;
#if INI_ALLOW_BOM
    if (Lineno == 1 && (UINT8)Start[0] == 0xEF &&
		  (UINT8)Start[1] == 0xBB &&
		  (UINT8)Start[2] == 0xBF) {
      Start += 3;
    }
#endif
    Start = IniRstrip (IniLskip (Start));

    if (IniStrChr (INI_START_COMMENT_PREFIXES, *Start)) {
      /* Start-of-Line comment */
    }
#if INI_ALLOW_MULTILINE
    else if (*PrevName && *Start && Start > Line) {
#if INI_ALLOW_INLINE_COMMENTS
      End = IniFindCharsOrComment (Start, NULL);
      if (*End) {
        *End = '\0';
      }

      IniRstrip (Start);
#endif
      //
      // Non-blank line with leading whitespace, treat as continuation
      // of previous name's value (as per Python configparser).
      //
      if (!HANDLER (User, Section, PrevName, Start) && !Error) {
        Error = Lineno;
      }
    }
#endif
    else if (*Start == '[') {
      //
      // A "[section]" line
      //
      End = IniFindCharsOrComment (Start + 1, "]");
      if (*End == ']') {
        *End = '\0';
        IniStrncpy0 (Section, Start + 1, sizeof(Section));
#if INI_ALLOW_MULTILINE
        *PrevName = '\0';
#endif

#if INI_CALL_HANDLER_ON_NEW_SECTION
        if (!HANDLER (User, Section, NULL, NULL) && !Error) {
          Error = Lineno;
        }
#endif
      }

      else if (!Error) {
        //
        // No ']' found on section line
        //
        Error = Lineno;
      }
    }

    else if (*Start) {
      //
      // Not a comment, must be a name[=:]value pair
      //
      End = IniFindCharsOrComment(Start, "=:");
      if (*End == '=' || *End == ':') {
        *End = '\0';
        Name = IniRstrip (Start);
        Value = End + 1;
#if INI_ALLOW_INLINE_COMMENTS
        End = IniFindCharsOrComment(Value, NULL);
        if (*End) {
          *End = '\0';
        }
#endif
        Value = IniLskip (Value);
        IniRstrip (Value);

#if INI_ALLOW_MULTILINE
        IniStrncpy0 (PrevName, Name, sizeof(PrevName));
#endif
        //
        // Valid name[=:]value pair found, call Handler
        //
        if (!HANDLER (User, Section, Name, Value) && !Error) {
	  Error = Lineno;
        }
      }

      else if (!Error) {
        //
        // No '=' or ':' found on name[=:]value line
        //
#if INI_ALLOW_NO_VALUE
        *End = '\0';
        Name = IniRstrip (Start);
        if (!HANDLER (User, Section, Name, NULL) && !Error) {
          Error = Lineno;
        }
#else
        Error = Lineno;
#endif
      }
    }

#if INI_STOP_ON_FIRST_ERROR
    if (Error) {
      break;
    }
#endif
  }
#if !INI_USE_STACK
  IniFree (Line);
#endif

  return Error;
}

/**
   See documentation in header file.
**/
INT32
IniParseFile (
  IN FILE        *File,
  IN INI_HANDLER Handler,
  IN VOID        *User
  )
{
  return IniParseStream ((INI_READER)fgets, File, Handler, User);
}

/**
   See documentation in header file.
**/
INT32
IniParse (
  IN CONST CHAR8  *FileName,
  IN INI_HANDLER  Handler,
  IN VOID         *User
  )
{
  FILE  *File;
  INT32 Error;

  File = fopen (FileName, "r");
  if (!File) {
    return -1;
  }

  Error = IniParseFile (File, Handler, User);
  fclose (File);

  return Error;
}

/**
   An INI_READER function to read the next line from a string buffer. This
   is the fgets() equivalent used by IniParseString().
**/
STATIC
CHAR8 *
IniReaderString (
  IN OUT CHAR8  *String,
  IN     UINT32 Number,
  IN     VOID   *Stream
  )
{
  INI_PARSE_STRING_CONTEXT *Ctx = (INI_PARSE_STRING_CONTEXT *)Stream;
  CONST CHAR8              *CtxPtr = Ctx->Ptr;
  UINTN                    CtxNumLeft = Ctx->NumLeft;
  CHAR8                    *Strp = String;
  CHAR8                    Char;

  if (CtxNumLeft == 0 || Number < 2) {
    return NULL;
  }

  while (Number > 1 && CtxNumLeft != 0) {
    Char = *CtxPtr ++;
    CtxNumLeft --;
    *Strp ++ = Char;
    if (Char == '\n') {
      break;
    }

    Number --;
  }

  *Strp = '\0';
  Ctx->Ptr = CtxPtr;
  Ctx->NumLeft = CtxNumLeft;

  return String;
}

/**
  See documentation in header file.
**/
EFIAPI
INT32
IniParseString (
  IN CONST CHAR8  *String,
  IN INI_HANDLER  Handler,
  IN VOID         *User
  )
{
  INI_PARSE_STRING_CONTEXT Ctx;

  Ctx.Ptr = String;
  Ctx.NumLeft = AsciiStrLen (String);

  return IniParseStream ((INI_READER)IniReaderString, &Ctx, Handler, User);
}
