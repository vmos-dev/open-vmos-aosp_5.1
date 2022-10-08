/*
 * dspbridge/mpu_api/inc/csl.h
 *
 * DSP-BIOS Bridge driver support functions for TI OMAP processors.
 *
 * Copyright (C) 2007 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published 
 * by the Free Software Foundation version 2.1 of the License.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any kind,
 * whether express or implied; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */


/*
 *  ======== csl.h ========
 *  Purpose:
 *      Platform independent C Standard library functions.
 *
 *  Public Functions:
 *      CSL_AnsiToWchar
 *      CSL_Atoi
 *      CSL_ByteSwap
 *      CSL_Exit
 *      CSL_Init
 *      CSL_NumToAscii
 *      CSL_Strcmp
 *      CSL_Strcpyn
 *      CSL_Strlen
 *      CSL_Strncat
 *      CSL_Strncmp
 *      CSL_Strtok
 *      CSL_Strtokr
 *      CSL_WcharToAnsi
 *      CSL_Wstrlen
 *
 *! Revision History:
 *! ================
 *! 07-Aug-2002 jeh: Added CSL_Strtokr().
 *! 21-Sep-2001 jeh: Added CSL_Strncmp.
 *! 22-Nov-2000 map: Added CSL_Atoi and CSL_Strtok
 *! 19-Nov-2000 kc:  Added CSL_ByteSwap().
 *! 09-Nov-2000 kc:  Added CSL_Strncat.
 *! 29-Oct-1999 kc:  Added CSL_Wstrlen().
 *! 20-Sep-1999 ag:  Added CSL_Wchar2Ansi().
 *! 19-Jan-1998 cr:  Code review cleanup (mostly documentation fixes).
 *! 29-Dec-1997 cr:  Changed CSL_lowercase to CSL_Uppercase, added
 *!                  CSL_AnsiToWchar.
 *! 30-Sep-1997 cr:  Added explicit cdecl descriptors to fxn definitions.
 *! 25-Jun-1997 cr:  Added CSL_strcmp.
 *! 12-Jun-1996 gp:  Created.
 */

#ifndef CSL_
#define CSL_

