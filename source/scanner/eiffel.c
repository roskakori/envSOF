/*
 * eiffel.c -- Eiffel feature scanner for GoldEd 4+.
 *
 * Copyright 1999 Thomas Aglassinger <agi@sbox.tu-graz.ac.at>
 *
 * Compile: "dmake"
 *
 */

#include <exec/types.h>

#define VERSION "$VER: eiffel 1.1 (" __COMMODORE_DATE__ ")"

/* The actual handler has to be the first prototype, because GoldEd
 * will *always* use the first function as handler. */
ULONG ScanHandlerEiffel(__D0 ULONG len, __A0 char **text, __A1 ULONG * line);

static UBYTE *try_skip_word(UBYTE * from, UBYTE * word);
static void seek_non_identifier(UBYTE **from, UBYTE *last);
static void skip_blanks(UBYTE **rider, UBYTE *last);

ULONG ScanHandlerEiffel(__D0 ULONG len, __A0 char **text, __A1 ULONG * line)
{
   if (len > 3)
   {
      UBYTE *from = *text;
      UBYTE *last;

      /* Skip backwards trailing blanks */
      for (last = from + len - 1; len && (*last == 32); --len)
         --last;

      /* Check for " is" at end of line */
      if ((last[0] == 's') && (last[-1] == 'i') && (last[-2] == ' '))
      {
         UBYTE *skipper = NULL; /* Seeks for "frozen", "infix" etc */
         BOOL after_hyphen = FALSE;

         skip_blanks(&from, last);

         /* If line contains a comment, don't consider it further. */
         skipper = from;
         while (skipper < last)
         {
            if (*skipper == '-')
            {
               if (after_hyphen)
               {
                  return FALSE; /* Forget it (violent return) */
               }
               else
               {
                  after_hyphen = TRUE;
               }
            }
            else
            {
               after_hyphen = FALSE;
            }
            skipper += 1;
         }

         /* Skip frozen */
         skipper = try_skip_word(from, "frozen ");
         if (skipper != NULL)
         {
            from = skipper;
            skipper = NULL;
         }

         /* Check for infix/prefix */
         skipper = try_skip_word(from, "infix ");
         if (skipper == NULL)
         {
            skipper = try_skip_word(from, "prefix ");
         }

         if (skipper == NULL)
         {
            /* Handle normal feature */
            skipper = from;
            seek_non_identifier(&skipper, last);
            last = skipper;
         }
         else
         {
            /* Handle infix/prefix */
            ULONG skipped_quotes = 0;

            skip_blanks(&skipper, last);

            while ((skipper <= last) && (skipped_quotes < 2))
            {
               if (*skipper == '"')
               {
                  skipped_quotes += 1;
               }
               skipper += 1;
            }
            last = skipper;
         }

         *text = from;
         return ((ULONG) (last - from));
      }
   }

   return (FALSE);
}

/* Compare string beginning at `from' with 0-terminated `word'.
 * Return pointer character of `from' after `word', or NULL if
 * `from' does not begin with `word'. */
static UBYTE *try_skip_word(UBYTE * from, UBYTE * word)
{
   while (*word && (*word == *from))
   {
      word += 1;
      from += 1;
   }

   if (*word)
   {
      from = NULL;
   }

   return from;
}

/* Seek from current position to first non-identifier character */
static void seek_non_identifier(UBYTE **from, UBYTE *last)
{
   UBYTE *rider = *from;

   while ((rider <= last)
          && ((*rider == '_')
              || ((*rider >= 'A') && (*rider <= 'Z'))
              || ((*rider >= 'a') && (*rider <= 'z'))
              || ((*rider >= '0') && (*rider <= '9'))
          ))
   {
      rider += 1;
   }

   *from = rider;
}

/* Skip blanks */
static void skip_blanks(UBYTE **rider, UBYTE *last)
{
   while ((*rider <= last) && (**rider == 32))
   {
      *rider = *rider + 1;
   }
}

const char *version = VERSION;
