/* test_eiffel.c -- Test ScanHandlerEiffel. */

#include <stdio.h>
#include <string.h>

#ifdef LATTICE
#define __A0
#define __A1
#define __D0
#define __COMMODORE_DATE__ __DATE__
#endif

/* This violently includes the scanner */
#include "eiffel.c"

static void test_handler(char *text)
{
   ULONG length = strlen(text);
   printf("%s\n", text);
   text[length] = '?';          /* Remove 0-termination */

   length = ScanHandlerEiffel(length, &text, NULL);

   if (length > 0)
   {
      printf("--> «");
      while (length > 0)
      {
         printf("%c", *text);
         text += 1;
         length -= 1;
      }
      printf("»\n");
   }
}

int main(void)
{
   test_handler("sepp(hugo: INTEGER) is");
   test_handler("sepp is");
   test_handler("-- Don't show up because this is");  /* a comment */
   test_handler("frozen sepp(hugo: INTEGER) is");
   test_handler("frozen conforms_to(other: GENERAL): BOOLEAN is");
   test_handler("infix \"#<<\"(s: INTEGER): like Current is");
   test_handler("frozen infix \"+\"(s: INTEGER): like Current is");
   return 0;
}
