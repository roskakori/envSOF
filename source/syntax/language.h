/*
 * language.h
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

// Syntax levels
#define SYNTAX_TEXT          0          // normal text
#define SYNTAX_COMMENT       1          // comments
#define SYNTAX_STRING        2          // string constants
#define SYNTAX_KEYWORD       3          // Eiffel keywords
#define SYNTAX_RESERVED_WORD 4          // Eiffel keynames
#define SYNTAX_TYPE          5          // types: class & generic parameters
#define SYNTAX_CONSTANT      6          // constants
#define SYNTAX_DECLARATION   8          // features declaration
#define SYNTAX_BAD           7 /* !! */ // bad names JavaLikeCrap

#define SYNTAX_MAXIMUM       8 /* !! */


