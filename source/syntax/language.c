/*
 * language.c -- Programing language specific functions
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

#include "defs.h"

#include "debug.h"
#include "language.h"
#include "keyword.h"

#include <stdlib.h>

Prototype void setup_char_array(void);
Prototype int fstrncmp(const char *s1, const char *s2, ULONG n);
Prototype int is_reserved_word(const char *word, ULONG word_start, ULONG word_end, struct keyword_info *info);
Prototype int is_reserved_name(const char *name, ULONG name_start, ULONG name_end, struct keyword_info *info);
Prototype int get_word_type(const char *text, ULONG word_start, ULONG word_end, struct keyword_info *info);
Prototype char *get_word_type_text(int word_type);

/// "Eiffel words"

static const char normal_char[] =
"abcdefghijklmnopqrstuvwxyz"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"_01234567890";
static const char alpha_char[] =
"abcdefghijklmnopqrstuvwxyz"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"_";

static const char numeric_char[] = "._0123456789";
static const char lower_char[] = "abcdefghijklmnopqrstuvwxyz";
static const char upper_char[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char is_normal[256] =
{0};

char is_alpha[256] =
{0};

char is_numeric[256] =
{0};

char is_upper[256] =
{0};

char is_lower[256] =
{0};

void setup_char_array(void)
{
   int i;

   /* alphabetical identifiers */
   i = 0;
   while (alpha_char[i])
   {
      is_alpha[alpha_char[i]] = 1;
      i += 1;
   }

   /* normal identifiers */
   i = 0;
   while (normal_char[i])
   {
      is_normal[normal_char[i]] = 1;
      i += 1;
   }

   /* numbers */
   i = 0;
   while (numeric_char[i])
   {
      is_numeric[numeric_char[i]] = 1;
      i += 1;
   }

   /* upper case */
   i = 0;
   while (upper_char[i])
   {
      is_upper[upper_char[i]] = 1;
      i += 1;
   }

   /* lower case */
   i = 0;
   while (lower_char[i])
   {
      is_lower[lower_char[i]] = 1;
      i += 1;
   }
}


int fstrncmp(const char *s1, const char *s2, ULONG n)
{
   ULONG c = 0;
   char c1, c2;

   while (*s2 && (*s1 == *s2) && (c < n))
   {
      s1++;
      s2++;
      c++;
   }

   c2 = *s2;
   if (c == n) {
      c1 = 0;
   } else {
      c1 = *s1;
   }

   if (c1 < c2)
      return -1;
   else if (c1 > c2)
      return 1;
   else
      return 0;
}

static char *fsearch(const char *text, ULONG word_start, ULONG word_end,
              const char *base[], const ULONG maximum_count)
{
   const char *word = &text[word_start];
   ULONG word_length = word_end - word_start;
   int a = 0;
   int b = maximum_count;
   int c;
   int d;

   DD(bug("  fsearch(\"%s\",%ld) in %d\n", word, word_length, maximum_count));
   for (;;)
   {
      c = (a + b) / 2;
      //DD(bug("  %2d [%2d,%2d]: %s\n", c, a, b, reserved_word[c]));
      if ((d = fstrncmp(word, base[c], word_length)) == 0)
         return base[c];
      if (c == a)
         break;
      if (d < 0)
         b = c;
      else
         a = c;
   }
   return NULL;
}

int is_reserved_word(const char *word, ULONG word_start, ULONG word_end, struct keyword_info *info)
{
   return ((is_lower[word[word_start]]) && (fsearch(word, word_start, word_end, info->word, info->word_count) != NULL));
}

int is_reserved_name(const char *name, ULONG name_start, ULONG name_end, struct keyword_info *info)
{
   return ((is_upper[name[name_start]]) && (fsearch(name, name_start, name_end, info->word, info->word_count) != NULL));
}

int get_word_type(const char *text, ULONG word_start, ULONG word_end, struct keyword_info *info)
{
   int word_type = SYNTAX_TEXT;
   ULONG word_index = word_start;

   if (is_lower[text[word_start]])
   {
      // first letter lower case
      if (is_reserved_word(text, word_start, word_end, info))
      {
         word_type = SYNTAX_KEYWORD;
      }
      else
      {
         word_type = SYNTAX_TEXT;
         while (word_index < word_end)
         {
            if (is_upper[text[word_index]])
            {
               word_type = SYNTAX_BAD;
            }
            word_index += 1;
         }
      }
   }
   else
   {
      // first letter upper case
      if (is_reserved_name(text, word_start, word_end, info))
      {
         word_type = SYNTAX_RESERVED_WORD;
      }
      else
      {

         int lower_count = 0;
         int upper_count = 0;
         word_type = SYNTAX_TYPE;
         word_index += 1;
         while (word_index < word_end)
         {
            if (is_upper[text[word_index]])
            {
               upper_count += 1;
            }
            if (is_lower[text[word_index]])
            {
               lower_count += 1;
            }
            word_index += 1;
         }
         if (lower_count > 0)
         {
            if (upper_count == 0)
            {
               word_type = SYNTAX_CONSTANT;
            }
            else
            {
               word_type = SYNTAX_BAD;
            }
         }
         else
         {
            word_type = SYNTAX_TYPE;
         }
      }
   }

   return word_type;
}

char *get_word_type_text(int word_type)
{
   char *type_text = "normal";

   switch (word_type)
   {
      case SYNTAX_KEYWORD:
         type_text = "keyword";
         break;
      case SYNTAX_RESERVED_WORD:
         type_text = "reserved";
         break;
      case SYNTAX_CONSTANT:
         type_text = "constant";
         break;
      case SYNTAX_TEXT:
         type_text = "variable";
         break;
      case SYNTAX_TYPE:
         type_text = "class";
         break;
   }

   return type_text;
}

