/*
 * debug.c -- Debugging trace routines.
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

#include "defs.h"
#include "debug.h"

#ifdef _DCC

#include <exec/types.h>
#include <clib/dos_protos.h>
ULONG debugFH = NULL;

#else /* _DCC */

#include <stdio.h>

#endif /* _DCC */

Prototype void kputs(char *text);
Prototype void kint(int number);


// print debugging message; automatically opens a console
// NOTE: The console is never closed, thus keeping a lock and resources.
//       This is ugly, but I don't know how to use kprintf() in DICE.
//       Nevertheless you can close the console window by clicking its
//       close gadget.
void kputs(char *text)
{
#ifdef _DCC
   if (debugFH == NULL) {
      debugFH = Open("con:9999//200/700/Debug/AUTO/INACTIVE/SCREEN*", 1006);
   }
   Write(debugFH, text, strlen(text));
#else
   printf("%s");
#endif
}

// display integer in debugging console
void kint(int number)
{
   char *digits3 = "1234";
   digits3[0] = (number / 1000) % 10 + '0';
   digits3[1] = (number / 100) % 10 + '0';
   digits3[2] = (number / 10) % 10 + '0';
   digits3[3] = (number / 1) % 10 + '0';

   kputs(digits3);
}

