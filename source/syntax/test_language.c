/* test_language.c -- Test functions in language.c
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "keyword.h"
#include "language.h"

extern char is_normal[256];
extern char is_alpha[256];
extern char is_numeric[256];
extern char is_upper[256];
extern char is_lower[256];

void test_word_types(char *text, struct keyword_info *info)
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
                get_word_type_text(get_word_type(text, word_start, word_end, info)));

         word_start = indent;
         was_abnormal = 1;
      }
   }
}

void test_is_reserved_word(char *text, ULONG word_start, ULONG word_end, int is, struct keyword_info *info)
{
   int actual_is;

   printf("is_reserved_word(\"%s\", %ld,%ld)\n", text, word_start, word_end);
   actual_is = is_reserved_word(text, word_start, word_end, info);
   printf("  -> %d\n", actual_is);
   assert(actual_is == is);
}

int main(int argc, char *argv[])
{
   struct keyword_info *info;

   setup_char_array();
   create_keyword_list();
   info = keywords_of("golded:add-ons/eiffel/syntax/eiffel.keyword");

   if (info == NULL) {
      printf("no keyword info\n");
   } else {
      STRPTR *word = info->word;
      ULONG  word_count = info->word_count;

      printf("%d reserved words: \"%s\"..\"%s\"\n", word_count,
             word[0], word[word_count - 1]);

      printf("word_type(is)   = %d\n", get_word_type("is", 0, 2, info));
      printf("word_type(Sepp) = %d\n", get_word_type("Sepp", 0, 4, info));
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
      test_is_reserved_word("sepp", 0, 4, 0, info);
      test_is_reserved_word("old stuff", 0, 3, 1, info);
      test_is_reserved_word("alias", 0, 5, 1, info);
      test_is_reserved_word("index", 0, 5, 0, info);
      test_is_reserved_word("pri", 0, 3, 0, info);
      test_is_reserved_word("xor", 0, 3, 1, info);

      test_word_types("  sepp := old sepp+Ranz_hanz;   "
                      "  class HUGO feature is nothing;", info);
#endif
   }
   dispose_keyword_list();

   return 0;
}

