/*
 * eiffel.c -- Eiffel feature scanner for GoldEd Studio 6.
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 *
 */

#include <exec/types.h>

#define VERSION "$VER: eiffel 2.0 (" __COMMODORE_DATE__ ")"

/* The actual handler has to be the first prototype, because GoldEd
 * will *always* use the first function as handler. */
ULONG ScanHandlerEiffel(__D0 ULONG len, __A0 char **text, __A1 ULONG * line);

static UBYTE *try_skip_word(UBYTE * from, UBYTE * word);
static void seek_non_identifier(UBYTE ** from, UBYTE * last);
static void skip_blanks(UBYTE ** rider, UBYTE * last);
static int is_letter(UBYTE byte);

ULONG
ScanHandlerEiffel(__D0 ULONG len, __A0 char **text, __A1 ULONG * line)
{
   if (len > 3) {
      UBYTE *from = *text;
      UBYTE *last;
      /* Find out if this line is a feature declaration */
      if ((from[0] == '\t') && is_letter(from[1])) {
         from += 1;
         len -= 1;
      } else if ((from[0] == ' ') && (from[0] == ' ')
                 && (from[0] == ' ') && is_letter(from[3])) {
         from += 3;
         len -= 3;
      } else {
         len = 0;
      }

      if (len > 0) {
         UBYTE *skipper = NULL; /* Seeks for "frozen", "infix" etc */
         BOOL has_feature = FALSE;

         /* Skip backwards trailing blanks */
         for (last = from + len - 1; len && (*last <= 32); --len)
            --last;

         /* Does line end in " is"? */
         has_feature = (last[0] == 's') && (last[-1] == 'i') && (last[-2] == ' ');
         if (!has_feature) {
            /* Does line have a ":" somewhere? Otherwise, this is just
             * a "creation" routine */
            skipper = from;
            while ((skipper <= last) && (!has_feature)) {
               if (*skipper == ':') {
                  /* Is it an attribute or an indexing line? Check first
                   * non-white space after ":" to not be a quote (") */
                  skipper+=1;
                  skip_blanks(&skipper, last);
                  if (*skipper != '"') {
                     /* Check if there are any commas (,) in the line,
                      * except inside quotes. If there is, it's an indexing
                      * line. */
                     BOOL figured_it = FALSE;
                     while ((skipper <= last) && (!figured_it)) {
                        switch(*skipper)
                        {
                           case '\"':
                           case '\'':
                              /* The only cases we can't discover with this
                               * are indexing lines like:
                               *
                               * indexing
                               *    year: 1999
                               */
                              has_feature = TRUE;
                              figured_it = TRUE;
                              break;
                           case ',':
                              figured_it = TRUE;
                              break;
                           default:
                              skipper += 1;
                              break;
                        }
                     }
                  }
               } else {
                  skipper += 1;
               }
            }
         }
         if (has_feature) {
            /* Skip frozen */
            skipper = try_skip_word(from, "frozen ");
            if (skipper != NULL) {
               from = skipper;
               skipper = NULL;
            }
            /* Check for infix/prefix */
            skipper = try_skip_word(from, "infix ");
            if (skipper == NULL) {
               skipper = try_skip_word(from, "prefix ");
            }
            if (skipper == NULL) {
               /* Handle normal feature */
               skipper = from;
               seek_non_identifier(&skipper, last);
               last = skipper;
            } else {
               /* Handle infix/prefix */
               ULONG skipped_quotes = 0;

               skip_blanks(&skipper, last);

               while ((skipper <= last) && (skipped_quotes < 2)) {
                  if (*skipper == '"') {
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
   }
   return (FALSE);
}

/* Compare string beginning at `from' with 0-terminated `word'.
 * Return pointer character of `from' after `word', or NULL if
 * `from' does not begin with `word'. */
static UBYTE *
try_skip_word(UBYTE * from, UBYTE * word)
{
   while (*word && (*word == *from)) {
      word += 1;
      from += 1;
   }

   if (*word) {
      from = NULL;
   }
   return from;
}

/* Seek from current position to first non-identifier character */
static void
seek_non_identifier(UBYTE ** from, UBYTE * last)
{
   UBYTE *rider = *from;

   while ((rider <= last)
          && ((*rider == '_')
              || ((*rider >= 'A') && (*rider <= 'Z'))
              || ((*rider >= 'a') && (*rider <= 'z'))
              || ((*rider >= '0') && (*rider <= '9'))
          )) {
      rider += 1;
   }

   *from = rider;
}

/* Skip blanks */
static void
skip_blanks(UBYTE ** rider, UBYTE * last)
{
   while ((*rider <= last) && (**rider == 32)) {
      *rider = *rider + 1;
   }
}

static int
is_letter(UBYTE byte)
{
   return (((byte >= 'a') && (byte <= 'z'))
           || ((byte >= 'A') && (byte <= 'Z')));
}

static const char *version = VERSION;
