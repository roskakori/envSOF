#ifndef TEST_STRING_STUFF
#define TEST_STRING_STUFF 0
#endif

typedef unsigned long ULONG;

#include <stdlib.h>

#if TEST_STRING_STUFF
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define DD(x) x
#define bbug  printf
#else
#define assert(x)
#define DD(x)
#define bbug
#endif

#include "language.h"

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

/*
 * IMPORTANT: These lists have to be sorted because they are searched binary!
 */
const char *reserved_word[] =
{
   "alias",
   "all",
   "and",
   "as",
   "check",
   "class",
   "creation",
   "debug",
   "deferred",
   "do",
   "else",
   "elseif",
   "end",
   "ensure",
   "expanded",
   "export",
   "external",
   "feature",
   "from",
   "frozen",
   "if",
   "implies",
   "indexing",
   "infix",
   "inherit",
   "inspect",
   "invariant",
   "is",
   "like",
   "local",
   "loop",
   "not",
   "obsolete",
   "old",
   "once",
   "or",
   "prefix",
   "redefine",
   "rename",
   "require",
   "rescue",
   "retry",
   "select",
   "separate",
   "strip",
   "then",
   "undefine",
   "until",
   "variant",
   "when",
   "xor",
   NULL
};

const char *reserved_name[] =
{
   "Current",
   "False",
   "Precursor",
   "Result",
   "True",
   "Unique",
   "Void",
   NULL
};

const int RESERVED_WORD_COUNT = (sizeof(reserved_word) / sizeof(char *)) - 1;
const int RESERVED_NAME_COUNT = (sizeof(reserved_name) / sizeof(char *)) - 1;

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

char *fsearch(const char *text, ULONG word_start, ULONG word_end,
              const char *base[], const int maximum_count)
{
   const char *word = &text[word_start];
   ULONG word_length = word_end - word_start;
   int a = 0;
   int b = maximum_count;
   int c;
   int d;

   DD(bbug("  fsearch(\"%s\",%ld) in %d\n", word, word_length, maximum_count));
   for (;;)
   {
      c = (a + b) / 2;
      //DD(bbug("  %2d [%2d,%2d]: %s\n", c, a, b, reserved_word[c]));
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

int is_reserved_word(const char *word, ULONG word_start, ULONG word_end)
{
   return (fsearch(word, word_start, word_end, reserved_word, RESERVED_WORD_COUNT) != NULL);
}

int is_reserved_name(const char *name, ULONG name_start, ULONG name_end)
{
   return (fsearch(name, name_start, name_end, reserved_name, RESERVED_NAME_COUNT) != NULL);
}

int get_word_type(const char *text, ULONG word_start, ULONG word_end)
{
   int word_type = SYNTAX_TEXT;
   ULONG word_index = word_start;

   if (is_lower[text[word_start]])
   {
      // first letter lower case
      if (is_reserved_word(text, word_start, word_end))
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
      if (is_reserved_name(text, word_start, word_end))
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

#if TEST_STRING_STUFF


void test_word_types(char *text)
{
   ULONG indent = 0;
   ULONG length = strlen(text);
   ULONG word_start = 0;
   ULONG word_end = 0;
   int was_abnormal = 1;

   printf("test_word_types(\"%s\")\n", text);
   for (indent = 0; indent < length; indent++)
   {
      if (was_abnormal)
      {
         if (is_normal[text[indent]]) {
            was_abnormal = 0;
            word_start = indent;
         }
      }
      else if (!is_normal[text[indent]]) {
         char buffer;

         word_end = indent;

         buffer = text[word_end];
         text[indent] = 0;
         printf("  %2ld-%2d: \"%s\"\n", word_start, word_end-1, &text[word_start]);
         text[word_end] = buffer;

         printf("         %s\n",
                get_word_type_text(get_word_type(text, word_start, word_end)));

         word_start = indent;
         was_abnormal = 1;
      }
   }
}

void test_is_reserved_word(char *text, ULONG word_start, ULONG word_end, int is)
{
   int actual_is;

   printf("is_reserved_word(\"%s\", %ld,%ld)\n", text, word_start, word_end);
   actual_is = is_reserved_word(text, word_start, word_end);
   printf("  -> %d\n", actual_is);
   assert(actual_is == is);
}

int main(int argc, char *argv[])
{
   setup_char_array();

   printf("%d reserved words: \"%s\"..\"%s\"\n", RESERVED_WORD_COUNT,
          reserved_word[0], reserved_word[RESERVED_WORD_COUNT - 1]);

   printf("word_type(is)   = %d\n", get_word_type("is", 0, 2));
   printf("word_type(Sepp) = %d\n", get_word_type("Sepp", 0, 4));
   printf("is_alpha: a b 1 _ !   ä\n"
          "          %d %d %d %d %d %d %d\n",
                    is_alpha['a'],
                    is_alpha['b'],
                    is_alpha['1'],
                    is_alpha['_'],
                    is_alpha['!'],
                    is_alpha[' '],
                    is_alpha[(unsigned char) 'ä']);
   printf("fsc(index,indexing) = %d\n", fstrncmp("index", "indexing", 5));
   printf("fsc(class,class) = %d\n", fstrncmp("class", "class", 5));

#if 1
   test_is_reserved_word("sepp", 0, 4, 0);
   test_is_reserved_word("old stuff", 0, 3, 1);
   test_is_reserved_word("alias", 0, 5, 1);
   test_is_reserved_word("index", 0, 5, 0);
   test_is_reserved_word("pri", 0, 3, 0);
   test_is_reserved_word("xor", 0, 3, 1);

   test_word_types("  sepp := old sepp+Ranz_hanz;   "
                   "  class HUGO feature is nothing;");
#endif

   return 0;
}
#endif
