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
ULONG debug_handle = NULL;
int debug_count = 0;

#else /* _DCC */

#include <stdio.h>

#endif /* _DCC */

Prototype void kputs(char *text);
Prototype void kint(int number);
Prototype void kputptr(void *pointer);

void
kputs(char *text)
{
#ifdef _DCC
   Write(debug_handle, text, strlen(text));
#else
   printf("%s");
#endif
}

// display integer in debugging console
void
kint(int number)
{
   char digit_buffer[6];
   char *digits;
   char minus = 0;

   if (number < 0) {
      number = - number;
      minus = 1;
   }

   digit_buffer[0] = (number / 10000) % 10 + '0';
   digit_buffer[1] = (number / 1000) % 10 + '0';
   digit_buffer[2] = (number / 100) % 10 + '0';
   digit_buffer[3] = (number / 10) % 10 + '0';
   digit_buffer[4] = (number / 1) % 10 + '0';
   digit_buffer[5] = '\0';

   // skip leading zeros
   digits = digit_buffer;
   while (*digits == '0') digits+=1;
   if (*digits== '\0') digits-=1;

   if (minus) {
      kputs("-");
   }
   kputs(digits);
}

static char
hex_as_char(char hex)
{
   if (hex < 10) {
      hex += '0';
   } else {
      hex += ('a' - 10);
   }
   return hex;
}
void
kdump(char *memory, size_t count)
{
   char *hex = "xx ";
   size_t i = 0;

   while (i < count) {
      if ((i & 7) == 0) {
         if (i != 0) {
            kputs("\n");
         }
         kint(i);
         kputs(": ");
      }
      hex[0] = hex_as_char(((unsigned char) memory[i]) >> 4);
      hex[1] = hex_as_char(memory[i] & 0x0f);
      kputs(hex);
      i += 1;
   }
   kputs("\n");
}

static char
shasc(ULONG value, int bits)
{
   return hex_as_char((value >> bits) & 0xf);
}

// display pointer in hex notation
void
kputptr(void *pointer)
{
   char *text = "12345678";
   ULONG address = (ULONG) pointer;

   text[0] = shasc(address, 28);
   text[1] = shasc(address, 24);
   text[2] = shasc(address, 20);
   text[3] = shasc(address, 16);
   text[4] = shasc(address, 12);
   text[5] = shasc(address, 8);
   text[6] = shasc(address, 4);
   text[7] = shasc(address, 0);

   kputs("0x");
   kputs(text);
}

void init_debug(void)
{
#ifdef _DCC
   if (debug_handle == NULL) {
      debug_handle = Open("con:0//512/200/Debug/AUTO/WAIT/INACTIVE/SCREEN*", 1006);
   }
   debug_count += 1;
#endif
}

void cleanup_debug(void)
{
#ifdef _DCC
   debug_count -= 1;
   if (debug_count == 0) {
      Close(debug_handle);
      debug_handle = NULL;
   }
#endif
}