#ifdef __cplusplus
extern "C" {
#endif

#include <dspapi.h>
#include <host_os.h>

#ifdef UNICODE
/*
 *  ======== CSL_AnsiToWchar ========
 *  Purpose:
 *      Convert an ansi string to a wide char string.
 *  Parameters:
 *      wpstrDest:  wide char buffer pointer.
 *      pstrSource: ansi string buffer pointer.
 *      uSize:      size of wpstrDest buffer.
 *  Returns:
 *      Number of characters copied into wpstrDest, excluding the NULL char.
 *  Requires:
 *      CSL initialized.
 *  Ensures:
 *  Details:
 *      uSize is the number of CHARACTERS in wpstrDest, NOT the number of BYTES
 *      in wpstrDest.  with a WCHAR, the number of characters is bytes/2.
 */
	extern ULONG CSL_AnsiToWchar(OUT WCHAR * pwszDest,
				     IN PSTR pstrSource, ULONG uSize);
#endif

/*
 *  ======== CSL_Atoi ========
 *  Purpose:
 *      Convert a 1 or 2 digit string number into an integer
 *  Parameters:
 *      ptstrSrc:   pointer to string.
 *  Returns:
 *      Integer
 *  Requires:
 *      CSL initialized.
 *      ptstrSrc is a valid string pointer.
 *  Ensures:
 */
	extern INT CSL_Atoi(IN CONST CHAR * ptstrSrc);

/*
 *  ======== CSL_ByteSwap ========
 *  Purpose:
 *      Convert an ansi string to a wide char string.
 *  Parameters:
 *      pstrSrc:    Data to be copied and swapped.
 *      pstrDest:   Output buffer for swapped data.
 *      ulBytes:    Number of bytes to be swapped (should be even).
 *  Returns:
 *  Requires:
 *      CSL initialized.
 *  Ensures:
 *  Details:
 */
	extern VOID CSL_ByteSwap(IN PSTR pstrSrc,
				 OUT PSTR pstrDest, IN ULONG ulBytes);

/*
 *  ======== CSL_Exit ========
 *  Purpose:
 *      Discontinue usage of the CSL module.
 *  Parameters:
 *  Returns:
 *  Requires:
 *      CSL initialized.
 *  Ensures:
 *      Resources acquired in CSL_Init() are freed.
 */
	extern VOID CSL_Exit();

/*
 *  ======== CSL_Init ========
 *  Purpose:
 *      Initialize the CSL module's private state.
 *  Parameters:
 *  Returns:
 *      TRUE if initialized; FALSE if error occured.
 *  Requires:
 *  Ensures:
 *      A requirement for each of the other public CSL functions.
 */
	extern bool CSL_Init();

/*
 *  ======== CSL_NumToAscii ========
 *  Purpose:
 *      Convert a 1 or 2 digit number to a 2 digit string.
 *  Parameters:
 *      pstrNumber: Buffer to store converted string.
 *      dwNum:      Number to convert.
 *  Returns:
 *  Requires:
 *      pstrNumber must be able to hold at least three characters.
 *  Ensures:
 *      pstrNumber will be null terminated.
 */
	extern VOID CSL_NumToAscii(OUT PSTR pstrNumber, IN DWORD dwNum);

/*
 *  ======== CSL_Strcmp ========
 *  Purpose:
 *      Compare 2 ASCII strings.  Works the same way as stdio's strcmp.
 *  Parameters:
 *      pstrStr1:   String 1.
 *      pstrStr2:   String 2.
 *  Returns:
 *      A signed value that gives the results of the comparison:
 *      Zero:   String1 equals String2.
 *      < Zero: String1 is less than String2.
 *      > Zero: String1 is greater than String2.
 *  Requires:
 *      CSL initialized.
 *      pstrStr1 is valid.
 *      pstrStr2 is valid.
 *  Ensures:
 */
	extern LONG CSL_Strcmp(IN CONST PSTR pstrStr1, IN CONST PSTR pstrStr2);

/*
 *  ======== CSL_Strcpyn ========
 *  Purpose:
 *      Safe strcpy function.
 *  Parameters:
 *      pstrDest:   Ptr to destination buffer.
 *      pstrSrc:    Ptr to source buffer.
 *      cMax:       Size of destination buffer.
 *  Returns:
 *      Ptr to destination buffer; or NULL if error.
 *  Requires:
 *      CSL initialized.
 *      pstrDest is valid.
 *      pstrSrc is valid.
 *  Ensures:
 *      Will not copy more than cMax bytes from pstrSrc into pstrDest.
 *      pstrDest will be terminated by a NULL character.
 */
	extern PSTR CSL_Strcpyn(OUT PSTR pstrDest, IN CONST PSTR pstrSrc,
				IN DWORD cMax);

/*
 *  ======== CSL_Strstr ========
 *  Purpose:
 *      Find substring in a stringn.
 *  Parameters:
 *      haystack:   Ptr to string1.
 *      needle:    Ptr to substring to catch.
 *  Returns:
 *      Ptr to first char matching the substring in the main string.
 *  Requires:
 *      CSL initialized.
 *      haystack is valid.
 *      needle is valid.
 *  Ensures:
 */
	extern PSTR CSL_Strstr(IN CONST PSTR haystack, IN CONST PSTR needle);

/*
 *  ======== CSL_Strlen ========
 *  Purpose:
 *      Determine the length of a null terminated ASCI string.
 *  Parameters:
 *      pstrSrc:    pointer to string.
 *  Returns:
 *      String length in bytes.
 *  Requires:
 *      CSL initialized.
 *      pstrSrc is a valid string pointer.
 *  Ensures:
 */
	extern DWORD CSL_Strlen(IN CONST PSTR pstrSrc);

/*
 *  ======== CSL_Strncat ========
 *  Purpose:
 *      Concatenate two strings together. 
 *  Parameters:
 *      pszDest:    Destination string.
 *      pszSrc:     Source string.
 *      dwSize:     Number of characters to copy.
 *  Returns:
 *      Pointer to destination string.
 *  Requires:
 *      CSL initialized.
 *      pszDest and pszSrc are valid pointers.
 *  Ensures:
 */
	extern PSTR CSL_Strncat(IN PSTR pszDest,
				IN PSTR pszSrc, IN DWORD dwSize);

/*
 *  ======== CSL_Strncmp ========
 *  Purpose:
 *      Compare at most n characters of two ASCII strings.  Works the same
 *      way as stdio's strncmp.
 *  Parameters:
 *      pstrStr1:   String 1.
 *      pstrStr2:   String 2.
 *      n:          Number of characters to compare.
 *  Returns:
 *      A signed value that gives the results of the comparison:
 *      Zero:   String1 equals String2.
 *      < Zero: String1 is less than String2.
 *      > Zero: String1 is greater than String2.
 *  Requires:
 *      CSL initialized.
 *      pstrStr1 is valid.
 *      pstrStr2 is valid.
 *  Ensures:
 */
	extern LONG CSL_Strncmp(IN CONST PSTR pstrStr1,
				IN CONST PSTR pstrStr2, IN DWORD n);

/*
 *  ======== CSL_Strtok ========
 *  Purpose:
 *      Tokenize a NULL terminated string
 *  Parameters:
 *      ptstrSrc:       pointer to string.
 *      szSeparators:   pointer to a string of seperators
 *  Returns:
 *      String
 *  Requires:
 *      CSL initialized.
 *      ptstrSrc is a valid string pointer.
 *      szSeparators is a valid string pointer.
 *  Ensures:
 */
	extern CHAR *CSL_Strtok(IN CHAR * ptstrSrc,
				IN CONST CHAR * szSeparators);

/*
 *  ======== CSL_Strtokr ========
 *  Purpose:
 *      Re-entrant version of strtok.
 *  Parameters:
 *      pstrSrc:        Pointer to string. May be NULL on subsequent calls.
 *      szSeparators:   Pointer to a string of seperators
 *      ppstrCur:       Location to store start of string for next call to
 *                      to CSL_Strtokr.
 *  Returns:
 *      String (the token)
 *  Requires:
 *      CSL initialized.
 *      szSeparators != NULL
 *      ppstrCur != NULL
 *  Ensures:
 */
	extern CHAR *CSL_Strtokr(IN CHAR * pstrSrc,
				 IN CONST CHAR * szSeparators,
				 OUT CHAR ** ppstrCur);

#ifdef UNICODE
/*
 *  ======== CSL_WcharToAnsi ========
 *  Purpose:
 *      Convert a wide char string to an ansi string.
 *  Parameters:
 *     pstrDest:    ansi string buffer pointer.
 *     pwszSource:  wide char buffer pointer.
 *     uSize:       number of chars to convert.
 *  Returns:
 *      Number of characters copied into pstrDest.
 *  Requires:
 *      CSL initialized.
 *  Ensures:
 *  Details:
 *      lNumOfChars is the number of CHARACTERS in wpstrDest, NOT the number of 
 *      BYTES
 */
	extern ULONG CSL_WcharToAnsi(OUT PSTR pstrDest,
				     IN WCHAR * pwszSource, IN ULONG uSize);

/*
 *  ======== CSL_Wstrlen ========
 *  Purpose:
 *      Determine the length of a null terminated UNICODE string.
 *  Parameters:
 *      ptstrSrc: pointer to string.
 *  Returns:
 *      String length in bytes.
 *  Requires:
 *      CSL initialized.
 *      ptstrSrc is a valid string pointer.
 *  Ensures:
 */
	extern DWORD CSL_Wstrlen(IN CONST TCHAR * ptstrSrc);
#endif

#ifdef __cplusplus
}
#endif
#endif				/* CSL_ */
