/* test_keyword.c -- Test functions in keyword.c
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */
#include "keyword.c"

int main(int argc, char *argv[])
{
   struct keyword_info *info = NULL;
   create_keyword_list();

   info = keywords_of("golded:add-ons/eiffel/syntax/xxxxx.keyword");
   info = keywords_of("golded:add-ons/eiffel/syntax/short.keyword");

   dispose_keyword_list();

   return 0;
}
